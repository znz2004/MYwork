import inspect
import yaml
import importlib
import sys
from typing import Any, Dict, Type, TypeVar, List, Set, Optional

from exceptions import CircularDependencyError
from decorators import component, lazy, inject, post_construct, pre_destroy

T = TypeVar('T')


class Container:
    """IoC 容器实现"""
    
    def __init__(self, config_file: str = None):
        self._registry: Dict[Type, Dict] = {}
        self._instances: Dict[Type, Any] = {}
        self._creating: Set[Type] = set()
        self._interface_impl: Dict[Type, Type] = {}
        self._config_file = config_file
        
        if config_file:
            self._load_from_config(config_file)
    
    def _load_from_config(self, config_file: str):
        """从 YAML 配置文件加载构件定义"""
        with open(config_file, 'r', encoding='utf-8') as f:
            config = yaml.safe_load(f)
        
        for comp_config in config.get('components', []):
            class_path = comp_config['class']
            properties = comp_config.get('properties', {})
            lazy_load = comp_config.get('lazy', False)
            
            # 动态导入类
            module_path, class_name = class_path.rsplit('.', 1)
            module = importlib.import_module(module_path)
            cls = getattr(module, class_name)
            
            # 注册到容器
            self.register(cls, lazy_load=lazy_load, properties=properties)
    
    def register(self, cls: Type[T], lazy_load: bool = False, properties: dict = None):
        """注册构件到容器"""
        if cls in self._registry:
            return
        
        is_lazy = lazy_load or getattr(cls, '__lazy__', False)
        
        self._registry[cls] = {
            'class': cls,
            'lazy': is_lazy,
            'properties': properties or {},
            'instance': None
        }
        
        # 记录接口映射
        for base in cls.__bases__:
            if base != object:
                self._interface_impl[base] = cls
        
        if not is_lazy:
            self._get_or_create(cls)
    
    def _get_implementation(self, interface: Type) -> Type:
        """获取接口的具体实现类"""
        if interface in self._interface_impl:
            return self._interface_impl[interface]
        
        for cls in self._registry.keys():
            try:
                if cls != interface and issubclass(cls, interface):
                    self._interface_impl[interface] = cls
                    return cls
            except TypeError:
                continue
        
        return interface
    
    def _resolve_type(self, type_hint):
        """解析类型标注，处理字符串前向引用"""
        if isinstance(type_hint, str):
            # 尝试在 sys.modules 中查找
            for module_name, module in sys.modules.items():
                if hasattr(module, type_hint):
                    return getattr(module, type_hint)
        return type_hint
    
    def _has_dependencies(self, cls: Type[T]) -> bool:
        try:
            signature = inspect.signature(cls.__init__)
            params = list(signature.parameters.values())
            return len(params) > 1
        except:
            return False
    
    def _resolve_dependencies(self, cls: Type[T], dependency_chain: List[Type] = None) -> dict:
        if dependency_chain is None:
            dependency_chain = []
        
        try:
            signature = inspect.signature(cls.__init__)
        except:
            return {}
        
        dependencies = {}
        
        for param_name, param in signature.parameters.items():
            if param_name == 'self':
                continue
            
            param_type = self._resolve_type(param.annotation)
            
            if param_type == inspect.Parameter.empty or not isinstance(param_type, type):
                continue
            
            actual_type = self._get_implementation(param_type)
            
            if actual_type in dependency_chain:
                raise CircularDependencyError(dependency_chain + [actual_type])
            
            try:
                if actual_type not in self._registry:
                    if hasattr(actual_type, '__component__') and actual_type.__component__:
                        self.register(actual_type)
                    else:
                        continue
                
                dependency_instance = self._get_or_create(actual_type, dependency_chain + [cls])
                dependencies[param_name] = dependency_instance
                
            except CircularDependencyError as e:
                raise e
            except Exception:
                continue
        
        return dependencies
    
    def _get_or_create(self, cls: Type[T], dependency_chain: List[Type] = None) -> T:
        if dependency_chain is None:
            dependency_chain = []
        
        if cls in dependency_chain:
            raise CircularDependencyError(dependency_chain + [cls])
        
        if cls in self._instances:
            return self._instances[cls]
        
        if cls not in self._registry:
            if hasattr(cls, '__component__') and cls.__component__:
                self.register(cls)
            else:
                raise KeyError(f"类 {cls.__name__} 未注册到容器")
        
        reg_info = self._registry[cls]
        self._creating.add(cls)
        
        try:
            dependencies = self._resolve_dependencies(cls, dependency_chain + [cls])
            
            try:
                instance = cls(**dependencies)
            except TypeError:
                # 如果参数不匹配，尝试无参构造
                instance = cls()
            
            self._perform_setter_injection(instance, dependency_chain + [cls])
            
            for prop_name, prop_value in reg_info['properties'].items():
                if hasattr(instance, prop_name):
                    setattr(instance, prop_name, prop_value)
            
            self._invoke_post_construct(instance)
            
            self._instances[cls] = instance
            reg_info['instance'] = instance
            
            return instance
            
        finally:
            self._creating.remove(cls)
    
    def _perform_setter_injection(self, instance: Any, dependency_chain: List[Type]):
        for method_name in dir(instance):
            method = getattr(instance, method_name)
            if callable(method) and hasattr(method, '__inject__'):
                try:
                    signature = inspect.signature(method)
                except:
                    continue
                
                params = {}
                for param_name, param in signature.parameters.items():
                    if param_name == 'self':
                        continue
                    
                    param_type = self._resolve_type(param.annotation)
                    if param_type != inspect.Parameter.empty and isinstance(param_type, type):
                        actual_type = self._get_implementation(param_type)
                        
                        if actual_type in dependency_chain:
                            raise CircularDependencyError(dependency_chain + [actual_type])
                        
                        if actual_type not in self._registry:
                            if hasattr(actual_type, '__component__') and actual_type.__component__:
                                self.register(actual_type)
                            else:
                                continue
                        
                        try:
                            dep_instance = self._get_or_create(actual_type, dependency_chain + [type(instance)])
                            params[param_name] = dep_instance
                        except:
                            continue
                
                if params:
                    try:
                        method(**params)
                    except:
                        pass
    
    def _invoke_post_construct(self, instance: Any):
        for method_name in dir(instance):
            method = getattr(instance, method_name)
            if callable(method) and hasattr(method, '__post_construct__'):
                try:
                    method()
                except:
                    pass
    
    def get(self, cls: Type[T]) -> T:
        actual_cls = self._get_implementation(cls)
        
        if actual_cls not in self._registry:
            if hasattr(actual_cls, '__component__') and actual_cls.__component__:
                self.register(actual_cls)
            else:
                raise KeyError(f"类 {actual_cls.__name__} 未注册到容器")
        
        if self._registry[actual_cls]['lazy'] and actual_cls not in self._instances:
            return self._get_or_create(actual_cls)
        
        if actual_cls not in self._instances:
            return self._get_or_create(actual_cls)
        
        return self._instances[actual_cls]
    
    def close(self):
        for cls, instance in reversed(list(self._instances.items())):
            self._invoke_pre_destroy(instance)
        
        self._instances.clear()
        for reg_info in self._registry.values():
            reg_info['instance'] = None
    
    def _invoke_pre_destroy(self, instance: Any):
        for method_name in dir(instance):
            method = getattr(instance, method_name)
            if callable(method) and hasattr(method, '__pre_destroy__'):
                try:
                    method()
                except:
                    pass
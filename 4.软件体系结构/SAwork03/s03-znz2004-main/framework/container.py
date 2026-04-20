import inspect
import yaml
import importlib
from typing import Any, Dict, Type, TypeVar

T = TypeVar('T')


class Container:
    """IoC 容器"""
    
    def __init__(self, config_file: str = None):
        self._registry: Dict[Type, Dict] = {}
        self._instances: Dict[Type, Any] = {}
        self._config_file = config_file
        
        if config_file:
            self._load_from_config(config_file)
    
    def _load_from_config(self, config_file: str):
        with open(config_file, 'r', encoding='utf-8') as f:
            config = yaml.safe_load(f)
        
        for comp_config in config.get('components', []):
            class_path = comp_config['class']
            properties = comp_config.get('properties', {})
            
            module_path, class_name = class_path.rsplit('.', 1)
            module = importlib.import_module(module_path)
            cls = getattr(module, class_name)
            
            self._registry[cls] = {
                'class': cls,
                'properties': properties or {},
                'instance': None
            }
        
        # 创建所有实例
        for cls in list(self._registry.keys()):
            self._get_or_create(cls)
    
    def _get_parameter_type(self, param) -> Type:
        annotation = param.annotation
        if annotation == inspect.Parameter.empty:
            return None
        return annotation
    
    def _find_dependency(self, target_type: Type) -> Type:
        if target_type in self._registry:
            return target_type
        
        for reg_cls in self._registry.keys():
            try:
                if issubclass(reg_cls, target_type):
                    return reg_cls
            except TypeError:
                continue
        
        return None
    
    def _resolve_dependencies(self, cls: Type[T]) -> dict:
        try:
            sig = inspect.signature(cls.__init__)
        except:
            return {}
        
        dependencies = {}
        
        for param_name, param in sig.parameters.items():
            if param_name == 'self':
                continue
            
            param_type = self._get_parameter_type(param)
            
            if param_type is None:
                continue
            
            dep_cls = self._find_dependency(param_type)
            
            if dep_cls:
                dependencies[param_name] = self._get_or_create(dep_cls)
        
        return dependencies
    
    def _get_or_create(self, cls: Type[T]) -> T:
        if cls in self._instances:
            return self._instances[cls]
        
        if cls not in self._registry:
            raise KeyError(f"类 {cls.__name__} 未注册")
        
        reg_info = self._registry[cls]
        
        if reg_info['instance'] is not None:
            return reg_info['instance']
        
        dependencies = self._resolve_dependencies(cls)
        
        try:
            instance = cls(**dependencies)
        except TypeError:
            instance = cls()
        
        for prop_name, prop_value in reg_info['properties'].items():
            if hasattr(instance, prop_name):
                setattr(instance, prop_name, prop_value)
        
        self._invoke_post_construct(instance)
        
        self._instances[cls] = instance
        reg_info['instance'] = instance
        
        return instance
    
    def _invoke_post_construct(self, instance: Any):
        for method_name in dir(instance):
            method = getattr(instance, method_name)
            if callable(method) and hasattr(method, '__post_construct__'):
                try:
                    method()
                except:
                    pass
    
    def get(self, cls: Type[T]) -> T:
        return self._get_or_create(cls)
    
    def close(self):
        for instance in self._instances.values():
            self._invoke_pre_destroy(instance)
        self._instances.clear()
    
    def _invoke_pre_destroy(self, instance: Any):
        for method_name in dir(instance):
            method = getattr(instance, method_name)
            if callable(method) and hasattr(method, '__pre_destroy__'):
                try:
                    method()
                except:
                    pass
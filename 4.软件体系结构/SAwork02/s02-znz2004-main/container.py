import inspect
import yaml
import importlib
import sys
from typing import Any, Dict, Type, TypeVar, List, Set, Optional, Callable

from exceptions import CircularDependencyError, LayerViolationError
from decorators import component, lazy, inject, post_construct, pre_destroy

T = TypeVar('T')


class Container:
    """支持分层架构的 IoC 容器"""
    
    def __init__(self, config_file: str = None, layers: List[str] = None, 
                 mode: str = "strict"):
        self._registry: Dict[Type, Dict] = {}
        self._instances: Dict[Type, Any] = {}
        self._creating: Set[Type] = set()
        self._interface_impl: Dict[Type, Type] = {}
        self._aspects: List[Dict] = []
        self._commands: Dict[str, Dict] = {}
        self._layers = layers or []
        self._layer_index: Dict[str, int] = {}
        self._mode = mode
        self._config_file = config_file
        
        for i, layer in enumerate(self._layers):
            self._layer_index[layer] = i
        
        if config_file:
            self._load_from_config(config_file)
    
    def _load_from_config(self, config_file: str):
        with open(config_file, 'r', encoding='utf-8') as f:
            config = yaml.safe_load(f)
        
        if 'layers' in config and not self._layers:
            self._layers = config['layers']
            for i, layer in enumerate(self._layers):
                self._layer_index[layer] = i
        
        for aspect_config in config.get('aspects', []):
            aspect_path = aspect_config['path']
            module_path, func_name = aspect_path.rsplit('.', 1)
            module = importlib.import_module(module_path)
            func = getattr(module, func_name)
            self.register_aspect(
                func, 
                layer=aspect_config.get('layer'),
                point=aspect_config.get('point', 'around')
            )
        
        for comp_config in config.get('components', []):
            class_path = comp_config['class']
            properties = comp_config.get('properties', {})
            lazy_load = comp_config.get('lazy', False)
            layer = comp_config.get('layer')
            
            module_path, class_name = class_path.rsplit('.', 1)
            module = importlib.import_module(module_path)
            cls = getattr(module, class_name)
            
            self.register(cls, lazy_load=lazy_load, properties=properties, layer=layer)
    
    def register(self, cls: Type[T], lazy_load: bool = False, 
                 properties: dict = None, layer: str = None):
        if cls in self._registry:
            return
        
        actual_layer = layer or getattr(cls, '__layer__', None)
        is_lazy = lazy_load or getattr(cls, '__lazy__', False)
        
        self._registry[cls] = {
            'class': cls,
            'lazy': is_lazy,
            'layer': actual_layer,
            'properties': properties or {},
            'instance': None
        }
        
        # 建立接口映射
        for base in cls.__bases__:
            if base != object:
                if base not in self._interface_impl:
                    self._interface_impl[base] = cls
        
        if not is_lazy:
            self._get_or_create(cls)
    
    def register_aspect(self, func: Callable, layer: str = None, point: str = "around"):
        self._aspects.append({
            'func': func,
            'layer': layer,
            'point': point
        })
    
    def _check_layer_dependency(self, source_cls: Type, source_layer: str,
                                 target_cls: Type, target_layer: str):
        if source_layer is None or target_layer is None:
            return
        
        if source_layer == target_layer:
            return
        
        src_idx = self._layer_index.get(source_layer)
        tgt_idx = self._layer_index.get(target_layer)
        
        if src_idx is None or tgt_idx is None:
            return
        
        if self._mode == "strict":
            if tgt_idx != src_idx + 1:
                allowed = [self._layers[src_idx + 1]] if src_idx + 1 < len(self._layers) else []
                raise LayerViolationError(
                    source_layer, source_cls.__name__,
                    target_layer, target_cls.__name__,
                    allowed
                )
        else:
            if tgt_idx <= src_idx:
                allowed = [self._layers[i] for i in range(src_idx + 1, len(self._layers))]
                raise LayerViolationError(
                    source_layer, source_cls.__name__,
                    target_layer, target_cls.__name__,
                    allowed
                )
    
    def _get_implementation(self, interface: Type) -> Type:
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
        if isinstance(type_hint, str):
            for module_name, module in sys.modules.items():
                if hasattr(module, type_hint):
                    return getattr(module, type_hint)
        return type_hint
    
    def _resolve_dependencies(self, cls: Type[T], dependency_chain: List[Type] = None) -> dict:
        if dependency_chain is None:
            dependency_chain = []
        
        try:
            signature = inspect.signature(cls.__init__)
        except:
            return {}
        
        dependencies = {}
        source_layer = self._registry.get(cls, {}).get('layer')
        
        for param_name, param in signature.parameters.items():
            if param_name == 'self':
                continue
            
            param_type = self._resolve_type(param.annotation)
            
            if param_type == inspect.Parameter.empty or not isinstance(param_type, type):
                continue
            
            actual_type = self._get_implementation(param_type)
            
            if actual_type in dependency_chain:
                raise CircularDependencyError(dependency_chain + [actual_type])
            
            if actual_type in self._registry:
                target_layer = self._registry[actual_type].get('layer')
                self._check_layer_dependency(cls, source_layer, actual_type, target_layer)
            
            try:
                if actual_type not in self._registry:
                    if hasattr(actual_type, '__component__') and actual_type.__component__:
                        self.register(actual_type)
                    else:
                        continue
                
                dependency_instance = self._get_or_create(actual_type, dependency_chain + [cls])
                dependencies[param_name] = dependency_instance
                
            except (CircularDependencyError, LayerViolationError) as e:
                raise e
            except Exception:
                continue
        
        return dependencies
    
    def _apply_aspects(self, instance: Any, method_name: str, method: Callable):
        layer = self._registry.get(type(instance), {}).get('layer')
        
        matching_aspects = []
        for aspect in self._aspects:
            if aspect['layer'] is None or aspect['layer'] == layer:
                matching_aspects.append(aspect)
        
        if not matching_aspects:
            return method
        
        wrapped = method
        for aspect in reversed(matching_aspects):
            point = aspect['point']
            aspect_func = aspect['func']
            
            if point == "before":
                original = wrapped
                def before_wrapper(*args, **kwargs):
                    aspect_func(method, *args, **kwargs)
                    return original(*args, **kwargs)
                wrapped = before_wrapper
            elif point == "after":
                original = wrapped
                def after_wrapper(*args, **kwargs):
                    result = original(*args, **kwargs)
                    aspect_func(method, result, *args, **kwargs)
                    return result
                wrapped = after_wrapper
            elif point == "around":
                original = wrapped
                def around_wrapper(*args, **kwargs):
                    return aspect_func(original, *args, **kwargs)
                wrapped = around_wrapper
        
        return wrapped
    
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
                instance = cls()
            
            self._perform_setter_injection(instance, dependency_chain + [cls])
            
            for prop_name, prop_value in reg_info['properties'].items():
                if hasattr(instance, prop_name):
                    setattr(instance, prop_name, prop_value)
            
            for method_name in dir(instance):
                if method_name.startswith('_'):
                    continue
                method = getattr(instance, method_name)
                if callable(method):
                    wrapped = self._apply_aspects(instance, method_name, method)
                    setattr(instance, method_name, wrapped)
            
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
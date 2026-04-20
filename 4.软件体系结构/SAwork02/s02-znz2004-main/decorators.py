from functools import wraps

def component(layer=None, lazy=False):
    """标记类为构件组件，可指定所在层"""
    def decorator(cls):
        cls.__component__ = True
        if layer:
            cls.__layer__ = layer
        if lazy:
            cls.__lazy__ = True
        return cls
    return decorator

def command(name=None, description=""):
    """将方法注册为命令行命令"""
    def decorator(func):
        func.__command__ = True
        func.__command_name__ = name or func.__name__
        func.__command_description__ = description
        return func
    return decorator

def aspect(layer=None, point="around"):
    """声明切面，对指定层织入增强"""
    def decorator(func):
        func.__aspect__ = True
        func.__aspect_layer__ = layer
        func.__aspect_point__ = point
        return func
    return decorator

def lazy(cls):
    cls.__lazy__ = True
    return cls

def post_construct(func):
    func.__post_construct__ = True
    return func

def pre_destroy(func):
    func.__pre_destroy__ = True
    return func

def inject(func):
    @wraps(func)
    def wrapper(*args, **kwargs):
        return func(*args, **kwargs)
    wrapper.__inject__ = True
    return wrapper
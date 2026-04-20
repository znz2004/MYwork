from functools import wraps

# 构件注册装饰器
def component(cls):
    """标记类为构件组件"""
    cls.__component__ = True
    return cls

# 懒加载装饰器
def lazy(cls):
    """标记为懒加载构件"""
    cls.__lazy__ = True
    return cls

# 注入装饰器（用于 setter 注入）
def inject(func):
    """标记需要注入的 setter 方法"""
    @wraps(func)
    def wrapper(*args, **kwargs):
        return func(*args, **kwargs)
    wrapper.__inject__ = True
    return wrapper

# 生命周期回调装饰器
def post_construct(func):
    """标记初始化回调方法"""
    func.__post_construct__ = True
    return func

def pre_destroy(func):
    """标记销毁回调方法"""
    func.__pre_destroy__ = True
    return func
def component(cls=None):
    """标记类为构件组件"""
    def decorator(c):
        c.__component__ = True
        return c
    
    if cls is None:
        return decorator
    return decorator(cls)


def route(path: str, method: str = "GET"):
    """路由装饰器"""
    def decorator(func):
        func.__route__ = True
        func.__route_path__ = path
        func.__route_method__ = method.upper()
        return func
    return decorator


def post_construct(func):
    func.__post_construct__ = True
    return func


def pre_destroy(func):
    func.__pre_destroy__ = True
    return func
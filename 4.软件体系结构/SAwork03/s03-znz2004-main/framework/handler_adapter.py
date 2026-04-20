import inspect
from typing import Callable
from framework.request import Request
from framework.response import View, json_view


class HandlerAdapter:
    """控制器调度器"""
    
    def invoke(self, handler: Callable, request: Request) -> View:
        sig = inspect.signature(handler)
        kwargs = {}
        
        for param_name, param in sig.parameters.items():
            if param_name == 'request':
                kwargs[param_name] = request
            elif param_name in request.path_params():
                kwargs[param_name] = request.path_params()[param_name]
            elif param_name in request.query_params():
                kwargs[param_name] = request.query_params()[param_name]
        
        result = handler(**kwargs)
        
        if isinstance(result, View):
            return result
        elif isinstance(result, dict):
            return json_view(result)
        else:
            return View("template", data={"template": "", "model": {}})
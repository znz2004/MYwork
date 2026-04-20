import json
from typing import Dict, Tuple


class View:
    """视图：封装渲染结果"""
    
    _view_resolver = None  # 类级别的视图解析器
    
    @classmethod
    def set_view_resolver(cls, resolver):
        """设置全局视图解析器"""
        cls._view_resolver = resolver
    
    def __init__(self, view_type: str, data: dict = None, url: str = None):
        self.view_type = view_type
        self.data = data or {}
        self.url = url
    
    def render(self) -> Tuple[int, Dict[str, str], bytes]:
        if self.view_type == "redirect":
            return (302, {"Location": self.url}, b"")
        
        elif self.view_type == "json":
            body = json.dumps(self.data, ensure_ascii=False).encode('utf-8')
            return (200, {"Content-Type": "application/json; charset=utf-8"}, body)
        
        elif self.view_type == "template":
            if not View._view_resolver:
                return (500, {"Content-Type": "text/html"}, b"View resolver not configured")
            
            template_name = self.data.get('template')
            model = self.data.get('model', {})
            body = View._view_resolver.render(template_name, model)
            return (200, {"Content-Type": "text/html; charset=utf-8"}, body)
        
        return (500, {}, b"Unknown view type")


def view(template: str, model: dict) -> View:
    return View("template", data={"template": template, "model": model})


def redirect(url: str) -> View:
    return View("redirect", url=url)


def json_view(data) -> View:
    return View("json", data=data)
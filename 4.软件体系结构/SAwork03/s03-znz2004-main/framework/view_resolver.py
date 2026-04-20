import os
from jinja2 import Environment, FileSystemLoader, TemplateNotFound


class ViewResolver:
    """视图解析器"""
    
    def __init__(self, template_dir: str = "templates"):
        self.template_dir = template_dir
        self._setup_environment()
    
    def _setup_environment(self):
        if os.path.exists(self.template_dir):
            self.env = Environment(
                loader=FileSystemLoader(self.template_dir),
                autoescape=True
            )
        else:
            self.env = None
    
    def render(self, template_name: str, model: dict = None) -> bytes:
        if not self.env:
            return b"Template directory not found"
        
        try:
            template = self.env.get_template(template_name)
            html = template.render(model or {})
            return html.encode('utf-8')
        except TemplateNotFound:
            return f"Template not found: {template_name}".encode('utf-8')
        except Exception as e:
            return f"Template error: {e}".encode('utf-8')
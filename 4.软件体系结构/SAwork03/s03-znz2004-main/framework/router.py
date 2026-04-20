import re
from typing import Dict, List, Tuple, Optional, Callable


class Router:
    """路由表"""
    
    def __init__(self):
        self._routes: List[Dict] = []
    
    def register(self, path: str, method: str, handler: Callable):
        param_names = []
        pattern_str = path
        
        # 提取参数名
        def replacer(match):
            param_names.append(match.group(1))
            return r'([^/]+)'
        
        pattern_str = re.sub(r'<([a-zA-Z_][a-zA-Z0-9_]*)>', replacer, pattern_str)
        pattern = re.compile(f'^{pattern_str}$')
        
        # 判断是否是固定路径（不含参数）
        is_static = '<' not in path
        
        self._routes.append({
            'pattern': pattern,
            'method': method,
            'handler': handler,
            'param_names': param_names,
            'path': path,
            'is_static': is_static,
            'segments': len(path.split('/'))
        })
        
        # 排序：固定路径优先，然后按路径段数降序（更具体的路径优先）
        self._routes.sort(key=lambda x: (not x['is_static'], -x['segments']))
        
        print(f"[Router] 注册: {method} {path} (static={is_static}, segments={len(path.split('/'))})")
    
    def match(self, path: str, method: str) -> Optional[Tuple[Callable, dict]]:
        print(f"[Router] 匹配: {method} {path}")
        
        for route in self._routes:
            if route['method'] != method:
                continue
            
            match = route['pattern'].match(path)
            if match:
                path_params = {}
                for i, name in enumerate(route['param_names']):
                    path_params[name] = match.group(i + 1)
                print(f"[Router] 匹配成功: {method} {path} -> {route['handler'].__name__}")
                return (route['handler'], path_params)
        
        print(f"[Router] 匹配失败: {method} {path}")
        return None
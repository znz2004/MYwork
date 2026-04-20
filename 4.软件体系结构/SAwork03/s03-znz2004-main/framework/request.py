import urllib.parse
from typing import Dict


class Request:
    """HTTP 请求对象"""
    
    def __init__(self, raw_request: bytes):
        self._raw_request = raw_request
        self._method: str = ""
        self._path: str = ""
        self._version: str = ""
        self._headers: Dict[str, str] = {}
        self._query_params: Dict[str, str] = {}
        self._form_data: Dict[str, str] = {}
        self._body: bytes = b""
        self._path_params: Dict[str, str] = {}
        self._parse()
    
    def _parse(self):
        decoded = self._raw_request.decode('utf-8', errors='ignore')
        lines = decoded.split('\r\n')
        
        if not lines:
            return
        
        # 解析请求行
        request_line = lines[0].split(' ')
        if len(request_line) >= 2:
            self._method = request_line[0]
            full_path = request_line[1]
            self._version = request_line[2] if len(request_line) > 2 else "HTTP/1.1"
            
            if '?' in full_path:
                self._path, query_string = full_path.split('?', 1)
                self._query_params = dict(urllib.parse.parse_qsl(query_string))
            else:
                self._path = full_path
        
        # 解析请求头
        i = 1
        while i < len(lines) and lines[i]:
            if ': ' in lines[i]:
                key, value = lines[i].split(': ', 1)
                self._headers[key] = value
            i += 1
        
        # 解析请求体
        body_start = i + 1
        if body_start < len(lines):
            body_content = '\r\n'.join(lines[body_start:])
            self._body = body_content.encode('utf-8')
            
            content_type = self._headers.get('Content-Type', '')
            if content_type == 'application/x-www-form-urlencoded':
                self._form_data = dict(urllib.parse.parse_qsl(body_content))
    
    def path(self) -> str:
        return self._path
    
    def method(self) -> str:
        return self._method
    
    def query_params(self) -> dict:
        return self._query_params
    
    def path_params(self) -> dict:
        return self._path_params
    
    def set_path_params(self, params: dict):
        self._path_params = params
    
    def form(self) -> dict:
        return self._form_data
    
    def body(self) -> bytes:
        return self._body
    
    def headers(self) -> dict:
        return self._headers
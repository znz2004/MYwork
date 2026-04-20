import socket
import threading
import time
from typing import Dict
from framework.request import Request
from framework.response import view


class HttpServer:
    """HTTP 服务器"""
    
    def __init__(self, router, handler_adapter, view_resolver):
        # router, handler_adapter, view_resolver 没有类型注解
        # 这样容器会跳过依赖注入，需要我们手动设置
        self.router = router
        self.handler_adapter = handler_adapter
        self.view_resolver = view_resolver
        self._static_dir = "static"
    
    def start(self, host: str = "localhost", port: int = 8080):
        with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as s:
            s.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
            s.bind((host, port))
            s.listen(128)
            print(f"服务器启动: http://{host}:{port}")
            
            try:
                while True:
                    conn, addr = s.accept()
                    thread = threading.Thread(target=self._handle, args=(conn, addr))
                    thread.daemon = True
                    thread.start()
            except KeyboardInterrupt:
                print("\n服务器已停止")
    
    def _handle(self, conn, addr):
        start_time = time.time()
        
        try:
            data = conn.recv(8192)
            if not data:
                conn.close()
                return
            
            request = Request(data)
            
            if request.path().startswith('/static/'):
                response = self._serve_static(request.path())
                status = 200
                conn.sendall(response)
            else:
                match = self.router.match(request.path(), request.method())
                
                if match:
                    handler, path_params = match
                    request.set_path_params(path_params)
                    response_view = self.handler_adapter.invoke(handler, request)
                else:
                    response_view = view("error/404.html", {"path": request.path()})
                    response_view.set_view_resolver(self.view_resolver)
                
                status, headers, body = response_view.render()
                response = self._build_response(status, headers, body)
                conn.sendall(response)
            
            elapsed_ms = (time.time() - start_time) * 1000
            print(f"[HTTP] {request.method()} {request.path()} -> {status} ({elapsed_ms:.1f}ms)")
            
        except Exception as e:
            print(f"处理请求出错: {e}")
            response = self._build_response(500, {"Content-Type": "text/plain"}, b"Internal Server Error")
            conn.sendall(response)
        finally:
            conn.close()
    
    def _serve_static(self, path: str) -> bytes:
        import os
        import mimetypes
        
        safe_path = os.path.normpath(path.lstrip('/'))
        full_path = os.path.join(self._static_dir, safe_path.replace('static/', '', 1))
        
        if os.path.exists(full_path) and os.path.isfile(full_path):
            with open(full_path, 'rb') as f:
                body = f.read()
            content_type = mimetypes.guess_type(full_path)[0] or 'application/octet-stream'
            return self._build_response(200, {"Content-Type": content_type}, body)
        else:
            return self._build_response(404, {"Content-Type": "text/html"}, b"File not found")
    
    def _build_response(self, status: int, headers: Dict[str, str], body: bytes) -> bytes:
        status_text = {
            200: "OK", 302: "Found", 404: "Not Found", 500: "Internal Server Error"
        }.get(status, "OK")
        
        response = f"HTTP/1.1 {status} {status_text}\r\n"
        for key, value in headers.items():
            response += f"{key}: {value}\r\n"
        response += f"Content-Length: {len(body)}\r\n"
        response += "\r\n"
        
        return response.encode('utf-8') + body
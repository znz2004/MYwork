#!/usr/bin/env python3
import sys
import os
import signal

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))

from framework.container import Container
from framework.router import Router
from framework.handler_adapter import HandlerAdapter
from framework.view_resolver import ViewResolver
from framework.http_server import HttpServer
from framework.response import View
from bookstore.controllers import BookController


def signal_handler(sig, frame):
    """处理 Ctrl+C 信号"""
    print("\n\n正在关闭服务器...")
    sys.exit(0)


def register_routes(router: Router, controller: BookController):
    for method_name in dir(controller):
        if method_name.startswith('_'):
            continue
        
        method = getattr(controller, method_name)
        if callable(method) and hasattr(method, '__route__'):
            path = getattr(method, '__route_path__')
            route_method = getattr(method, '__route_method__')
            router.register(path, route_method, method)
            print(f"注册路由: {route_method} {path}")


def main():
    # 注册信号处理器
    signal.signal(signal.SIGINT, signal_handler)
    
    container = Container("config.yml")
    
    router = Router()
    handler_adapter = HandlerAdapter()
    view_resolver = ViewResolver(template_dir="templates")
    
    View.set_view_resolver(view_resolver)
    
    book_service = container.get(bookstore.services.BookService)
    book_controller = BookController(book_service)
    
    register_routes(router, book_controller)
    
    server = HttpServer(router, handler_adapter, view_resolver)
    
    print("\n" + "=" * 50)
    print("在线书店系统启动成功！")
    print("=" * 50)
    print("访问地址: http://localhost:8080/books")
    print("API 地址: http://localhost:8080/api/books")
    print("按 Ctrl+C 停止服务器")
    print("=" * 50 + "\n")
    
    try:
        server.start()
    except KeyboardInterrupt:
        print("\n服务器已停止")
    finally:
        container.close()


if __name__ == "__main__":
    import bookstore.services
    main()
[![Review Assignment Due Date](https://classroom.github.com/assets/deadline-readme-button-22041afd0340ce965d47ae6ef1cefeee28c7c493a6346c4f15d667ab976d596c.svg)](https://classroom.github.com/a/BwEEfzMV)
# 作业 03：Web 框架与 MVC 模式

在作业 01/02 实现的 **Python IoC 容器**基础上，构建一个支撑 MVC 模式的 Web 框架，并用它实现一个在线书店系统。

> 本作业的核心是**框架开发**——你要自己定义类似 Java Servlet 的规范，自己实现请求分发、控制器调度、视图解析等构件，最终组合出一个可用的 MVC Web 框架。

### 背景

在课件中我们看到，从 CGI 到 Servlet 到 Spring MVC，Web 框架的核心演进是：**将请求处理的通用逻辑（接收、解析、分发）与业务逻辑（做什么）分离**。

Servlet 规范正是这种分离的体现：
- **容器**（Tomcat/Jetty）负责 HTTP 通信、线程管理、生命周期
- **Servlet**（`HttpServlet`）负责处理特定 URL 的请求

```
HTTP 请求 → Tomcat（容器）→ DispatcherServlet → HandlerMapping → Controller → View
                      ↑                        ↑
              线程池、连接管理           路由分发、视图解析
```

你的任务是：**不引入任何 Web 框架（Flask/Django/FastAPI），从 socket 开始，自己定义这套规范，自己实现每一块构件**，最终用 IoC 容器将它们组装在一起。

---

## 一、定义框架规范

### 1.1 请求对象

`Request` 封装来自客户端的 HTTP 请求数据：

```python
class Request:
    def path(self) -> str:
        """返回 URL 路径，如 "/books/123" """

    def method(self) -> str:
        """返回 HTTP 方法，如 "GET"、"POST" """

    def query_params(self) -> dict:
        """返回 URL 查询参数，如 {"name": "python"} → /books?name=python """

    def path_params(self) -> dict:
        """返回路径参数，如 {"id": "123"} → /books/<id> """

    def form(self) -> dict:
        """返回 POST 表单数据（application/x-www-form-urlencoded）"""

    def body(self) -> bytes:
        """返回原始请求体"""

    def headers(self) -> dict:
        """返回 HTTP 请求头"""

```

### 1.2 响应对象与视图

响应由 **View** 对象表示，而非直接写入 OutputStream：

```python
class View:
    """视图：封装渲染结果"""
    def render(self) -> tuple[int, dict, bytes]:
        """返回 (status_code, headers, body)
        - status_code: 200, 302, 404, 500
        - headers: {"Content-Type": "text/html; charset=utf-8", ...}
        - body: 渲染后的字节内容
        """
```

框架提供三个构造器（内部持有对 `ViewResolver` 的引用，框架启动时由容器注入）：

```python
def view(template: str, model: dict) -> View:
    """渲染模板 + 数据，返回 HTML"""
    ...

def redirect(url: str) -> View:
    """返回 302 重定向响应"""
    ...

def json_view(data) -> View:
    """返回 JSON 响应（用于 REST API）"""
    ...
```

> `view()` 构造出的 `TemplateView` 在 `render()` 时会调用 `ViewResolver.resolve(template, model)` 完成渲染；`redirect()` 和 `json_view()` 不需要模板，直接在 `render()` 中生成响应内容。

### 1.3 控制器

控制器是一个普通的 Python 类，通过 `@route` 装饰器声明路由：

```python
@component
class BookController:
    def __init__(self, book_service: BookService):
        self.book_service = book_service

    @route("/books", method="GET")
    def list(self, request: Request) -> View:
        books = self.book_service.find_all()
        return view("book/list.html", {"books": books})

    @route("/books/<id>", method="GET")
    def show(self, request: Request, id: str) -> View:
        book = self.book_service.find_by_id(id)
        return view("book/show.html", {"book": book})

    @route("/books", method="POST")
    def create(self, request: Request) -> View:
        data = request.form()
        self.book_service.add(data)
        return redirect("/books")
```

`@route` 装饰器应支持：
- `path`：路由路径，支持路径参数（如 `/books/<id>`）
- `method`：HTTP 方法（`GET`、`POST`、`PUT`、`DELETE`），默认为 `GET`
- 路径参数通过方法签名自动提取（用 `inspect` 读取参数名）

### 1.4 HTTP 服务器

```python
@component
class HttpServer:
    def __init__(self, router: Router, handler_adapter: HandlerAdapter, view_resolver: ViewResolver):
        self.router = router
        self.handler_adapter = handler_adapter
        self.view_resolver = view_resolver

    def start(self, host: str = "localhost", port: int = 8080):
        with socket.socket() as s:
            s.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
            s.bind((host, port))
            s.listen(128)
            print(f"服务器启动: http://{host}:{port}")
            while True:
                conn, addr = s.accept()
                self._handle(conn)

    def _handle(self, conn):
        """解析请求、分发、返回响应"""
        request = self._parse_request(conn)
        handler = self.router.match(request.path(), request.method())
        if handler:
            response_view = self.handler_adapter.invoke(handler, request)
        else:
            response_view = view("error/404.html", {})
        status, headers, body = response_view.render()
        self._write_response(conn, status, headers, body)
```

要求：
- 用标准库 `socket` 实现，**不引入任何 HTTP 服务器库**（如 http.server）
- 每个请求在新线程中处理（用 `threading` 或 `concurrent.futures`）
- 正确解析 HTTP 请求行、请求头、请求体
- 返回标准 HTTP 响应

---

## 二、框架核心构件

### 2.1 路由表（Router）

扫描所有 Controller 实例上标有 `@route` 的方法，构建路由表。接收请求时，根据路径和 HTTP 方法找到对应的处理方法。

路由匹配需要支持路径参数：

```
路由: /books/<id>
GET /books/123  → 匹配，提取 id=123
GET /books      → 不匹配
GET /books/     → 不匹配
```

固定路径优先于参数路径（`/books/new` 优先于 `/books/<id>`）。

### 2.2 控制器调度器（HandlerAdapter）

接收路由匹配的处理方法和 `Request`，提取路径参数、查询参数、表单数据，按方法签名自动传入，调用后返回 `View`。调用方无需关心参数如何提取和类型转换。

### 2.3 视图解析器（ViewResolver）

接收模板名称和数据模型，渲染为 HTML 字节。使用 Jinja2 作为模板引擎（`pip install jinja2`）。

模板目录结构示例：

```
templates/
  base.html
  book/
    list.html
    show.html
    new.html
    edit.html
```

### 2.4 静态文件服务（必做）

处理 `/static/*` 路径的请求，直接从 `static/` 目录读取文件并返回，根据扩展名设置正确的 `Content-Type`（`.css`、`.js`、`.png` 等）。

---

## 三、用 IoC 容器组装框架

所有框架构件和业务层构件由同一个 IoC 容器统一管理。框架负责接收请求和分发，业务层负责数据和逻辑，两者通过容器解耦，互不直接依赖。

### 3.1 切面日志（AOP）

为框架增加切面支持，使 HTTP 请求日志能够以横切关注点的方式加入，而无需修改任何 Controller 代码。

日志应至少包含：HTTP 方法、请求路径、响应状态码、处理耗时。

预期效果：

```
[HTTP] GET /books -> 200  (1.2ms)
[HTTP] POST /books -> 302  (0.8ms)
[HTTP] GET /api/books -> 200  (0.3ms)
[HTTP] GET /books/abc123 -> 200  (0.5ms)
```

---

## 四、验证：在线书店

用你实现的框架开发一个在线书店系统。

### 4.1 业务功能

| 路由 | 方法 | 功能 |
|------|------|------|
| `/books` | GET | 书籍列表 |
| `/books/<id>` | GET | 书籍详情 |
| `/books/new` | GET | 新增书籍表单页 |
| `/books` | POST | 提交新增书籍 |
| `/books/<id>/delete` | POST | 删除书籍 |
| `/api/books` | GET | REST API：返回 JSON 书籍列表 |
| `/api/books/<id>` | GET | REST API：返回 JSON 单本书 |

### 4.2 数据模型

```python
class Book:
    def __init__(self, id: str, title: str, author: str, price: float):
        self.id = id
        self.title = title
        self.author = author
        self.price = price
```

使用内存存储或 JSON 文件持久化均可。

### 4.3 验收场景

**场景 1：书籍列表**
```
GET /books
→ 返回 HTML，渲染书籍列表（标题、作者、价格）
→ View: book/list.html，Model: {"books": [...]}
```

**场景 2：新增书籍**
```
GET /books/new
→ 返回 HTML，渲染表单页（标题、作者、价格输入框）

POST /books (form: title=Python&author=张三&price=59.9)
→ 调用 BookService.add()
→ 重定向到 /books
```

**场景 3：REST API**
```
GET /api/books
→ 返回 JSON: [{"id": "1", "title": "Python实战", "author": "张三", "price": 59.9}, ...]
→ Content-Type: application/json
```

**场景 4：路由不匹配时的 404**
```
GET /no-such-page
→ 路由表中无匹配项
→ HttpServer 直接返回 404 页面（error/404.html）
→ Controller 层无需关心
```

> 注意：`/books/nonexistent` 会匹配 `/books/<id>` 路由（id="nonexistent"），
> 此时是 `BookService.find_by_id()` 返回 `None`，由 Controller 决定如何处理（返回 404 视图或抛出异常），与框架层的 404 处理是两回事。


---

## 五、文档要求

在项目根目录的 `DESIGN.md` 中，用自己的话写清楚：

1. **框架设计**：你的框架由哪些构件组成，它们如何协作处理一次 HTTP 请求
2. **关键实现**：实现过程中遇到的核心技术问题及解决思路
3. **MVC 体会**：通过这次实现，你对 MVC 架构的理解是什么——Model、View、Controller 的职责边界在哪里，分离的价值是什么

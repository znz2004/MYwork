# DESIGN.md

## 运行方式
```bash
# 安装依赖
pip install jinja2 pyyaml

# 启动服务器
python main.py

# 访问
http://localhost:8080/books
```

## 框架设计
### 1.框架构件组成
框架由以下核心构件组成：
| 构件 | 文件 | 职责 |
|------|------|------|
| HttpServer | `framework/http_server.py` | 接收 HTTP 请求，解析协议，返回响应 |
| Request | `framework/request.py` | 封装 HTTP 请求数据（路径、方法、参数、表单） |
| View | `framework/response.py` | 封装响应结果（HTML/JSON/重定向） |
| Router | `framework/router.py` | 路由表，匹配 URL 到控制器方法 |
| HandlerAdapter | `framework/handler_adapter.py` | 参数提取，调用控制器方法 |
| ViewResolver | `framework/view_resolver.py` | 模板渲染（使用 Jinja2） |
| Container | `framework/container.py` | IoC 容器，管理组件依赖 |

### 2.一次 HTTP 请求的处理流程
```txt
浏览器请求 http://localhost:8080/books/1
         ↓
┌────────────────────────────────────────────────────────────────┐
│ 1. HttpServer 接收 socket 连接                                  │
│    - 使用 socket.accept() 接收请求                              │
│    - 解析 HTTP 请求行: "GET /books/1 HTTP/1.1"                  │
│    - 解析请求头: Host, Content-Type 等                          │
│    - 解析请求体: POST 时的表单数据                               │
│    - 创建 Request 对象                                          │
└────────────────────────────────────────────────────────────────┘
         ↓
┌────────────────────────────────────────────────────────────────┐
│ 2. Router 路由匹配                                              │
│    - 根据 path="/books/1" 和 method="GET" 查找处理器            │
│    - 正则匹配: /books/<id> → 提取 id="1"                        │
│    - 固定路径优先: /books/new 优先于 /books/<id>                │
└────────────────────────────────────────────────────────────────┘
         ↓
┌────────────────────────────────────────────────────────────────┐
│ 3. HandlerAdapter 调度                                          │
│    - 使用 inspect.signature() 读取方法参数                      │
│    - 自动注入: request 对象、路径参数、查询参数                  │
│    - 调用控制器方法: show(request, id="1")                      │
└────────────────────────────────────────────────────────────────┘
         ↓
┌────────────────────────────────────────────────────────────────┐
│ 4. Controller 业务逻辑                                          │
│    - 调用 BookService.find_by_id("1") 查询书籍                  │
│    - 返回 View 对象: view("book/show.html", {"book": book})     │
└────────────────────────────────────────────────────────────────┘
         ↓
┌────────────────────────────────────────────────────────────────┐
│ 5. ViewResolver 视图渲染                                        │
│    - Jinja2 加载模板: templates/book/show.html                  │
│    - 传入模型数据: {"book": {...}}                               │
│    - 渲染为 HTML 字符串                                          │
└────────────────────────────────────────────────────────────────┘
         ↓
┌────────────────────────────────────────────────────────────────┐
│ 6. HttpServer 返回响应                                          │
│    - 构造 HTTP 响应行: "HTTP/1.1 200 OK"                        │
│    - 构造响应头: Content-Type, Content-Length                   │
│    - 发送响应体: HTML 内容                                       │
│    - 记录日志: GET /books/1 -> 200 (1.2ms)                      │
└────────────────────────────────────────────────────────────────┘
         ↓
浏览器显示书籍详情页面
```

## 关键实现及技术问题
### 1.从零实现 HTTP 服务器
不使用任何现成的 HTTP 服务器库，需要从 socket 开始实现。
```txt
1.使用 Python 标准库 socket 监听端口
2.手动解析 HTTP 协议格式：
(1)请求行：GET /books HTTP/1.1
(2)请求头：Host: localhost:8080
(3)空行分隔
(4)请求体：POST 时的表单数据
3.每个请求使用独立线程处理（threading.Thread）
```
```python
with socket.socket() as s:
    s.bind((host, port))
    s.listen(128)
    while True:
        conn, addr = s.accept()
        thread = threading.Thread(target=self._handle, args=(conn, addr))
        thread.start()
```

### 2.路由匹配与参数提取
需要支持 /books/<id> 这样的动态路由，并提取参数；同时要区分固定路径（如 /books/new）和参数路径。
```txt
1.将路径模式转换为正则表达式：/books/<id> → /books/([^/]+)
2.使用正则匹配请求路径，提取参数值
3.排序规则：固定路径优先（is_static=True），然后按路径段数降序
```
```python
# 路径转换
pattern_str = re.sub(r'<([^>]+)>', r'([^/]+)', "/books/<id>")
# 结果: /books/([^/]+)

# 排序：固定路径优先
self._routes.sort(key=lambda x: (not x['is_static'], -x['segments']))
```

### 3.控制器参数自动注入
控制器方法的参数来自不同来源（Request 对象、路径参数、查询参数），需要自动识别并注入。
```python
# 使用 Python 的 inspect 模块分析函数签名
sig = inspect.signature(handler)
for param_name in sig.parameters:
    if param_name == 'request':
        kwargs['request'] = request
    elif param_name in request.path_params():
        kwargs[param_name] = request.path_params()[param_name]
    elif param_name in request.query_params():
        kwargs[param_name] = request.query_params()[param_name]
```

### 4.模板渲染
需要将 HTML 模板和数据模型结合，生成最终页面。
```python
# 使用 Jinja2 模板引擎
from jinja2 import Environment, FileSystemLoader

env = Environment(loader=FileSystemLoader("templates"))
template = env.get_template("book/list.html")
html = template.render({"books": books})
```

### 5.静态文件服务
需要提供 CSS、JS 等静态文件。
```python
# 处理 /static/* 路径，直接读取文件并设置正确的 Content-Type
if request.path().startswith('/static/'):
    file_path = request.path().replace('/static/', 'static/', 1)
    with open(file_path, 'rb') as f:
        body = f.read()
    content_type = mimetypes.guess_type(file_path)[0]
```

### 6.路由优先级问题
/books/new 被 /books/<id> 匹配，把 "new" 当作 id 参数。
```txt
1.在路由注册时标记是否为固定路径（is_static）
2.排序时固定路径优先
3.固定路径中路径段数多的优先（/books/new 有3段，/books 有2段）
```

### 7.IoC 容器依赖注入
组件之间存在依赖关系（如 Controller 依赖 Service），需要自动注入。
```txt
1.使用 inspect.signature() 读取构造函数参数
2.递归解析依赖，创建实例
3.支持 @component 装饰器标记组件
```
```python
def _resolve_dependencies(self, cls):
    sig = inspect.signature(cls.__init__)
    dependencies = {}
    for param_name, param in sig.parameters.items():
        if param_name != 'self':
            param_type = param.annotation
            dependencies[param_name] = self._get_or_create(param_type)
    return dependencies
```

## MVC 架构的理解
### 1.三层职责边界
| 层 | 职责 | 本框架中的体现 | 不应该做的事 |
|---|---|---|---|
| Model | 数据和业务逻辑 | Book 模型、BookService | 不应该知道 HTTP 请求/响应细节 |
| View | 数据展示 | HTML 模板、JSON 序列化 | 不应该包含业务逻辑 |
| Controller | 请求处理和响应 | BookController，使用 @route | 不应该直接操作数据库 |

### 2.分离的价值
```txt
1.关注点分离:每一层只关心自己的事情，互不干扰
2.可测试性:每一层可以独立测试
3.具有可维护性和可扩展性
```

### 3.我的体会
通过这个项目我理解了：
```txt
1.MVC提供了清晰的架构边界。当项目规模扩大时，这些边界能有效防止代码混乱。
2.框架的价值在于提取通用逻辑。HTTP 解析、路由匹配、参数注入这些逻辑应该由框架完成，开发者只需要关注业务。
3.控制反转让组件更灵活。Controller 不直接创建 Service，而是通过容器注入，这样替换实现时不需要修改 Controller 代码。
4.从 socket 到 MVC 的完整实现让我真正理解了 Web 框架的工作原理，而不仅仅是会用某个框架。
5.路由优先级处理是一个容易被忽视但很重要的问题，固定路径应该优先于参数路径，否则会导致意外匹配。
6.模板引擎的使用分离了页面展示和业务逻辑，前端和后端可以并行开发。
```

## 验收结果
| 功能 | 状态 |
|------|------|
| GET /books (书籍列表) | ok |
| GET /books/new (新增表单) | ok |
| POST /books (提交新增) | ok |
| GET /books/1 (书籍详情) | ok |
| POST /books/1/delete (删除) | ok |
| GET /api/books (API列表) | ok |
| GET /api/books/1 (API详情) | ok |
| 404 错误处理 | ok |
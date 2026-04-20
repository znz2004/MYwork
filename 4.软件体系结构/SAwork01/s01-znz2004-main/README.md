# IoC/DI 容器实现

## 项目概述
本项目实现了一个简易的 IoC (Inversion of Control) / DI (Dependency Injection) 容器，模拟 Spring IoC 容器的核心机制。通过该容器，可以实现构件的自动创建、依赖注入和生命周期管理。

## 项目结构
```txt
ioc_container_homework/
├── container.py # IoC 容器核心实现
├── decorators.py # 装饰器定义
├── exceptions.py # 异常类定义
├── order_system.py # 订单处理系统（验证演示）
├── test_container.py # 单元测试
├── run_tests.py # 测试运行脚本
├── beans-alipay.yml # 支付宝配置文件
├── beans-wechat.yml # 微信支付配置文件
├── beans-circular.yml # 循环依赖测试配置
└── README.md # 项目说明文档
```

## 运行方式
### 1. 安装依赖
```bash
pip install pyyaml
```

### 2. 运行订单系统演示
```bash
python order_system.py
```

### 3. 运行单元测试
```bash
python run_tests.py
```

## 功能实现
### 1. 构件定义与注册
两种注册方式：
#### 方式一：装饰器注册
```python
from decorators import component

@component
class MovieLister:
    def __init__(self, finder: MovieFinder):
        self.finder = finder
```
#### 方式二：配置文件注册
```yaml
# beans.yml
components:
  - class: moviefinder.ColonMovieFinder
    properties:
      filename: movies.txt
  - class: moviefinder.MovieLister
```

### 2. 依赖注入方式
#### 构造函数注入
```python
class MovieLister:
    def __init__(self, finder: MovieFinder):  # 类型标注 = 依赖声明
        self.finder = finder
```
#### Setter注入
```python
class MovieLister:
    def __init__(self):
        self.finder = None
    
    @inject
    def set_finder(self, finder: MovieFinder):
        self.finder = finder
```

### 3. 生命周期回调
```python
@component
class ColonMovieFinder:
    @post_construct
    def init(self):
        print("Loading movies...")
    
    @pre_destroy
    def cleanup(self):
        print("Releasing resources")
```

### 4. 懒加载
```python
@component
@lazy
class HeavyMovieFinder:
    def __init__(self):
        print("Loading huge database...")  # 首次使用时才创建
```

### 5. 循环依赖检测
当构件之间形成循环依赖时，容器会检测并报错：
CircularDependencyError: A -> B -> A

## IoC 容器的工作机理
### 1. 构件的创建
容器通过两种方式管理构件的创建：
装饰器注册：使用 @component 装饰器标记类，容器自动扫描并注册
配置文件注册：通过 YAML 配置文件定义构件，容器使用 importlib 动态导入类

### 2. 依赖解析
容器使用类型标注作为依赖声明：
使用 inspect.signature() 读取构造函数的参数类型
在注册表中查找匹配的类型（使用 issubclass() 判断）
递归创建依赖实例
通过构造函数参数注入

### 3. 注入方式
构造函数注入：必需依赖，通过 __init__ 参数声明
Setter 注入：可选依赖，通过 @inject 标记的方法注入

### 4. 生命周期管理
容器管理构件的完整生命周期：
构造 → 依赖注入 → @post_construct → 使用 → @pre_destroy
@post_construct：依赖注入完成后调用，用于初始化资源
@pre_destroy：容器关闭时调用，用于释放资源

### 5. 懒加载机制
通过 @lazy 装饰器标记的构件不会在容器启动时立即创建，而是在首次调用 get() 时才创建，适用于资源密集型的构件。

### 6. 循环依赖检测
容器维护一个 _creating 集合，记录正在创建的类。当检测到某个类已经在创建链中时，抛出 CircularDependencyError 异常，并输出完整的依赖链路。

## 基于容器的构件化应用软件的特点
### 1. 可替换性
订单处理系统中，OrderService 依赖 PaymentProcessor 接口。通过修改配置文件，将 AlipayProcessor 替换为 WechatPayProcessor，无需修改任何业务代码即可切换支付实现。
```python
# 配置 A：支付宝
container = Container("beans-alipay.yml")
# 配置 B：微信支付（只改配置文件）
container = Container("beans-wechat.yml")
```
这种设计使得构件之间的耦合降至最低，实现了开闭原则（对扩展开放，对修改关闭）。

### 2. 可测试性
由于依赖关系由容器管理，单元测试时可以轻松替换真实依赖为 Mock 对象：
```python
# 测试代码中注册 Mock 对象
container.register(MockPaymentProcessor)
container.register(MockNotificationService)
order_service = container.get(OrderService)
```
不需要修改 OrderService 的任何代码，极大提高了测试的便利性。

### 3. 松耦合
构件之间不再直接创建依赖，而是通过接口进行交互：
OrderService 只依赖 PaymentProcessor 接口，不关心具体实现
NotificationService 通过 Setter 注入 OrderLogger，将日志记录变为可选依赖
DBConnection 标记为懒加载，启动时不会因为数据库不可用而失败
这种设计使得每个构件都可以独立开发、测试和部署。

### 4. 配置化管理
通过配置文件管理构件装配，改变应用的行为只需要修改配置文件，符合控制反转原则——容器控制构件装配，而不是构件自己控制。

### 5. 资源管理自动化
生命周期回调机制确保资源的正确初始化和释放：
@post_construct：自动初始化数据库表
@pre_destroy：自动刷新缓冲区、关闭连接

## 验证结果
### 1. 订单系统演示场景
 ```txt
运行 python order_system.py，验证以下场景：
场景1：正常下单流程（支付宝）
[AlipayProcessor] 支付 59.9 元
[NotificationService] 订单已创建
[OrderLogger] 日志已记录
场景2：切换支付方式（微信支付）
[WechatPayProcessor] 支付 59.9 元  ← 实现已替换，代码零修改
[NotificationService] 订单已创建
场景3：懒加载验证
容器启动完毕
[DBConnection] 建立数据库连接  ← 延迟到使用时才创建
场景4：循环依赖检测
CircularDependencyError: OrderService -> ReportService -> OrderService
```

### 2. 单元测试结果
```bash
$ python run_tests.py
============================================================
运行单元测试
============================================================
test_basic_registration ... ok
test_circular_detection ... ok
test_constructor_injection ... ok
test_lazy_loading ... ok
test_post_construct ... ok
test_setter_injection ... ok
----------------------------------------------------------------------
Ran 6 tests in 0.015s
OK
============================================================
运行订单系统演示场景
============================================================
场景1：正常下单流程（支付宝） ✓
场景2：切换支付方式（微信支付） ✓
场景3：懒加载验证 ✓
场景4：循环依赖检测 ✓
============================================================
测试总结
============================================================
单元测试: ✓ 通过
演示场景: ✓ 通过
所有测试通过！容器实现正确。
```
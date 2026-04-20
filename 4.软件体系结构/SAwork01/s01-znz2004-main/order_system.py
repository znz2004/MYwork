from typing import Dict, Any, Optional
from decorators import component, inject, post_construct, pre_destroy, lazy
from container import Container


# ========== 接口定义 ==========
class PaymentProcessor:
    """支付处理器接口"""
    def pay(self, amount: float) -> str:
        raise NotImplementedError("子类必须实现 pay 方法")


# ========== 具体实现 ==========
@component
class AlipayProcessor(PaymentProcessor):
    """支付宝支付实现"""
    
    def pay(self, amount: float) -> str:
        return f"[AlipayProcessor] 支付 {amount} 元"


@component
class WechatPayProcessor(PaymentProcessor):
    """微信支付实现"""
    
    def pay(self, amount: float) -> str:
        return f"[WechatPayProcessor] 支付 {amount} 元"


@component
class NotificationService:
    """通知服务"""
    
    def __init__(self):
        self.logger = None
    
    @inject
    def set_logger(self, logger: 'OrderLogger'):
        """Setter 注入 OrderLogger"""
        self.logger = logger
    
    def send_notification(self, order_id: str):
        print(f"[NotificationService] 订单 {order_id} 已创建")
        if self.logger:
            self.logger.log(f"通知已发送: {order_id}")


@component
@lazy
class DBConnection:
    """数据库连接（懒加载）"""
    
    def __init__(self):
        print("[DBConnection] 建立数据库连接")
        self.connected = True
    
    def execute(self, sql: str):
        if self.connected:
            print(f"[DBConnection] 执行SQL: {sql}")
    
    @pre_destroy
    def close(self):
        if self.connected:
            print("[DBConnection] 关闭数据库连接")
            self.connected = False


@component
class OrderLogger:
    """订单日志记录器"""
    
    def __init__(self):
        self.db = None
        self.buffer = []
    
    @inject
    def set_db_connection(self, db: DBConnection):
        """Setter 注入数据库连接"""
        self.db = db
    
    @post_construct
    def init_table(self):
        """初始化日志表"""
        print("[OrderLogger] 初始化日志表")
        if self.db:
            self.db.execute("CREATE TABLE IF NOT EXISTS order_logs (id INT, message TEXT)")
    
    def log(self, message: str):
        self.buffer.append(message)
        print(f"[OrderLogger] 日志已记录: {message}")
        if self.db:
            self.db.execute(f"INSERT INTO order_logs VALUES (1, '{message}')")
    
    @pre_destroy
    def flush_buffer(self):
        """刷新缓冲区并关闭连接"""
        print(f"[OrderLogger] 刷新缓冲区: {self.buffer}")
        self.buffer.clear()


@component
class OrderService:
    """订单服务（核心业务）"""
    
    def __init__(self, payment_processor: PaymentProcessor, notification_service: NotificationService):
        """构造函数注入支付处理器和通知服务"""
        self.payment_processor = payment_processor
        self.notification_service = notification_service
        self.logger = None
    
    @inject
    def set_logger(self, logger: OrderLogger):
        """Setter 注入日志记录器（可选依赖）"""
        self.logger = logger
    
    def place_order(self, order_data: Dict[str, Any]) -> str:
        """处理订单"""
        order_id = f"ORD-{abs(hash(str(order_data)))}"
        amount = order_data.get('price', 0)
        
        # 处理支付
        payment_result = self.payment_processor.pay(amount)
        print(payment_result)
        
        # 发送通知
        self.notification_service.send_notification(order_id)
        
        # 记录日志
        if self.logger:
            self.logger.log(f"订单 {order_id} 已下单，金额: {amount}")
        
        return order_id


@component
class ReportService:
    """报表服务（用于循环依赖测试）"""
    
    def __init__(self, order_service: OrderService):
        self.order_service = order_service


# ========== 验证场景 ==========
def scenario1_alipay():
    """场景1：正常下单流程（支付宝）"""
    print("\n" + "="*50)
    print("场景1：正常下单流程（支付宝）")
    print("="*50)
    
    # 不使用配置文件，直接注册
    container = Container()
    container.register(DBConnection, lazy_load=True)
    container.register(OrderLogger)
    container.register(NotificationService)
    container.register(AlipayProcessor)
    container.register(OrderService)
    
    order_service = container.get(OrderService)
    result = order_service.place_order({"item": "Python编程", "price": 59.9})
    print(f"订单结果: {result}")
    container.close()


def scenario2_wechat():
    """场景2：切换支付方式（微信支付）"""
    print("\n" + "="*50)
    print("场景2：切换支付方式（微信支付）")
    print("="*50)
    
    # 只改变注册的支付实现
    container = Container()
    container.register(DBConnection, lazy_load=True)
    container.register(OrderLogger)
    container.register(NotificationService)
    container.register(WechatPayProcessor)  # 只改这一行
    container.register(OrderService)
    
    order_service = container.get(OrderService)
    result = order_service.place_order({"item": "Python编程", "price": 59.9})
    print(f"订单结果: {result}")
    container.close()


def scenario3_lazy():
    """场景3：懒加载验证"""
    print("\n" + "="*50)
    print("场景3：懒加载验证")
    print("="*50)
    
    container = Container()
    container.register(DBConnection, lazy_load=True)  # 懒加载
    container.register(OrderLogger)
    container.register(NotificationService)
    container.register(AlipayProcessor)
    container.register(OrderService)
    
    print("容器启动完毕 - DBConnection 尚未创建（懒加载）")
    print("首次调用 place_order，将触发 DBConnection 创建...")
    
    order_service = container.get(OrderService)
    result = order_service.place_order({"item": "Python编程", "price": 59.9})
    print(f"订单结果: {result}")
    container.close()


def scenario4_circular():
    """场景4：循环依赖报错"""
    print("\n" + "="*50)
    print("场景4：循环依赖检测")
    print("="*50)
    
    try:
        # 创建循环依赖的配置
        container = Container()
        
        @component
        class ServiceA:
            def __init__(self, b: 'ServiceB'):
                self.b = b
        
        @component
        class ServiceB:
            def __init__(self, a: ServiceA):
                self.a = a
        
        container.register(ServiceA)
        container.register(ServiceB)
        
        container.get(ServiceA)
        
    except Exception as e:
        print(f"捕获异常: {e}")
        print("✓ 循环依赖检测成功")


if __name__ == "__main__":
    # 运行所有验证场景
    scenario1_alipay()
    scenario2_wechat()
    scenario3_lazy()
    scenario4_circular()
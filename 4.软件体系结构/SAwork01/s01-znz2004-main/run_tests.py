#!/usr/bin/env python3
# -*- coding: utf-8 -*-

"""
统一测试运行脚本
"""

import sys
import unittest

def run_all_tests():
    """运行所有单元测试"""
    print("=" * 60)
    print("运行单元测试")
    print("=" * 60)
    
    # 加载所有测试
    loader = unittest.TestLoader()
    suite = loader.discover('.', pattern='test_*.py')
    
    # 运行测试
    runner = unittest.TextTestRunner(verbosity=2)
    result = runner.run(suite)
    
    return result.wasSuccessful()

def run_demo_scenarios():
    """运行订单系统演示场景"""
    print("\n" + "=" * 60)
    print("运行订单系统演示场景")
    print("=" * 60)
    
    from order_system import (
        scenario1_alipay, 
        scenario2_wechat, 
        scenario3_lazy, 
        scenario4_circular
    )
    
    try:
        scenario1_alipay()
        scenario2_wechat()
        scenario3_lazy()
        scenario4_circular()
        return True
    except Exception as e:
        print(f"演示场景运行失败: {e}")
        import traceback
        traceback.print_exc()
        return False

def interactive_test():
    """交互式测试模式"""
    print("\n" + "=" * 60)
    print("交互式测试模式")
    print("=" * 60)
    print("请选择要测试的功能：")
    print("1. 测试基本注册和获取")
    print("2. 测试构造函数注入")
    print("3. 测试 Setter 注入")
    print("4. 测试生命周期回调")
    print("5. 测试懒加载")
    print("6. 测试循环依赖检测")
    print("7. 测试配置文件加载")
    print("8. 测试支付切换（支付宝 vs 微信）")
    print("9. 运行所有测试")
    print("0. 退出")
    
    while True:
        choice = input("\n请输入选择 (0-9): ").strip()
        
        if choice == '0':
            break
        elif choice == '1':
            test_basic_registration()
        elif choice == '2':
            test_constructor_injection()
        elif choice == '3':
            test_setter_injection()
        elif choice == '4':
            test_lifecycle()
        elif choice == '5':
            test_lazy_loading()
        elif choice == '6':
            test_circular_detection()
        elif choice == '7':
            test_config_loading()
        elif choice == '8':
            test_payment_switch()
        elif choice == '9':
            run_all_tests()
            run_demo_scenarios()
        else:
            print("无效选择，请重新输入")

def test_basic_registration():
    """测试基本注册和获取"""
    print("\n--- 测试基本注册和获取 ---")
    from container import Container
    from decorators import component
    
    @component
    class TestClass:
        pass
    
    container = Container()
    container.register(TestClass)
    instance = container.get(TestClass)
    
    print(f"✓ 成功创建实例: {instance}")
    print(f"✓ 实例类型正确: {isinstance(instance, TestClass)}")
    assert isinstance(instance, TestClass)
    print("✓ 测试通过")

def test_constructor_injection():
    """测试构造函数注入"""
    print("\n--- 测试构造函数注入 ---")
    from container import Container
    from decorators import component
    
    @component
    class Dependency:
        def __init__(self):
            self.value = "injected"
    
    @component
    class MainClass:
        def __init__(self, dep: Dependency):
            self.dep = dep
    
    container = Container()
    container.register(Dependency)
    container.register(MainClass)
    
    main = container.get(MainClass)
    print(f"✓ MainClass 实例: {main}")
    print(f"✓ 依赖已注入: {main.dep}")
    print(f"✓ 依赖值正确: {main.dep.value == 'injected'}")
    assert main.dep.value == "injected"
    print("✓ 测试通过")

def test_setter_injection():
    """测试 Setter 注入"""
    print("\n--- 测试 Setter 注入 ---")
    from container import Container
    from decorators import component, inject
    
    @component
    class Logger:
        def log(self, msg):
            print(f"Log: {msg}")
    
    @component
    class Service:
        def __init__(self):
            self.logger = None
        
        @inject
        def set_logger(self, logger: Logger):
            self.logger = logger
    
    container = Container()
    container.register(Logger)
    container.register(Service)
    
    service = container.get(Service)
    print(f"✓ Service 实例: {service}")
    print(f"✓ Setter 注入成功: {service.logger is not None}")
    assert service.logger is not None
    print("✓ 测试通过")

def test_lifecycle():
    """测试生命周期回调"""
    print("\n--- 测试生命周期回调 ---")
    from container import Container
    from decorators import component, post_construct, pre_destroy
    
    @component
    class LifecycleComponent:
        def __init__(self):
            print("  1. 构造函数调用")
            self.init_called = False
            self.destroy_called = False
        
        @post_construct
        def init(self):
            print("  2. @post_construct 调用")
            self.init_called = True
        
        @pre_destroy
        def destroy(self):
            print("  3. @pre_destroy 调用")
            self.destroy_called = True
    
    container = Container()
    container.register(LifecycleComponent)
    
    print("创建实例...")
    instance = container.get(LifecycleComponent)
    
    print("\n验证回调状态...")
    print(f"  init_called: {instance.init_called}")
    print(f"  destroy_called: {instance.destroy_called}")
    assert instance.init_called == True
    assert instance.destroy_called == False
    
    print("\n关闭容器...")
    container.close()
    print(f"  destroy_called: {instance.destroy_called}")
    assert instance.destroy_called == True
    
    print("✓ 测试通过")

def test_lazy_loading():
    """测试懒加载"""
    print("\n--- 测试懒加载 ---")
    from container import Container
    from decorators import component, lazy
    
    creation_count = 0
    
    @component
    @lazy
    class LazyComponent:
        def __init__(self):
            global creation_count
            creation_count += 1
            print(f"  LazyComponent 创建 (第 {creation_count} 次)")
    
    container = Container()
    container.register(LazyComponent)
    
    print("容器启动后，检查实例是否创建...")
    assert LazyComponent not in container._instances
    print("✓ 懒加载组件尚未创建")
    
    print("\n首次获取实例...")
    instance1 = container.get(LazyComponent)
    print(f"✓ 实例已创建: {instance1}")
    assert LazyComponent in container._instances
    
    print("\n再次获取实例...")
    instance2 = container.get(LazyComponent)
    print(f"✓ 返回同一个实例: {instance1 is instance2}")
    assert instance1 is instance2
    assert creation_count == 1
    
    print("✓ 测试通过")

def test_circular_detection():
    """测试循环依赖检测"""
    print("\n--- 测试循环依赖检测 ---")
    from container import Container
    from decorators import component
    from exceptions import CircularDependencyError
    
    @component
    class A:
        def __init__(self, b: 'B'):
            self.b = b
    
    @component
    class B:
        def __init__(self, a: A):
            self.a = a
    
    container = Container()
    container.register(A)
    container.register(B)
    
    try:
        container.get(A)
        print("✗ 应该抛出循环依赖异常但没有")
        assert False, "应该抛出异常"
    except CircularDependencyError as e:
        print(f"✓ 正确捕获异常: {e}")
        print("✓ 测试通过")

def test_config_loading():
    """测试配置文件加载"""
    print("\n--- 测试配置文件加载 ---")
    from container import Container
    
    # 创建临时配置文件
    import yaml
    import tempfile
    import os
    
    config_data = {
        'components': [
            {
                'class': 'order_system.AlipayProcessor',
                'properties': {}
            }
        ]
    }
    
    with tempfile.NamedTemporaryFile(mode='w', suffix='.yml', delete=False) as f:
        yaml.dump(config_data, f)
        temp_file = f.name
    
    try:
        container = Container(temp_file)
        from order_system import AlipayProcessor
        instance = container.get(AlipayProcessor)
        print(f"✓ 从配置文件成功加载类: {instance}")
        assert isinstance(instance, AlipayProcessor)
        print("✓ 测试通过")
    finally:
        os.unlink(temp_file)

def test_payment_switch():
    """测试支付切换"""
    print("\n--- 测试支付切换 ---")
    from container import Container
    from order_system import OrderService, AlipayProcessor, WechatPayProcessor
    
    # 测试支付宝
    print("\n1. 使用支付宝:")
    container1 = Container("beans-alipay.yml")
    order_service1 = container1.get(OrderService)
    payment1 = order_service1.payment_processor
    print(f"   支付实现: {payment1.__class__.__name__}")
    assert isinstance(payment1, AlipayProcessor)
    
    # 测试微信支付
    print("\n2. 使用微信支付:")
    container2 = Container("beans-wechat.yml")
    order_service2 = container2.get(OrderService)
    payment2 = order_service2.payment_processor
    print(f"   支付实现: {payment2.__class__.__name__}")
    assert isinstance(payment2, WechatPayProcessor)
    
    print("\n✓ 测试通过：通过配置文件成功切换支付实现")
    print("   OrderService 代码零修改！")

if __name__ == "__main__":
    print("\n" + "=" * 60)
    print("IoC/DI 容器测试套件")
    print("=" * 60)
    
    # 运行单元测试
    unit_test_success = run_all_tests()
    
    # 运行演示场景
    demo_success = run_demo_scenarios()
    
    # 进入交互模式（可选）
    print("\n是否进入交互式测试模式？(y/n): ", end="")
    if input().strip().lower() == 'y':
        interactive_test()
    
    # 总结
    print("\n" + "=" * 60)
    print("测试总结")
    print("=" * 60)
    print(f"单元测试: {'✓ 通过' if unit_test_success else '✗ 失败'}")
    print(f"演示场景: {'✓ 通过' if demo_success else '✗ 失败'}")
    
    if unit_test_success and demo_success:
        print("\n所有测试通过！容器实现正确。")
    else:
        print("\n部分测试失败，请检查实现。")
        sys.exit(1)
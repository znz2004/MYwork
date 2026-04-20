from decorators import aspect


@aspect(layer="business", point="around")
def transaction_log(method, *args, **kwargs):
    """记录业务层事务日志"""
    print(f"[TX] 开始: {method.__qualname__}")
    try:
        result = method(*args, **kwargs)
        print(f"[TX] 完成: {method.__qualname__}")
        return result
    except Exception as e:
        print(f"[TX] 失败: {method.__qualname__} - {e}")
        raise


@aspect(layer="data", point="after")
def audit_log(method, result, *args, **kwargs):
    """记录数据层操作审计"""
    print(f"[AUDIT] {method.__qualname__} -> OK")
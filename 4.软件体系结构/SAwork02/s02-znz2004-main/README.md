# 分层架构命令行框架与 POS 系统

## 项目概述
本项目在上一章实现的 IoC 容器基础上，扩展了分层架构约束和命令行交互能力，实现了一个完整的 POS（收银机）系统。框架的核心是提供分层架构的基础设施，类似 Spring Shell 在 Spring 生态中的角色。

## 项目结构
```txt
S02/
├── container.py # IoC 容器核心实现（支持分层）
├── decorators.py # 装饰器定义（component, command, aspect等）
├── exceptions.py # 异常类定义
├── aspects.py # AOP 切面实现
├── command_shell.py # 命令路由与交互式 Shell
├── pos_system.py # POS 系统业务实现
├── config-json.yml # JSON 配置文件
├── data/ # 数据层实现
│ ├── init.py
│ ├── product_repo.py # 商品仓储接口
│ ├── transaction_repo.py # 交易仓储接口
│ └── json_repo.py # JSON 文件实现
├── result/ # 数据存储目录（运行时自动生成）
│ ├── products.json # 商品数据
│ └── transactions.json # 交易记录
└── README.md # 项目说明文档
```

## 运行方式
### 1. 安装依赖
```bash
pip install pyyaml
```

### 2. 运行 POS 系统
```bash
python pos_system.py config-json.yml
```

### 3. 可用命令
| 命令 | 说明 | 示例 |
|------|------|------|
| `scan <barcode>` | 扫码添加商品 | `scan 6901234567890` |
| `remove <barcode>` | 删除商品 | `remove 6901234567890` |
| `list` | 查看购物车 | `list` |
| `pay <method>` | 结账 (cash/card) | `pay cash` |
| `receipt` | 打印小票 | `receipt` |
| `refund <txn_id>` | 退款 | `refund txn_20260408_154700` |
| `report <daily\|monthly>` | 销售报表 | `report daily` |
| `help` | 显示帮助 | `help` |
| `quit` | 退出 | `quit` |

## 框架设计说明
本框架通过在容器层面实现层间依赖检查，确保分层架构的约束得到强制执行：
```txt
1.层声明：通过 @component(layer="xxx") 声明组件所属层
2.层序配置：通过 layers 列表定义层的拓扑顺序
3.依赖检查：容器在解析依赖时，检查源层和目标层的关系：
(1)同层允许互相依赖
(2)strict 模式：只允许依赖相邻下层
(3)relaxed 模式：允许依赖任意下层
(4)禁止下层依赖上层
```
```python
# 容器启动时自动检查
container = Container(
    layers=["presentation", "business", "data"],
    mode="strict"
)
```

## 从构件容器到分层框架
相比上一章的 IoC 容器，分层架构本质上在依赖注入的基础上增加了：
| 维度 | IoC 容器 | 分层框架 |
|:----:|:--------:|:--------:|
| 依赖管理 | 自动注入 | 自动注入 + 拓扑约束 |
| 架构约束 | 无 | 强制层间依赖规则 |
| 可维护性 | 基础 | 防止架构腐化 |
| 可视化 | 无 | 层定义使结构清晰 |

## 跨层关注点的处理
AOP 切面机制用于处理横切关注点（Cross-Cutting Concerns）
### 1.三种切入点类型
| 类型 | 说明 | 适用场景 |
|------|------|----------|
| before | 方法执行前织入 | 权限检查、参数验证 |
| after | 方法执行后织入 | 审计日志、结果处理 |
| around | 完全环绕方法执行 | 事务管理、性能监控 |

### 2.切面按层生效
```python
@aspect(layer="business", point="around")
def transaction_log(method, *args, **kwargs):
    """业务层所有方法自动织入事务日志"""
    print(f"[TX] 开始: {method.__qualname__}")
    result = method(*args, **kwargs)
    print(f"[TX] 完成: {method.__qualname__}")
    return result
```

### 3.典型跨层关注点
```txt
事务管理：业务层所有方法需要事务控制
审计日志：数据层所有操作需要记录
性能监控：跨所有层的方法执行时间
安全控制：表现层的权限验证
```

## 验证结果
### 1.完整购物流程演示
```text
POS> scan 6901234567890
已添加: 可口可乐 ¥3.5 (库存: 99)
POS> scan 6901234567891
已添加: 农夫山泉 ¥2.0 (库存: 49)
POS> list
购物车:
  1. 可口可乐 x1   ¥3.50
  2. 农夫山泉 x1   ¥2.00
  合计: ¥5.50
POS> pay cash
[TX] 开始: PaymentService.process_payment
[TX] 完成: PaymentService.process_payment
交易完成！交易号: txn_20260408_154700
POS> receipt
════════════════════════════════════════
       收 银 小 票
────────────────────────────────────────
可口可乐 x1          ¥3.50
农夫山泉 x1          ¥2.00
────────────────────────────────────────
合计:                  ¥5.50
支付方式:              cash
交易号:                txn_20260408_154700
════════════════════════════════════════
```

### 2.层约束违反检测
如果违反分层规则，容器会在启动时报错：
```python
@component(layer="presentation")
class BadCommand:
    def __init__(self, repo: ProductRepository):  # 跨层！
        pass
```
错误信息：
```txt
LayerViolationError: BadCommand (presentation) -> ProductRepository (data)
  presentation can only depend on: ['business']
```

### 3.切面生效验证
```txt
POS> pay card
[TX] 开始: PaymentService.process_payment
[TX] 开始: CartService.clear
[TX] 完成: CartService.clear
[TX] 完成: PaymentService.process_payment
[AUDIT] JsonTransactionRepository.save -> OK
```

### 4.存储层可替换
通过修改配置文件，可以在不同数据存储实现之间切换：
```yaml
# JSON 文件实现
components:
  - class: data.json_repo.JsonProductRepository
    layer: data

# SQLite 实现（只需改配置）
components:
  - class: data.sqlite_repo.SqliteProductRepository
    layer: data
```
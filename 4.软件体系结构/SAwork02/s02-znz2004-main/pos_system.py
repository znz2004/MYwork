import json
import os
from datetime import datetime
from typing import Dict, List, Optional
from decorators import component, command, inject, post_construct, pre_destroy, lazy
from container import Container
from data.json_repo import JsonProductRepository, JsonTransactionRepository


# ========== Business Layer ==========

@component(layer="business")
class CartService:
    """购物车服务"""
    
    def __init__(self, product_repo: JsonProductRepository):
        self.product_repo = product_repo
        self.cart: Dict[str, Dict] = {}
    
    def add_item(self, barcode: str) -> Dict:
        product = self.product_repo.find_by_barcode(barcode)
        if not product:
            raise ValueError(f"未找到商品: {barcode}")
        
        if product['stock'] <= 0:
            raise ValueError(f"商品 {product['name']} 库存不足")
        
        if barcode in self.cart:
            self.cart[barcode]['quantity'] += 1
        else:
            self.cart[barcode] = {
                'barcode': barcode,
                'name': product['name'],
                'price': product['price'],
                'quantity': 1
            }
        
        self.product_repo.update_stock(barcode, -1)
        
        return {
            'name': product['name'],
            'price': product['price'],
            'stock': product['stock'] - 1
        }
    
    def remove_item(self, barcode: str) -> Dict:
        if barcode not in self.cart:
            raise ValueError(f"购物车中没有该商品: {barcode}")
        
        item = self.cart[barcode]
        item['quantity'] -= 1
        
        if item['quantity'] <= 0:
            del self.cart[barcode]
        
        self.product_repo.update_stock(barcode, 1)
        
        return item
    
    def get_items(self) -> List[Dict]:
        return list(self.cart.values())
    
    def get_total(self) -> float:
        return sum(item['price'] * item['quantity'] for item in self.cart.values())
    
    def clear(self):
        for barcode, item in self.cart.items():
            self.product_repo.update_stock(barcode, item['quantity'])
        self.cart.clear()
    
    def apply_discount(self, total: float) -> float:
        if total >= 100:
            return total * 0.9
        elif total >= 50:
            return total - 5
        return total


@component(layer="business")
class PaymentService:
    """支付服务"""
    
    def __init__(self, transaction_repo: JsonTransactionRepository):
        self.transaction_repo = transaction_repo
        self.last_transaction = None
    
    def process_payment(self, method: str, total: float, items: List[Dict]) -> Dict:
        transaction = {
            'method': method,
            'total': total,
            'items': items,
            'status': 'completed'
        }
        
        txn_id = self.transaction_repo.save(transaction)
        transaction['id'] = txn_id
        self.last_transaction = transaction
        
        return transaction
    
    def get_last_transaction(self) -> Optional[Dict]:
        return self.last_transaction


@component(layer="business")
class ReceiptService:
    """小票服务"""
    
    def format_receipt(self, transaction: Dict) -> str:
        lines = []
        lines.append("═" * 40)
        lines.append("       收 银 小 票")
        lines.append("─" * 40)
        
        for item in transaction['items']:
            lines.append(f"{item['name']} x{item['quantity']:<10} ¥{item['price'] * item['quantity']:.2f}")
        
        lines.append("─" * 40)
        lines.append(f"合计: {'':>16} ¥{transaction['total']:.2f}")
        lines.append(f"支付方式: {'':>12} {transaction['method']}")
        lines.append(f"交易号: {'':>14} {transaction['id']}")
        lines.append("═" * 40)
        
        return '\n'.join(lines)


@component(layer="business")
@lazy
class ReportService:
    """报表服务"""
    
    def __init__(self, transaction_repo: JsonTransactionRepository):
        self.transaction_repo = transaction_repo
        print("[ReportService] 初始化报表服务")
    
    def generate_daily_report(self, date: str = None) -> str:
        if date is None:
            date = datetime.now().strftime('%Y-%m-%d')
        
        report = self.transaction_repo.get_daily_report(date)
        
        lines = []
        lines.append("=" * 50)
        lines.append(f"日报表 - {report['date']}")
        lines.append("-" * 50)
        lines.append(f"总交易额: ¥{report['total_amount']:.2f}")
        lines.append(f"交易笔数: {report['total_transactions']}")
        lines.append("\n商品销售排行:")
        
        sorted_products = sorted(
            report['product_sales'].items(),
            key=lambda x: x[1]['quantity'],
            reverse=True
        )
        
        for i, (barcode, data) in enumerate(sorted_products[:10], 1):
            lines.append(f"  {i}. {data['name']} x{data['quantity']}  ¥{data['amount']:.2f}")
        
        lines.append("=" * 50)
        return '\n'.join(lines)
    
    def generate_monthly_report(self, year_month: str = None) -> str:
        if year_month is None:
            year_month = datetime.now().strftime('%Y-%m')
        
        report = self.transaction_repo.get_monthly_report(year_month)
        
        lines = []
        lines.append("=" * 50)
        lines.append(f"月报表 - {report['year_month']}")
        lines.append("-" * 50)
        lines.append(f"总交易额: ¥{report['total_amount']:.2f}")
        lines.append(f"交易笔数: {report['total_transactions']}")
        lines.append("\n每日统计:")
        
        for date, stats in sorted(report['daily_stats'].items()):
            lines.append(f"  {date}: ¥{stats['amount']:.2f} ({stats['count']}笔)")
        
        lines.append("=" * 50)
        return '\n'.join(lines)


@component(layer="business")
class RefundService:
    """退款服务"""
    
    def __init__(self, transaction_repo: JsonTransactionRepository, 
                 product_repo: JsonProductRepository):
        self.transaction_repo = transaction_repo
        self.product_repo = product_repo
        self.refund_config = {}
    
    @post_construct
    def init_config(self):
        self.refund_config = {
            'max_days': 7,
            'fee_rate': 0,
        }
        print("[RefundService] 加载退款策略配置完成")
    
    def refund(self, txn_id: str) -> Dict:
        transaction = self.transaction_repo.find_by_id(txn_id)
        if not transaction:
            raise ValueError(f"未找到交易: {txn_id}")
        
        txn_date = datetime.fromisoformat(transaction['timestamp'])
        days_diff = (datetime.now() - txn_date).days
        
        if days_diff > self.refund_config['max_days']:
            raise ValueError(f"交易已超过{self.refund_config['max_days']}天，无法退款")
        
        for item in transaction['items']:
            self.product_repo.update_stock(item['barcode'], item['quantity'])
        
        refund_transaction = {
            'original_txn': txn_id,
            'amount': transaction['total'],
            'method': transaction['method'],
            'status': 'refunded',
            'refund_time': datetime.now().isoformat()
        }
        
        refund_id = self.transaction_repo.save(refund_transaction)
        
        return {
            'refund_id': refund_id,
            'original_txn': txn_id,
            'amount': transaction['total']
        }


# ========== Presentation Layer ==========

@component(layer="presentation")
class POSCommands:
    """POS 系统命令"""
    
    def __init__(self, cart_service: CartService, payment_service: PaymentService,
                 receipt_service: ReceiptService, refund_service: RefundService,
                 report_service: ReportService):
        self.cart_service = cart_service
        self.payment_service = payment_service
        self.receipt_service = receipt_service
        self.refund_service = refund_service
        self.report_service = report_service
    
    @command("scan", description="扫码添加商品")
    def scan(self, barcode: str):
        item = self.cart_service.add_item(barcode)
        return f"已添加: {item['name']} ¥{item['price']} (库存: {item['stock']})"
    
    @command("remove", description="删除商品")
    def remove(self, barcode: str):
        item = self.cart_service.remove_item(barcode)
        return f"已移除: {item['name']}"
    
    @command("list", description="查看购物车")
    def list_cart(self):
        items = self.cart_service.get_items()
        if not items:
            return "购物车为空"
        
        lines = ["购物车:", "-" * 30]
        for i, item in enumerate(items, 1):
            lines.append(f"  {i}. {item['name']} x{item['quantity']:<3} ¥{item['price'] * item['quantity']:.2f}")
        lines.append("-" * 30)
        lines.append(f"  合计: ¥{self.cart_service.get_total():.2f}")
        return '\n'.join(lines)
    
    @command("pay", description="结账 (cash/card)")
    def pay(self, method: str):
        total = self.cart_service.get_total()
        if total == 0:
            return "购物车为空，无法结账"
        
        discounted_total = self.cart_service.apply_discount(total)
        if discounted_total < total:
            print(f"优惠后: ¥{discounted_total:.2f} (原价: ¥{total:.2f})")
        
        items = self.cart_service.get_items()
        transaction = self.payment_service.process_payment(method, discounted_total, items)
        self.cart_service.clear()
        
        return f"交易完成！交易号: {transaction['id']}"
    
    @command("receipt", description="打印小票")
    def receipt(self):
        transaction = self.payment_service.get_last_transaction()
        if not transaction:
            return "暂无交易记录"
        return self.receipt_service.format_receipt(transaction)
    
    @command("refund", description="退款")
    def refund(self, txn_id: str):
        result = self.refund_service.refund(txn_id)
        return f"退款成功！退款单号: {result['refund_id']} 金额: ¥{result['amount']:.2f}"
    
    @command("report", description="销售报表 (daily/monthly)")
    def report(self, period: str):
        if period == "daily":
            return self.report_service.generate_daily_report()
        elif period == "monthly":
            return self.report_service.generate_monthly_report()
        else:
            return "参数错误！请使用 daily 或 monthly"


# ========== 启动函数 ==========

def main():
    import sys
    config_file = sys.argv[1] if len(sys.argv) > 1 else "config-json.yml"
    
    container = Container(
        config_file=config_file,
        layers=["presentation", "business", "data"],
        mode="relaxed"
    )
    
    from aspects import transaction_log, audit_log
    container.register_aspect(transaction_log, layer="business", point="around")
    container.register_aspect(audit_log, layer="data", point="after")
    
    from command_shell import CommandShell
    shell = CommandShell(container)
    shell.run()


if __name__ == "__main__":
    main()
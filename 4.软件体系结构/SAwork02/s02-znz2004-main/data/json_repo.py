import json
import os
from datetime import datetime, timedelta
from typing import Dict, List, Optional
from data.product_repo import ProductRepository
from data.transaction_repo import TransactionRepository


class JsonProductRepository(ProductRepository):
    """JSON 文件实现的商品仓储"""
    
    def __init__(self, data_file: str = "result/products.json"):
        os.makedirs(os.path.dirname(data_file), exist_ok=True)
        self.data_file = data_file
        self._init_data()
    
    def _init_data(self):
        if not os.path.exists(self.data_file):
            products = {
                "6901234567890": {"name": "可口可乐", "price": 3.5, "stock": 100},
                "6901234567891": {"name": "农夫山泉", "price": 2.0, "stock": 50},
                "6901234567892": {"name": "康师傅冰红茶", "price": 3.0, "stock": 80},
                "6901234567893": {"name": "统一方便面", "price": 5.0, "stock": 30},
            }
            with open(self.data_file, 'w', encoding='utf-8') as f:
                json.dump(products, f, ensure_ascii=False, indent=2)
    
    def _load_data(self) -> Dict:
        with open(self.data_file, 'r', encoding='utf-8') as f:
            return json.load(f)
    
    def _save_data(self, data: Dict):
        with open(self.data_file, 'w', encoding='utf-8') as f:
            json.dump(data, f, ensure_ascii=False, indent=2)
    
    def find_by_barcode(self, barcode: str) -> Optional[Dict]:
        data = self._load_data()
        product = data.get(barcode)
        if product:
            return {"barcode": barcode, **product}
        return None
    
    def update_stock(self, barcode: str, quantity: int) -> bool:
        data = self._load_data()
        if barcode in data:
            data[barcode]['stock'] += quantity
            self._save_data(data)
            return True
        return False
    
    def get_all_products(self) -> List[Dict]:
        data = self._load_data()
        return [{"barcode": k, **v} for k, v in data.items()]


class JsonTransactionRepository(TransactionRepository):
    """JSON 文件实现的交易仓储"""
    
    def __init__(self, data_file: str = "result/transactions.json"):
        os.makedirs(os.path.dirname(data_file), exist_ok=True)
        self.data_file = data_file
        self._init_data()
    
    def _init_data(self):
        if not os.path.exists(self.data_file):
            with open(self.data_file, 'w', encoding='utf-8') as f:
                json.dump([], f)
    
    def _load_data(self) -> List:
        with open(self.data_file, 'r', encoding='utf-8') as f:
            return json.load(f)
    
    def _save_data(self, data: List):
        with open(self.data_file, 'w', encoding='utf-8') as f:
            json.dump(data, f, ensure_ascii=False, indent=2)
    
    def save(self, transaction: Dict) -> str:
        data = self._load_data()
        txn_id = f"txn_{datetime.now().strftime('%Y%m%d_%H%M%S')}"
        transaction['id'] = txn_id
        transaction['timestamp'] = datetime.now().isoformat()
        data.append(transaction)
        self._save_data(data)
        return txn_id
    
    def find_by_id(self, txn_id: str) -> Optional[Dict]:
        data = self._load_data()
        for txn in data:
            if txn.get('id') == txn_id:
                return txn
        return None
    
    def get_daily_report(self, date: str) -> Dict:
        data = self._load_data()
        total_amount = 0
        total_count = 0
        product_sales = {}
        
        for txn in data:
            if txn['timestamp'].startswith(date):
                total_amount += txn['total']
                total_count += 1
                for item in txn.get('items', []):
                    barcode = item['barcode']
                    if barcode not in product_sales:
                        product_sales[barcode] = {
                            'name': item['name'],
                            'quantity': 0,
                            'amount': 0
                        }
                    product_sales[barcode]['quantity'] += item['quantity']
                    product_sales[barcode]['amount'] += item['quantity'] * item['price']
        
        return {
            'date': date,
            'total_amount': total_amount,
            'total_transactions': total_count,
            'product_sales': product_sales
        }
    
    def get_monthly_report(self, year_month: str) -> Dict:
        data = self._load_data()
        total_amount = 0
        total_count = 0
        daily_stats = {}
        
        for txn in data:
            if txn['timestamp'].startswith(year_month):
                date = txn['timestamp'][:10]
                total_amount += txn['total']
                total_count += 1
                if date not in daily_stats:
                    daily_stats[date] = {'amount': 0, 'count': 0}
                daily_stats[date]['amount'] += txn['total']
                daily_stats[date]['count'] += 1
        
        return {
            'year_month': year_month,
            'total_amount': total_amount,
            'total_transactions': total_count,
            'daily_stats': daily_stats
        }
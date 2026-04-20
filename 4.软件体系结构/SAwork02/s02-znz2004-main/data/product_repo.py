from typing import Dict, List, Optional


class ProductRepository:
    """商品仓储接口"""
    
    def find_by_barcode(self, barcode: str) -> Optional[Dict]:
        raise NotImplementedError
    
    def update_stock(self, barcode: str, quantity: int) -> bool:
        raise NotImplementedError
    
    def get_all_products(self) -> List[Dict]:
        raise NotImplementedError
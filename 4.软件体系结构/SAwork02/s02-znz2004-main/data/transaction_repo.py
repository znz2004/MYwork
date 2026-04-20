from typing import Dict, List, Optional


class TransactionRepository:
    """交易仓储接口"""
    
    def save(self, transaction: Dict) -> str:
        raise NotImplementedError
    
    def find_by_id(self, txn_id: str) -> Optional[Dict]:
        raise NotImplementedError
    
    def get_daily_report(self, date: str) -> Dict:
        raise NotImplementedError
    
    def get_monthly_report(self, year_month: str) -> Dict:
        raise NotImplementedError
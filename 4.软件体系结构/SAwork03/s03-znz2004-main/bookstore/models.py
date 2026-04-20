from typing import List, Optional
import json
import os


class Book:
    """书籍模型"""
    
    def __init__(self, id: str, title: str, author: str, price: float):
        self.id = id
        self.title = title
        self.author = author
        self.price = price
    
    def to_dict(self) -> dict:
        return {
            "id": self.id,
            "title": self.title,
            "author": self.author,
            "price": self.price
        }
    
    @staticmethod
    def from_dict(data: dict) -> 'Book':
        return Book(data['id'], data['title'], data['author'], data['price'])
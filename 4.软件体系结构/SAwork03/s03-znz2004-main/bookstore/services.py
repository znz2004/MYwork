import json
import os
from typing import List, Optional
from bookstore.models import Book
from framework.decorators import component, post_construct


@component
class BookService:
    """书籍服务"""
    
    def __init__(self, data_file: str = "bookstore/data/books.json"):
        self.data_file = data_file
        self._books: List[Book] = []
    
    @post_construct
    def init(self):
        self._ensure_data_dir()
        self._load_data()
        print(f"[BookService] 加载了 {len(self._books)} 本书")
    
    def _ensure_data_dir(self):
        os.makedirs(os.path.dirname(self.data_file), exist_ok=True)
    
    def _load_data(self):
        if os.path.exists(self.data_file):
            with open(self.data_file, 'r', encoding='utf-8') as f:
                data = json.load(f)
                self._books = [Book.from_dict(b) for b in data]
        else:
            self._books = [
                Book("1", "三体", "刘慈欣", 68.0),
                Book("2", "百年孤独", "马尔克斯", 55.0),
                Book("3", "人工智能简史", "尼克", 79.0),
            ]
            self._save_data()
    
    def _save_data(self):
        with open(self.data_file, 'w', encoding='utf-8') as f:
            json.dump([b.to_dict() for b in self._books], f, ensure_ascii=False, indent=2)
    
    def find_all(self) -> List[Book]:
        return self._books
    
    def find_by_id(self, book_id: str) -> Optional[Book]:
        for book in self._books:
            if book.id == book_id:
                return book
        return None
    
    def add(self, title: str, author: str, price: float) -> Book:
        max_id = max([int(b.id) for b in self._books], default=0)
        new_id = str(max_id + 1)
        
        book = Book(new_id, title, author, price)
        self._books.append(book)
        self._save_data()
        return book
    
    def delete(self, book_id: str) -> bool:
        book = self.find_by_id(book_id)
        if book:
            self._books.remove(book)
            self._save_data()
            return True
        return False
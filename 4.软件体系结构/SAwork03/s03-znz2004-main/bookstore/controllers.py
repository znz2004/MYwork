from bookstore.services import BookService
from framework.decorators import component, route
from framework.request import Request
from framework.response import view, redirect, json_view


@component
class BookController:
    
    def __init__(self, book_service: BookService):
        self.book_service = book_service
        print(f"[BookController] 初始化完成")
    
    # ========== 固定路径（没有参数）优先 ==========
    
    @route("/books/new", method="GET")
    def new_form(self, request: Request):
        print(f"[BookController.new_form] 被调用")
        return view("book/new.html", {})
    
    @route("/books", method="GET")
    def list(self, request: Request):
        print(f"[BookController.list] 被调用")
        books = self.book_service.find_all()
        return view("book/list.html", {"books": books})
    
    @route("/books", method="POST")
    def create(self, request: Request):
        print(f"[BookController.create] 被调用")
        form = request.form()
        title = form.get('title', '')
        author = form.get('author', '')
        price = float(form.get('price', 0))
        
        self.book_service.add(title, author, price)
        return redirect("/books")
    
    # ========== 参数路径（包含参数）放在后面 ==========
    
    @route("/books/<id>", method="GET")
    def show(self, request: Request, id: str):
        print(f"[BookController.show] 被调用, id={id}")
        book = self.book_service.find_by_id(id)
        if not book:
            return view("error/404.html", {"path": f"/books/{id}"})
        return view("book/show.html", {"book": book})
    
    @route("/books/<id>/delete", method="POST")
    def delete(self, request: Request, id: str):
        print(f"[BookController.delete] 被调用, id={id}")
        self.book_service.delete(id)
        return redirect("/books")
    
    @route("/api/books", method="GET")
    def api_list(self, request: Request):
        print(f"[BookController.api_list] 被调用")
        books = self.book_service.find_all()
        return json_view([b.to_dict() for b in books])
    
    @route("/api/books/<id>", method="GET")
    def api_show(self, request: Request, id: str):
        print(f"[BookController.api_show] 被调用, id={id}")
        book = self.book_service.find_by_id(id)
        if not book:
            return json_view({"error": "Book not found"})
        return json_view(book.to_dict())
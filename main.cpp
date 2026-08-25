#include "crow.h"
#include <vector>
#include <string>
#include <algorithm> 

// Windows ke 'DELETE' macro ko hatane ke liye ye line add karein:
#undef DELETE

// Book ka structure
struct Book {
    int id;
    std::string title;
    std::string author;
};

int main() {
    crow::SimpleApp app;
    std::vector<Book> library;

    // Dummy books
    library.push_back({1, "C++ Programming", "Bjarne Stroustrup"});
    library.push_back({2, "Clean Code", "Robert C. Martin"});

    // 1. GET Route
    CROW_ROUTE(app, "/books").methods(crow::HTTPMethod::GET)([&library](){
        crow::json::wvalue x;
        for(size_t i = 0; i < library.size(); i++) {
            x[i]["id"] = library[i].id;
            x[i]["title"] = library[i].title;
            x[i]["author"] = library[i].author;
        }
        return x;
    });

    // 2. POST Route
    CROW_ROUTE(app, "/books").methods(crow::HTTPMethod::POST)([&library](const crow::request& req){
        auto x = crow::json::load(req.body);
        if (!x) return crow::response(400, "Invalid JSON");
        
        Book new_book;
        new_book.id = x["id"].i();
        new_book.title = x["title"].s();
        new_book.author = x["author"].s();
        
        library.push_back(new_book);
        return crow::response(201, "Book added successfully!");
    });

    // 3. DELETE Route
    CROW_ROUTE(app, "/book/<int>").methods(crow::HTTPMethod::DELETE)([&library](int id){
        auto it = std::remove_if(library.begin(), library.end(), [id](const Book& b){ 
            return b.id == id; 
        });

        if (it != library.end()) {
            library.erase(it, library.end());
            return crow::response(200, "Book deleted successfully!");
        }
        return crow::response(404, "Book not found!");
    });
    // 4. GET Route: Kisi ek specific ID wali book ko dhundhna
    CROW_ROUTE(app, "/book/<int>").methods(crow::HTTPMethod::GET)([&library](int id){
        for(const auto& book : library) {
            if(book.id == id) {
                crow::json::wvalue x;
                x["id"] = book.id;
                x["title"] = book.title;
                x["author"] = book.author;
                return crow::response(200, x);
            }
        }
        return crow::response(404, "Book not found!");
    });

    // 5. PUT Route: Kisi existing book ki details Update/Edit karna
    CROW_ROUTE(app, "/book/<int>").methods(crow::HTTPMethod::PUT)([&library](const crow::request& req, int id){
        auto x = crow::json::load(req.body);
        if (!x) return crow::response(400, "Invalid JSON");

        for(auto& book : library) {
            if(book.id == id) {
                // Naya data set kar rahe hain
                book.title = x["title"].s();
                book.author = x["author"].s();
                return crow::response(200, "Book updated successfully!");
            }
        }
        return crow::response(404, "Book not found!");
    });

    app.port(8080).multithreaded().run();
    return 0;
}
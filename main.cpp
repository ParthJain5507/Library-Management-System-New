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
// User ka structure
struct User {
    std::string username;
    std::string password;
    std::string token;
};

int main() {
    crow::SimpleApp app;
    std::vector<Book> library;

    // Dummy books
    library.push_back({1, "C++ Programming", "Bjarne Stroustrup"});
    library.push_back({2, "Clean Code", "Robert C. Martin"});
    std::vector<User> users;
    // Ek dummy admin account banate hain
    users.push_back({"admin", "admin123", "secret-admin-token"});

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

    // 2. POST Route: Nayi book add karne ke liye (Ab Secured Hai)
    CROW_ROUTE(app, "/books").methods(crow::HTTPMethod::POST)([&library](const crow::request& req){
        
        // 🔒 SECURITY CHECK: Token check kar rahe hain
        auto auth_header = req.get_header_value("Authorization");
        if (auth_header != "secret-admin-token") {
            return crow::response(401, "Unauthorized! Sirf Admin books add kar sakta hai.");
        }

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
    // 6. LOGIN Route: Username/Password check karke Token dena
    CROW_ROUTE(app, "/login").methods(crow::HTTPMethod::POST)([&users](const crow::request& req){
        auto x = crow::json::load(req.body);
        if (!x || !x.has("username") || !x.has("password")) {
            return crow::response(400, "Username aur password zaroori hai!");
        }

        std::string uname = x["username"].s();
        std::string pwd = x["password"].s();

        for (const auto& u : users) {
            if (u.username == uname && u.password == pwd) {
                crow::json::wvalue res;
                res["message"] = "Login successful!";
                res["token"] = u.token; // Admin ko token de rahe hain
                return crow::response(200, res);
            }
        }
        return crow::response(401, "Galat Username ya Password!");
    });

    app.port(8080).multithreaded().run();
    return 0;
}
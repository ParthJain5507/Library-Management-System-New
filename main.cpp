#include "crow.h"
#include <vector>
#include <string>
#include <algorithm>
#include <ctime>
#include <fstream>   // Naya: File me likhne ke liye (File I/O)
#include <sstream>   // Naya: File se data padhne ke liye

#undef DELETE

struct Book {
    int id;
    std::string title;
    std::string author;
    bool is_issued;
    std::string student_id;
    time_t due_date;
};

struct User {
    std::string username;
    std::string password;
    std::string token;
    std::string role;
};

// --- 1. DATABASE SAVE KARNE KA FUNCTION ---
void saveDatabase(const std::vector<Book>& library) {
    crow::json::wvalue x;
    for (size_t i = 0; i < library.size(); i++) {
        x[i]["id"] = library[i].id;
        x[i]["title"] = library[i].title;
        x[i]["author"] = library[i].author;
        x[i]["is_issued"] = library[i].is_issued;
        x[i]["student_id"] = library[i].student_id;
        x[i]["due_date"] = (long)library[i].due_date;
    }
    // "library_data.json" naam ki file banayega
    std::ofstream file("library_data.json");
    if (file.is_open()) {
        file << x.dump();
        file.close();
    }
}

// --- 2. DATABASE LOAD KARNE KA FUNCTION ---
void loadDatabase(std::vector<Book>& library) {
    std::ifstream file("library_data.json");
    if (!file.is_open()) {
        // Agar file nahi hai (First Time), toh dummy books daalo aur file create kar do
        library.push_back({1, "C++ Programming", "Bjarne Stroustrup", false, "", 0});
        library.push_back({2, "Clean Code", "Robert C. Martin", false, "", 0});
        saveDatabase(library);
        return;
    }

    // File se data read karna
    std::stringstream buffer;
    buffer << file.rdbuf();
    auto x = crow::json::load(buffer.str());
    
    if (x) {
        for (const auto& item : x) {
            Book b;
            b.id = item["id"].i();
            b.title = item["title"].s();
            b.author = item["author"].s();
            b.is_issued = item["is_issued"].b();
            b.student_id = item["student_id"].s();
            b.due_date = item["due_date"].i();
            library.push_back(b);
        }
    }
}


int main() {
    crow::SimpleApp app;
    
    std::vector<Book> library;
    
    // --- 3. SERVER START HOTE HI PEHLE DATA LOAD KAREIN ---
    loadDatabase(library);

    std::vector<User> users;
    users.push_back({"admin", "admin123", "secret-admin-token", "admin"});
    users.push_back({"rahul99", "pass123", "rahul-token", "student"});

    // [LOGIN ROUTE]
    CROW_ROUTE(app, "/login").methods(crow::HTTPMethod::POST)([&users](const crow::request& req){
        auto x = crow::json::load(req.body);
        if (!x || !x.has("username") || !x.has("password")) return crow::response(400, "Username/Password missing");
        std::string uname = x["username"].s(), pwd = x["password"].s();
        for (const auto& u : users) {
            if (u.username == uname && u.password == pwd) {
                crow::json::wvalue res;
                res["message"] = "Welcome " + u.role + "!";
                res["token"] = u.token;
                res["role"] = u.role;
                return crow::response(200, res);
            }
        }
        return crow::response(401, "Galat Username ya Password!");
    });

    // [GET BOOKS ROUTE]
    CROW_ROUTE(app, "/books").methods(crow::HTTPMethod::GET)([&library](){
        crow::json::wvalue x;
        for(size_t i = 0; i < library.size(); i++) {
            x[i]["id"] = library[i].id;
            x[i]["title"] = library[i].title;
            x[i]["is_issued"] = library[i].is_issued;
            if(library[i].is_issued) x[i]["due_date"] = (long)library[i].due_date;
        }
        return x;
    });

    // [ADD BOOK ROUTE]
    CROW_ROUTE(app, "/books").methods(crow::HTTPMethod::POST)([&library](const crow::request& req){
        auto auth = req.get_header_value("Authorization");
        if (auth != "secret-admin-token") return crow::response(401, "Unauthorized!");
        auto x = crow::json::load(req.body);
        if (!x || !x.has("id") || !x.has("title") || !x.has("author")) return crow::response(400, "Invalid JSON");
        
        Book b;
        b.id = x["id"].i(); b.title = x["title"].s(); b.author = x["author"].s();
        b.is_issued = false; b.student_id = ""; b.due_date = 0;
        
        library.push_back(b);
        saveDatabase(library); // <-- DATA SAVE KIYA
        return crow::response(201, "Book added successfully!");
    });

    // [ISSUE BOOK ROUTE]
    CROW_ROUTE(app, "/issue").methods(crow::HTTPMethod::POST)([&library](const crow::request& req){
        auto token = req.get_header_value("Authorization");
        if (token != "rahul-token") return crow::response(401, "Unauthorized!");
        auto x = crow::json::load(req.body);
        if (!x || !x.has("book_id")) return crow::response(400, "Book ID missing");
        
        int b_id = x["book_id"].i();
        for (auto& book : library) {
            if (book.id == b_id) {
                if (book.is_issued) return crow::response(400, "Ye book already issued hai!");
                book.is_issued = true; book.student_id = "rahul99";
                book.due_date = time(0) + (7 * 24 * 60 * 60); 
                
                saveDatabase(library); // <-- DATA SAVE KIYA
                return crow::response(200, "Book issued successfully!");
            }
        }
        return crow::response(404, "Book nahi mili!");
    });

    // [RETURN BOOK ROUTE]
    CROW_ROUTE(app, "/return").methods(crow::HTTPMethod::POST)([&library](const crow::request& req){
        auto token = req.get_header_value("Authorization");
        if (token != "rahul-token") return crow::response(401, "Unauthorized!");
        auto x = crow::json::load(req.body);
        if (!x || !x.has("book_id")) return crow::response(400, "Book ID missing");
        
        int b_id = x["book_id"].i();
        for (auto& book : library) {
            if (book.id == b_id) {
                if (!book.is_issued) return crow::response(400, "Book pehle se library me hai!");
                book.is_issued = false; book.student_id = ""; book.due_date = 0;
                
                saveDatabase(library); // <-- DATA SAVE KIYA
                return crow::response(200, "Book returned successfully!");
            }
        }
        return crow::response(404, "Book nahi mili!");
    });

    app.port(8080).multithreaded().run();
    return 0;
}
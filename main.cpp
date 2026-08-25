#include "crow.h"
#include <vector>
#include <string>
#include <algorithm>
#include <ctime> // Date aur Time ke liye

#undef DELETE

// 1. Naya Book Structure
struct Book {
    int id;
    std::string title;
    std::string author;
    bool is_issued;             // Kya book kisi ke paas hai?
    std::string student_id;     // Kisne issue ki hai?
    time_t due_date;            // Wapas kab karni hai? (Timestamp)
};

// 2. Naya User Structure (Role ke sath)
struct User {
    std::string username;
    std::string password;
    std::string token;
    std::string role;           // "admin" ya "student"
};

int main() {
    crow::SimpleApp app;
    
    std::vector<Book> library;
    // Initial books (shuru me koi issue nahi hui hai)
    library.push_back({1, "C++ Programming", "Bjarne Stroustrup", false, "", 0});
    library.push_back({2, "Clean Code", "Robert C. Martin", false, "", 0});

    std::vector<User> users;
    // Ek Admin aur ek Student ka account
    users.push_back({"admin", "admin123", "secret-admin-token", "admin"});
    users.push_back({"rahul99", "pass123", "rahul-token", "student"});

    // --- PURANE ROUTES (LOGIN, GET, POST, DELETE) ---

    CROW_ROUTE(app, "/login").methods(crow::HTTPMethod::POST)([&users](const crow::request& req){
        auto x = crow::json::load(req.body);
        if (!x || !x.has("username") || !x.has("password")) return crow::response(400, "Username/Password missing");

        std::string uname = x["username"].s();
        std::string pwd = x["password"].s();

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

    // --- NAYE ROUTES (ISSUE & RETURN) ---

    // 1. Book Issue Karna (Sirf Student kar sakta hai)
    CROW_ROUTE(app, "/issue").methods(crow::HTTPMethod::POST)([&library, &users](const crow::request& req){
        auto token = req.get_header_value("Authorization");
        if (token != "rahul-token") return crow::response(401, "Unauthorized! Sirf Student login karein.");

        auto x = crow::json::load(req.body);
        if (!x || !x.has("book_id")) return crow::response(400, "Book ID missing");
        int b_id = x["book_id"].i();

        for (auto& book : library) {
            if (book.id == b_id) {
                if (book.is_issued) return crow::response(400, "Ye book already kisi aur ke paas hai!");
                
                book.is_issued = true;
                book.student_id = "rahul99";
                
                // 7 Din ki Due date set kar rahe hain (7 days * 24 hrs * 60 min * 60 sec)
                book.due_date = time(0) + (7 * 24 * 60 * 60); 

                return crow::response(200, "Book successfully issued for 7 days!");
            }
        }
        return crow::response(404, "Book nahi mili!");
    });

    // 2. Book Return Karna aur Fine calculate karna
    CROW_ROUTE(app, "/return").methods(crow::HTTPMethod::POST)([&library](const crow::request& req){
        auto token = req.get_header_value("Authorization");
        if (token != "rahul-token") return crow::response(401, "Unauthorized!");

        auto x = crow::json::load(req.body);
        if (!x || !x.has("book_id")) return crow::response(400, "Book ID missing");
        int b_id = x["book_id"].i();

        for (auto& book : library) {
            if (book.id == b_id) {
                if (!book.is_issued) return crow::response(400, "Ye book pehle se hi library me hai!");
                if (book.student_id != "rahul99") return crow::response(403, "Ye book aapne issue nahi ki hai!");

                // Fine Calculation
                time_t now = time(0);
                double diff_seconds = difftime(now, book.due_date);
                int fine = 0;

                if (diff_seconds > 0) {
                    int days_late = diff_seconds / (24 * 60 * 60);
                    fine = days_late * 10; // Rs 10 per day late fine
                }

                // Book wapas library me aa gayi
                book.is_issued = false;
                book.student_id = "";
                book.due_date = 0;

                crow::json::wvalue res;
                res["message"] = "Book returned successfully!";
                res["fine_amount"] = fine;
                return crow::response(200, res);
            }
        }
        return crow::response(404, "Book nahi mili!");
    });

    app.port(8080).multithreaded().run();
    return 0;
}
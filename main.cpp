#include "crow.h"
#include <vector>
#include <string>

// Book ka structure
struct Book {
    int id;
    std::string title;
    std::string author;
};

int main() {
    crow::SimpleApp app;
    std::vector<Book> library;

    // Dummy books add kar rahe hain
    library.push_back({1, "C++ Programming", "Bjarne Stroustrup"});
    library.push_back({2, "Clean Code", "Robert C. Martin"});

    // Route: Saari books dekhne ke liye (GET Request)
    CROW_ROUTE(app, "/books")([&library](){
        crow::json::wvalue x;
        for(size_t i = 0; i < library.size(); i++) {
            x[i]["id"] = library[i].id;
            x[i]["title"] = library[i].title;
            x[i]["author"] = library[i].author;
        }
        return x;
    });

    // Server ko port 8080 par start karein
    app.port(8080).multithreaded().run();
    return 0;
}
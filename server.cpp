/**
 * Book Recommendation System - C++ HTTP Server
 * 
 * Compile with:
 *   g++ -std=c++17 -I include -o server server.cpp -lws2_32
 * 
 * Run:
 *   ./server
 */

#include "include/httplib.h"
#include "include/recommender.h"
#include <iostream>
#include <sstream>

BookRecommender recommender;

// Convert vector of books to JSON array
std::string booksToJson(const std::vector<Book>& books) {
    std::stringstream ss;
    ss << "[";
    for (size_t i = 0; i < books.size(); i++) {
        if (i > 0) ss << ",";
        ss << books[i].toJson();
    }
    ss << "]";
    return ss.str();
}

// Convert recommendations to JSON
std::string recommendationsToJson(const std::vector<std::pair<Book, double>>& recs, Book* sourceBook) {
    std::stringstream ss;
    ss << "{";
    
    // Source book
    if (sourceBook) {
        ss << "\"source_book\":" << sourceBook->toJson() << ",";
    } else {
        ss << "\"source_book\":null,";
    }
    
    // Recommendations array
    ss << "\"recommendations\":[";
    for (size_t i = 0; i < recs.size(); i++) {
        if (i > 0) ss << ",";
        ss << "{\"book\":" << recs[i].first.toJson() 
           << ",\"score\":" << recs[i].second << "}";
    }
    ss << "],";
    
    ss << "\"count\":" << recs.size();
    ss << "}";
    
    return ss.str();
}

int main() {
    std::cout << "=== Book Recommendation System ===" << std::endl;
    std::cout << "Starting server..." << std::endl;
    
    // Load book data
    recommender.loadData("GoodReads_100k_books.csv");
    
    if (!recommender.loaded()) {
        std::cerr << "Failed to load book data!" << std::endl;
        return 1;
    }

    httplib::Server server;

    // Search endpoint
    server.Get("/api/search", [](const httplib::Request& req, httplib::Response& res) {
        std::string query = req.params.count("q") ? req.params.at("q") : "";
        int limit = 10;
        if (req.params.count("limit")) {
            try { limit = std::stoi(req.params.at("limit")); } catch(...) {}
        }

        if (query.empty()) {
            res.set_content("{\"books\":[],\"error\":\"Please provide a search query\"}", "application/json");
            return;
        }

        auto books = recommender.searchBooks(query, limit);
        std::stringstream ss;
        ss << "{\"books\":" << booksToJson(books) << ",\"count\":" << books.size() << "}";
        res.set_content(ss.str(), "application/json");
    });

    // Recommend endpoint
    server.Get("/api/recommend", [](const httplib::Request& req, httplib::Response& res) {
        std::string title = req.params.count("title") ? req.params.at("title") : "";
        int k = 10;
        if (req.params.count("k")) {
            try { k = std::stoi(req.params.at("k")); } catch(...) {}
        }

        if (title.empty()) {
            res.set_content("{\"recommendations\":[],\"error\":\"Please provide a book title\"}", "application/json");
            return;
        }

        auto recommendations = recommender.getRecommendations(title, k);
        Book* sourceBook = recommender.getBookByTitle(title);

        if (recommendations.empty()) {
            std::stringstream ss;
            ss << "{\"recommendations\":[],\"source_book\":null,\"error\":\"No recommendations found for \\\"" 
               << title << "\\\". Try searching for a different book.\"}";
            res.set_content(ss.str(), "application/json");
            return;
        }

        res.set_content(recommendationsToJson(recommendations, sourceBook), "application/json");
    });

    // Health check
    server.Get("/api/health", [](const httplib::Request& req, httplib::Response& res) {
        res.set_content("{\"status\":\"ok\"}", "application/json");
    });

    std::cout << "\nServer ready! Open index.html in your browser." << std::endl;
    std::cout << "API running at http://localhost:8080" << std::endl;
    
    server.listen("0.0.0.0", 8080);
    
    return 0;
}

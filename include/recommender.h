#ifndef RECOMMENDER_H
#define RECOMMENDER_H

#include "book.h"
#include "graph.h"
#include "data_loader.h"
#include <memory>

class BookRecommender {
private:
    BookGraph graph;
    bool isLoaded;

public:
    BookRecommender() : isLoaded(false) {}

    void loadData(const std::string& dataFile = "GoodReads_100k_books.csv") {
        std::cout << "Loading book data..." << std::endl;
        
        DataLoader loader(dataFile);
        std::vector<Book> books = loader.loadBooks();
        
        std::cout << "Adding " << books.size() << " books to graph..." << std::endl;
        for (const auto& book : books) {
            graph.addBook(book);
        }
        
        std::cout << "Building similarity edges..." << std::endl;
        graph.buildSimilarityEdges(0.3);
        
        std::cout << "Graph built: " << graph.size() << " books, " 
                  << graph.edgeCount() << " edges" << std::endl;
        
        isLoaded = true;
    }

    bool loaded() const { return isLoaded; }

    std::vector<Book> searchBooks(const std::string& query, int limit = 10) {
        if (!isLoaded) return {};
        return graph.searchBooks(query, limit);
    }

    std::vector<std::pair<Book, double>> getRecommendations(const std::string& title, int k = 10) {
        if (!isLoaded) return {};

        // Find source book
        Book* sourceBook = graph.getBookByName(title);
        
        if (!sourceBook) {
            // Try fuzzy search
            auto matches = graph.searchBooks(title, 1);
            if (!matches.empty()) {
                sourceBook = graph.getBookById(matches[0].id);
            }
        }

        if (!sourceBook) return {};

        // Get recommendations using hybrid approach
        auto neighbors = graph.getTopKSimilar(sourceBook->id, k);
        auto bfsResults = graph.bfsRecommendations(sourceBook->id, k);

        // Merge results
        std::set<int> seen;
        std::vector<std::pair<Book, double>> combined;

        for (const auto& pair : neighbors) {
            if (seen.find(pair.first.id) == seen.end()) {
                seen.insert(pair.first.id);
                combined.push_back(pair);
            }
        }

        for (const auto& pair : bfsResults) {
            if (seen.find(pair.first.id) == seen.end()) {
                seen.insert(pair.first.id);
                combined.push_back({pair.first, pair.second * 0.8});
            }
        }

        // Sort by score
        std::sort(combined.begin(), combined.end(),
            [](const auto& a, const auto& b) { return a.second > b.second; });

        if ((int)combined.size() > k) {
            combined.resize(k);
        }

        return combined;
    }

    Book* getBookByTitle(const std::string& title) {
        if (!isLoaded) return nullptr;
        
        Book* book = graph.getBookByName(title);
        if (!book) {
            auto matches = graph.searchBooks(title, 1);
            if (!matches.empty()) {
                book = graph.getBookById(matches[0].id);
            }
        }
        return book;
    }

    Book* getBookById(int id) {
        return graph.getBookById(id);
    }
};

#endif // RECOMMENDER_H

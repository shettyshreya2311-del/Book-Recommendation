#ifndef GRAPH_H
#define GRAPH_H

#include "book.h"
#include <unordered_map>
#include <vector>
#include <queue>
#include <algorithm>
#include <functional>
#ifdef min
#undef min
#endif

class BookGraph {
private:
    // Adjacency list: book_id -> [(neighbor_id, weight), ...]
    std::unordered_map<int, std::vector<std::pair<int, double>>> adjacencyList;
    
    // Book storage: book_id -> Book
    std::unordered_map<int, Book> books;
    
    // Name index: lowercase title -> book_id
    std::unordered_map<std::string, int> nameToId;

public:
    BookGraph() {}

    // Add a book to the graph
    void addBook(const Book& book) {
        books[book.id] = book;
        std::string normalizedName = Book::toLower(book.title);
        nameToId[normalizedName] = book.id;
    }

    // Add weighted edge between two books
    void addEdge(int bookId1, int bookId2, double weight) {
        if (books.count(bookId1) && books.count(bookId2)) {
            adjacencyList[bookId1].push_back({bookId2, weight});
            adjacencyList[bookId2].push_back({bookId1, weight});
        }
    }

    // Get book by ID
    Book* getBookById(int bookId) {
        auto it = books.find(bookId);
        return (it != books.end()) ? &it->second : nullptr;
    }

    // Get book by exact name match
    Book* getBookByName(const std::string& name) {
        std::string normalized = Book::toLower(name);
        auto it = nameToId.find(normalized);
        if (it != nameToId.end()) {
            return getBookById(it->second);
        }
        return nullptr;
    }

    // Search books by partial name match
    std::vector<Book> searchBooks(const std::string& query, int limit = 10) {
        std::string queryLower = Book::toLower(query);
        std::vector<std::tuple<int, double, Book>> matches; // (priority, rating, book)

        for (const auto& pair : nameToId) {
            const std::string& name = pair.first;
            if (name.find(queryLower) != std::string::npos) {
                Book& book = books[pair.second];
                int priority;
                if (name == queryLower) {
                    priority = -1; // Exact match
                } else if (name.find(queryLower) == 0) {
                    priority = 0;  // Starts with
                } else {
                    priority = 1;  // Contains
                }
                matches.push_back({priority, book.rating, book});
            }
        }

        // Sort by priority, then by rating (descending)
        std::sort(matches.begin(), matches.end(), 
            [](const auto& a, const auto& b) {
                if (std::get<0>(a) != std::get<0>(b)) 
                    return std::get<0>(a) < std::get<0>(b);
                return std::get<1>(a) > std::get<1>(b);
            });

        std::vector<Book> results;
        for (int i = 0; i < std::min((int)matches.size(), limit); i++) {
            results.push_back(std::get<2>(matches[i]));
        }
        return results;
    }

    // Build similarity edges between books by same author
    void buildSimilarityEdges(double threshold = 0.3) {
        // Group books by author
        std::unordered_map<std::string, std::vector<int>> authorBooks;
        
        for (const auto& pair : books) {
            const Book& book = pair.second;
            if (!book.author.empty()) {
                std::vector<std::string> authors = Book::split(Book::toLower(book.author), ',');
                for (const auto& author : authors) {
                    if (!author.empty()) {
                        authorBooks[author].push_back(book.id);
                    }
                }
            }
        }

        // Create edges between books by same author
        for (const auto& pair : authorBooks) {
            const std::vector<int>& bookList = pair.second;
            for (size_t i = 0; i < bookList.size(); i++) {
                for (size_t j = i + 1; j < bookList.size(); j++) {
                    Book& book1 = books[bookList[i]];
                    Book& book2 = books[bookList[j]];
                    double similarity = book1.similarityScore(book2);
                    if (similarity >= threshold) {
                        addEdge(bookList[i], bookList[j], similarity);
                    }
                }
            }
        }
    }

    // Get top-k similar books using priority queue (max-heap)
    std::vector<std::pair<Book, double>> getTopKSimilar(int bookId, int k = 10) {
        std::vector<std::pair<Book, double>> results;
        
        if (books.find(bookId) == books.end()) {
            return results;
        }

        auto& neighbors = adjacencyList[bookId];
        
        if (!neighbors.empty()) {
            // Use max-heap to get top-k neighbors
            std::priority_queue<std::pair<double, int>> maxHeap;
            
            for (const auto& neighbor : neighbors) {
                maxHeap.push({neighbor.second, neighbor.first});
            }

            std::set<int> seen;
            seen.insert(bookId);
            
            while (!maxHeap.empty() && (int)results.size() < k) {
                auto top = maxHeap.top();
                maxHeap.pop();
                
                if (seen.find(top.second) == seen.end()) {
                    seen.insert(top.second);
                    results.push_back({books[top.second], top.first});
                }
            }
        } else {
            // Fallback: compute similarity on the fly
            Book& sourceBook = books[bookId];
            std::priority_queue<std::pair<double, int>> maxHeap;
            
            for (const auto& pair : books) {
                if (pair.first != bookId) {
                    double score = sourceBook.similarityScore(pair.second);
                    if (score > 0) {
                        maxHeap.push({score, pair.first});
                    }
                }
            }

            while (!maxHeap.empty() && (int)results.size() < k) {
                auto top = maxHeap.top();
                maxHeap.pop();
                results.push_back({books[top.second], top.first});
            }
        }

        return results;
    }

    // BFS-based recommendations
    std::vector<std::pair<Book, double>> bfsRecommendations(int bookId, int k = 10, int maxDepth = 2) {
        std::vector<std::pair<Book, double>> results;
        
        if (books.find(bookId) == books.end()) {
            return results;
        }

        std::set<int> visited;
        visited.insert(bookId);
        
        // Queue: (book_id, depth, accumulated_weight)
        std::queue<std::tuple<int, int, double>> queue;
        queue.push({bookId, 0, 1.0});
        
        std::vector<std::pair<double, int>> candidates;

        while (!queue.empty()) {
            auto [currentId, depth, accWeight] = queue.front();
            queue.pop();

            if (depth >= maxDepth) continue;

            for (const auto& neighbor : adjacencyList[currentId]) {
                int neighborId = neighbor.first;
                double edgeWeight = neighbor.second;
                
                if (visited.find(neighborId) == visited.end()) {
                    visited.insert(neighborId);
                    double newWeight = accWeight * edgeWeight;
                    candidates.push_back({newWeight, neighborId});
                    queue.push({neighborId, depth + 1, newWeight});
                }
            }
        }

        // Sort by weight descending
        std::sort(candidates.begin(), candidates.end(), std::greater<>());

        for (int i = 0; i < std::min((int)candidates.size(), k); i++) {
            results.push_back({books[candidates[i].second], candidates[i].first});
        }

        return results;
    }

    size_t size() const { return books.size(); }
    
    size_t edgeCount() const {
        size_t count = 0;
        for (const auto& pair : adjacencyList) {
            count += pair.second.size();
        }
        return count / 2;
    }
};

#endif // GRAPH_H

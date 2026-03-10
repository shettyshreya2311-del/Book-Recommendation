#ifndef DATA_LOADER_H
#define DATA_LOADER_H

#include "book.h"
#include <fstream>
#include <sstream>
#include <iostream>
#include <vector>

class DataLoader {
private:
    std::string dataFile;

    // Parse a CSV line handling quoted fields
    std::vector<std::string> parseCSVLine(const std::string& line) {
        std::vector<std::string> fields;
        std::string field;
        bool inQuotes = false;
        
        for (size_t i = 0; i < line.size(); i++) {
            char c = line[i];
            
            if (c == '"') {
                if (inQuotes && i + 1 < line.size() && line[i + 1] == '"') {
                    field += '"';
                    i++;
                } else {
                    inQuotes = !inQuotes;
                }
            } else if (c == ',' && !inQuotes) {
                fields.push_back(field);
                field.clear();
            } else {
                field += c;
            }
        }
        fields.push_back(field);
        
        return fields;
    }

public:
    DataLoader(const std::string& file = "GoodReads_100k_books.csv") 
        : dataFile(file) {}

    std::vector<Book> loadBooks() {
        std::vector<Book> books;
        std::ifstream file(dataFile);
        
        if (!file.is_open()) {
            std::cerr << "Error: Could not open " << dataFile << std::endl;
            return books;
        }

        std::cout << "Loading " << dataFile << "..." << std::endl;
        
        std::string line;
        // Skip header
        std::getline(file, line);
        
        int id = 1;
        int count = 0;
        
        while (std::getline(file, line)) {
            if (line.empty()) continue;
            
            std::vector<std::string> fields = parseCSVLine(line);
            
            // Expected: author, genre, rating, title
            if (fields.size() >= 4) {
                std::string author = fields[0];
                std::string genre = fields[1];
                double rating = 0.0;
                try {
                    rating = std::stod(fields[2]);
                } catch (...) {
                    rating = 0.0;
                }
                std::string title = fields[3];
                
                if (!title.empty()) {
                    books.emplace_back(id++, title, author, genre, rating);
                    count++;
                }
            }
        }
        
        file.close();
        std::cout << "Loaded " << count << " books" << std::endl;
        
        return books;
    }
};

#endif // DATA_LOADER_H

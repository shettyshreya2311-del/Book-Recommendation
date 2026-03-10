#ifndef BOOK_H
#define BOOK_H

#include <string>
#include <vector>
#include <set>
#include <sstream>
#include <algorithm>
#include <cctype>
#ifdef max
#undef max
#endif

struct Book {
    int id;
    std::string title;
    std::string author;
    std::string genre;
    double rating;

    Book() : id(0), rating(0.0) {}
    
    Book(int id, const std::string& title, const std::string& author, 
         const std::string& genre, double rating)
        : id(id), title(title), author(author), genre(genre), rating(rating) {}

    // Helper to split string by delimiter
    static std::vector<std::string> split(const std::string& str, char delim) {
        std::vector<std::string> tokens;
        std::stringstream ss(str);
        std::string token;
        while (std::getline(ss, token, delim)) {
            // Trim whitespace
            size_t start = token.find_first_not_of(" \t");
            size_t end = token.find_last_not_of(" \t");
            if (start != std::string::npos) {
                tokens.push_back(token.substr(start, end - start + 1));
            }
        }
        return tokens;
    }

    // Convert string to lowercase
    static std::string toLower(const std::string& str) {
        std::string result = str;
        std::transform(result.begin(), result.end(), result.begin(), ::tolower);
        return result;
    }

    // Calculate similarity score between two books
    double similarityScore(const Book& other) const {
        double score = 0.0;

        // Author similarity (0.4 weight)
        if (!author.empty() && !other.author.empty()) {
            std::vector<std::string> thisAuthors = split(toLower(author), ',');
            std::vector<std::string> otherAuthors = split(toLower(other.author), ',');
            
            for (const auto& a1 : thisAuthors) {
                for (const auto& a2 : otherAuthors) {
                    if (a1 == a2) {
                        score += 0.4;
                        goto author_done;
                    }
                }
            }
        }
        author_done:

        // Rating similarity (0.3 weight)
        if (rating > 0 && other.rating > 0) {
            double ratingDiff = std::abs(rating - other.rating);
            score += std::max(0.0, 0.3 - ratingDiff * 0.06);
        }

        // Genre similarity (0.3 weight)
        if (!genre.empty() && !other.genre.empty()) {
            std::vector<std::string> thisGenres = split(toLower(genre), ',');
            std::vector<std::string> otherGenres = split(toLower(other.genre), ',');
            
            std::set<std::string> thisSet(thisGenres.begin(), thisGenres.end());
            std::set<std::string> otherSet(otherGenres.begin(), otherGenres.end());
            
            int commonCount = 0;
            for (const auto& g : thisSet) {
                if (otherSet.count(g)) commonCount++;
            }
            
            if (commonCount > 0) {
                double genreScore = (double)commonCount / std::max(thisSet.size(), otherSet.size());
                score += 0.3 * genreScore;
            }
        }

        return score;
    }

    // Convert to JSON string
    std::string toJson() const {
        std::stringstream ss;
        ss << "{";
        ss << "\"id\":" << id << ",";
        ss << "\"title\":\"" << escapeJson(title) << "\",";
        ss << "\"author\":\"" << escapeJson(author) << "\",";
        ss << "\"genre\":\"" << escapeJson(genre) << "\",";
        ss << "\"rating\":" << rating;
        ss << "}";
        return ss.str();
    }

private:
    static std::string escapeJson(const std::string& str) {
        std::string result;
        for (char c : str) {
            switch (c) {
                case '"': result += "\\\""; break;
                case '\\': result += "\\\\"; break;
                case '\n': result += "\\n"; break;
                case '\r': result += "\\r"; break;
                case '\t': result += "\\t"; break;
                default: result += c;
            }
        }
        return result;
    }
};

#endif // BOOK_H

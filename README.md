# 📚 Book Recommendation System

A book recommendation system using C++ for backend data structures and algorithms, with a static HTML/CSS frontend.

## Features

- **Search Books**: Search for books by title
- **Get Recommendations**: Input a book title and get Top-K similar book recommendations
- **Graph-Based Recommendations**: Uses graph traversal algorithms (BFS) and similarity scoring
- **Genre-Based Similarity**: Recommends books based on shared genres, authors, and ratings

## Project Structure

```
DSA_EL/
├── include/                  # C++ header files
│   ├── book.h               # Book class with similarity scoring
│   ├── graph.h              # Graph data structure (adjacency list)
│   ├── data_loader.h        # CSV data loading
│   ├── recommender.h        # Recommendation engine
│   └── httplib.h            # HTTP server library
├── server.cpp               # Main C++ server
├── index.html               # Frontend HTML (static)
├── static/
│   └── style.css            # CSS styles
├── GoodReads_100k_books.csv # Book dataset
└── README.md                # This file
```

## Data Structures Used

- **Graph (Adjacency List)**: Books are nodes, edges represent similarity relationships
- **Hash Map (unordered_map)**: O(1) lookup for books by ID and name
- **Priority Queue (Max-Heap)**: Efficient Top-K retrieval
- **BFS Queue**: For multi-hop recommendations

## Algorithms

1. **Similarity Scoring**: Based on author overlap, rating similarity, and genre overlap
2. **BFS Traversal**: Explores neighbors up to max depth for recommendations
3. **Top-K Selection**: Uses max-heap for efficient selection

## Setup Instructions

### Prerequisites

- C++ compiler with C++17 support (g++, MSVC, clang++)
- Windows: MinGW-w64 or Visual Studio

### 1. Compile the Server

**Windows (MinGW-w64):**
```bash
g++ -std=c++17 -I include -o server.exe server.cpp -lws2_32
```

**Windows (Visual Studio Developer Command Prompt):**
```bash
cl /EHsc /std:c++17 /I include server.cpp ws2_32.lib
```

**Linux/macOS:**
```bash
g++ -std=c++17 -I include -o server server.cpp -pthread
```

### 2. Run the Server

```bash
./server.exe    # Windows
./server        # Linux/macOS
```

The server will start at `http://localhost:8080`

### 3. Open the Frontend

Open `index.html` directly in your web browser.

**Note:** The HTML file communicates with the C++ backend running on port 8080.

## Usage

1. **Start the C++ server** (it loads the book data)
2. **Open index.html** in your browser
3. **Search for a book** by entering a title
4. **Click on a search result** to get recommendations
5. **Explore similar books** by clicking "Get Similar"

## API Endpoints

| Endpoint | Method | Description |
|----------|--------|-------------|
| `/api/search?q=<query>` | GET | Search books by title |
| `/api/recommend?title=<title>&k=<num>` | GET | Get recommendations |
| `/api/health` | GET | Health check |

## Time Complexity

| Operation | Complexity |
|-----------|------------|
| Search | O(n) where n = number of books |
| Get Recommendations | O(k log k) for Top-K with heap |
| Build Graph | O(n × m) where m = avg edges per node |

## Technologies

- **Backend**: C++17 with custom HTTP server
- **Data Structures**: Graph, Hash Map, Priority Queue
- **Frontend**: HTML5, CSS3, JavaScript (Vanilla)
- **Dataset**: GoodReads 100k books

## License

This project is for educational purposes - DSA Project.

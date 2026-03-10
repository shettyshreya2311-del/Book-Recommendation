//
//  httplib.h - A single-file C++ HTTP/HTTPS library (minimal version)
//  Copyright (c) 2024 MIT License
//  Simplified for this project
//

#ifndef HTTPLIB_H
#define HTTPLIB_H

#ifdef _WIN32
#ifndef _WIN32_WINNT
#define _WIN32_WINNT 0x0600
#endif
#include <winsock2.h>
#include <ws2tcpip.h>
#pragma comment(lib, "ws2_32.lib")
#else
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#define SOCKET int
#define INVALID_SOCKET -1
#define SOCKET_ERROR -1
#define closesocket close
#endif

#include <string>
#include <map>
#include <functional>
#include <sstream>
#include <iostream>
#include <fstream>
#include <thread>
#include <vector>
#include <regex>

namespace httplib {

struct Request {
    std::string method;
    std::string path;
    std::map<std::string, std::string> params;
    std::map<std::string, std::string> headers;
    std::string body;
};

struct Response {
    int status = 200;
    std::map<std::string, std::string> headers;
    std::string body;

    void set_content(const std::string& content, const std::string& content_type) {
        body = content;
        headers["Content-Type"] = content_type;
    }

    void set_header(const std::string& key, const std::string& value) {
        headers[key] = value;
    }
};

using Handler = std::function<void(const Request&, Response&)>;

class Server {
private:
    std::map<std::string, Handler> get_handlers;
    SOCKET server_socket = INVALID_SOCKET;
    bool running = false;

    void parseQueryString(const std::string& query, std::map<std::string, std::string>& params) {
        std::istringstream stream(query);
        std::string pair;
        while (std::getline(stream, pair, '&')) {
            size_t pos = pair.find('=');
            if (pos != std::string::npos) {
                std::string key = urlDecode(pair.substr(0, pos));
                std::string value = urlDecode(pair.substr(pos + 1));
                params[key] = value;
            }
        }
    }

    std::string urlDecode(const std::string& str) {
        std::string result;
        for (size_t i = 0; i < str.size(); i++) {
            if (str[i] == '%' && i + 2 < str.size()) {
                int value;
                std::istringstream iss(str.substr(i + 1, 2));
                if (iss >> std::hex >> value) {
                    result += static_cast<char>(value);
                    i += 2;
                } else {
                    result += str[i];
                }
            } else if (str[i] == '+') {
                result += ' ';
            } else {
                result += str[i];
            }
        }
        return result;
    }

    Request parseRequest(const std::string& raw) {
        Request req;
        std::istringstream stream(raw);
        std::string line;
        
        // Parse request line
        if (std::getline(stream, line)) {
            std::istringstream lineStream(line);
            lineStream >> req.method >> req.path;
            
            // Parse query string
            size_t queryPos = req.path.find('?');
            if (queryPos != std::string::npos) {
                std::string query = req.path.substr(queryPos + 1);
                req.path = req.path.substr(0, queryPos);
                parseQueryString(query, req.params);
            }
        }

        // Parse headers
        while (std::getline(stream, line) && line != "\r" && !line.empty()) {
            if (line.back() == '\r') line.pop_back();
            size_t pos = line.find(": ");
            if (pos != std::string::npos) {
                req.headers[line.substr(0, pos)] = line.substr(pos + 2);
            }
        }

        return req;
    }

    std::string buildResponse(Response& res) {
        std::ostringstream stream;
        stream << "HTTP/1.1 " << res.status << " OK\r\n";
        res.headers["Content-Length"] = std::to_string(res.body.size());
        res.headers["Connection"] = "close";
        
        for (const auto& header : res.headers) {
            stream << header.first << ": " << header.second << "\r\n";
        }
        stream << "\r\n" << res.body;
        return stream.str();
    }

    void handleClient(SOCKET client_socket) {
        char buffer[8192] = {0};
        int bytes = recv(client_socket, buffer, sizeof(buffer) - 1, 0);
        
        if (bytes > 0) {
            Request req = parseRequest(std::string(buffer, bytes));
            Response res;
            res.set_header("Access-Control-Allow-Origin", "*");
            res.set_header("Access-Control-Allow-Methods", "GET, POST, OPTIONS");
            res.set_header("Access-Control-Allow-Headers", "Content-Type");

            if (req.method == "OPTIONS") {
                res.status = 204;
            } else {
                // Find matching handler
                bool found = false;
                for (const auto& handler : get_handlers) {
                    // Simple path matching (supports basic patterns)
                    std::string pattern = handler.first;
                    if (req.path == pattern || 
                        (pattern.back() == '*' && req.path.find(pattern.substr(0, pattern.size()-1)) == 0)) {
                        handler.second(req, res);
                        found = true;
                        break;
                    }
                }
                
                if (!found) {
                    res.status = 404;
                    res.set_content("{\"error\":\"Not found\"}", "application/json");
                }
            }

            std::string response = buildResponse(res);
            send(client_socket, response.c_str(), (int)response.size(), 0);
        }
        
        closesocket(client_socket);
    }

public:
    Server() {
#ifdef _WIN32
        WSADATA wsaData;
        WSAStartup(MAKEWORD(2, 2), &wsaData);
#endif
    }

    ~Server() {
        stop();
#ifdef _WIN32
        WSACleanup();
#endif
    }

    void Get(const std::string& path, Handler handler) {
        get_handlers[path] = handler;
    }

    bool listen(const std::string& host, int port) {
        server_socket = socket(AF_INET, SOCK_STREAM, 0);
        if (server_socket == INVALID_SOCKET) {
            std::cerr << "Failed to create socket" << std::endl;
            return false;
        }

        int opt = 1;
        setsockopt(server_socket, SOL_SOCKET, SO_REUSEADDR, (const char*)&opt, sizeof(opt));

        sockaddr_in addr;
        addr.sin_family = AF_INET;
        addr.sin_port = htons(port);
        addr.sin_addr.s_addr = INADDR_ANY;

        if (bind(server_socket, (sockaddr*)&addr, sizeof(addr)) == SOCKET_ERROR) {
            std::cerr << "Failed to bind to port " << port << std::endl;
            closesocket(server_socket);
            return false;
        }

        if (::listen(server_socket, SOMAXCONN) == SOCKET_ERROR) {
            std::cerr << "Failed to listen" << std::endl;
            closesocket(server_socket);
            return false;
        }

        std::cout << "Server listening on http://" << host << ":" << port << std::endl;
        running = true;

        while (running) {
            sockaddr_in client_addr;
            int client_len = sizeof(client_addr);
            SOCKET client_socket = accept(server_socket, (sockaddr*)&client_addr, 
#ifdef _WIN32
                &client_len
#else
                (socklen_t*)&client_len
#endif
            );
            
            if (client_socket != INVALID_SOCKET) {
                std::thread(&Server::handleClient, this, client_socket).detach();
            }
        }

        return true;
    }

    void stop() {
        running = false;
        if (server_socket != INVALID_SOCKET) {
            closesocket(server_socket);
            server_socket = INVALID_SOCKET;
        }
    }
};

} // namespace httplib

#endif // HTTPLIB_H

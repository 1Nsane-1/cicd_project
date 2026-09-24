#ifndef HTTP_SERVER_H
#define HTTP_SERVER_H

#include <winsock2.h>
#include <ws2tcpip.h>
#include <iostream>
#include <string>
#include <sstream>
#include <thread>
#include <fstream>
#include <vector>
#include <algorithm>
#include "pipeline.h"

#pragma comment(lib, "ws2_32.lib")

class HttpServer {
    int port;
    SOCKET listen_sock = INVALID_SOCKET;
    PipelineManager pipeline;
    std::vector<SOCKET> sse_clients;
    std::mutex sse_mutex;

    std::string get_mime_type(const std::string& path) {
        if (path.rfind(".html") != std::string::npos) return "text/html";
        if (path.rfind(".css") != std::string::npos) return "text/css";
        if (path.rfind(".js") != std::string::npos) return "application/javascript";
        if (path.rfind(".json") != std::string::npos) return "application/json";
        return "text/plain";
    }

    void broadcast_sse(const std::string& event, const std::string& data) {
        std::lock_guard<std::mutex> lock(sse_mutex);
        std::string msg = "event: " + event + "\ndata: " + data + "\n\n";
        for (auto it = sse_clients.begin(); it != sse_clients.end(); ) {
            if (send(*it, msg.c_str(), (int)msg.size(), 0) == SOCKET_ERROR) {
                closesocket(*it);
                it = sse_clients.erase(it);
            } else {
                ++it;
            }
        }
    }

public:
    explicit HttpServer(int p) : port(p) {}

    void start() {
        WSADATA wsaData;
        WSAStartup(MAKEWORD(2, 2), &wsaData);

        listen_sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
        sockaddr_in service{};
        service.sin_family = AF_INET;
        service.sin_addr.s_addr = inet_addr("127.0.0.1");
        service.sin_port = htons(port);

        bind(listen_sock, (SOCKADDR*)&service, sizeof(service));
        listen(listen_sock, SOMAXCONN);

        std::cout << "[SERVER] CI/CD Pipeline Server running on http://localhost:" << port << std::endl;

        while (true) {
            SOCKET client = accept(listen_sock, NULL, NULL);
            if (client != INVALID_SOCKET) {
                std::thread(&HttpServer::handle_client, this, client).detach();
            }
        }
    }

    void handle_client(SOCKET client) {
        char buffer[4096];
        int bytes = recv(client, buffer, sizeof(buffer) - 1, 0);
        if (bytes <= 0) { closesocket(client); return; }
        buffer[bytes] = '\0';

        std::istringstream iss(buffer);
        std::string method, path, proto;
        iss >> method >> path >> proto;

        // Поиск тела запроса (после \r\n\r\n)
        std::string raw_req(buffer, bytes);
        std::string body;
        size_t body_pos = raw_req.find("\r\n\r\n");
        if (body_pos != std::string::npos) {
            body = raw_req.substr(body_pos + 4);
        }

        // SSE Endpoint
        if (path == "/api/events") {
            std::string header = "HTTP/1.1 200 OK\r\nContent-Type: text/event-stream\r\nCache-Control: no-cache\r\nConnection: keep-alive\r\n\r\n";
            send(client, header.c_str(), (int)header.size(), 0);
            {
                std::lock_guard<std::mutex> lock(sse_mutex);
                sse_clients.push_back(client);
            }
            return; // Соединение остается открытым для SSE
        }

        std::string res_body;
        std::string content_type = "application/json";

        if (method == "POST" && path == "/api/build") {
            pipeline.execute_build("./src", "./bin");
            res_body = "{\"status\":\"ok\"}";
        } else if (method == "POST" && path == "/api/test") {
            pipeline.execute_test();
            res_body = "{\"status\":\"ok\"}";
        } else if (method == "POST" && path == "/api/deploy") {
            pipeline.execute_deploy("./bin", "./deploy");
            res_body = "{\"status\":\"ok\"}";
        } else if (method == "POST" && path == "/api/run") {
            std::thread([this]() {
                pipeline.run_pipeline_file("pipeline.json");
                broadcast_sse("status", pipeline.get_status());
            }).detach();
            res_body = "{\"status\":\"started\"}";
        } else if (method == "GET" && path.find("/api/status") == 0) {
            res_body = "{\"status\":\"" + pipeline.get_status() + "\"}";
        } else if (method == "GET" && path.find("/api/logs") == 0) {
            JsonValue v; v.type = JsonType::String; v.str_val = pipeline.get_logs();
            res_body = "{\"logs\":" + v.serialize() + "}";
        } else {
            // Статические файлы (HTML/CSS/JS)
            std::string file_path = "./web" + path;
            if (path == "/") file_path = "./web/index.html";
            
            std::ifstream f(file_path, std::ios::binary);
            if (f.is_open()) {
                std::string content((std::istreambuf_iterator<char>(f)), std::istreambuf_iterator<char>());
                content_type = get_mime_type(file_path);
                res_body = content;
            } else {
                std::string err = "HTTP/1.1 404 Not Found\r\nContent-Length: 0\r\n\r\n";
                send(client, err.c_str(), (int)err.size(), 0);
                closesocket(client);
                return;
            }
        }

        std::ostringstream response;
        response << "HTTP/1.1 200 OK\r\n"
                 << "Content-Type: " << content_type << "\r\n"
                 << "Content-Length: " << res_body.size() << "\r\n"
                 << "Connection: close\r\n\r\n"
                 << res_body;

        std::string resp_str = response.str();
        send(client, resp_str.c_str(), (int)resp_str.size(), 0);
        closesocket(client);
    }
};

#endif
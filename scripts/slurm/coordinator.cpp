// Z.E.T.A. Slurm Coordinator - Load balancer for GPU farm
//
// Nodes register themselves on startup. Coordinator routes requests
// to available nodes using round-robin or least-loaded strategy.
//
// Build: g++ -std=c++17 -O2 -pthread coordinator.cpp -o coordinator
// Usage: ./coordinator --port 8000
//
// Endpoints:
//   POST /register          - Node registration
//   DELETE /unregister      - Node removal
//   GET /nodes              - List active nodes
//   POST /v1/chat/completions - Proxied to nodes
//   GET /health             - Coordinator health

#include <iostream>
#include <string>
#include <vector>
#include <map>
#include <mutex>
#include <thread>
#include <chrono>
#include <atomic>
#include <sstream>
#include <cstring>
#include <cstdlib>
#include <ctime>
#include <algorithm>

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <unistd.h>
#include <poll.h>

// ----------------------------------------------------------------------------
// Node Management
// ----------------------------------------------------------------------------

struct Node {
    std::string host;
    int         port;
    std::string job_id;
    int         task_id;
    time_t      registered_at;
    time_t      last_health;
    std::atomic<int> active_requests{0};
    std::atomic<int> total_requests{0};
    std::atomic<bool> healthy{true};

    std::string key() const { return host + ":" + std::to_string(port); }
    std::string url() const { return "http://" + host + ":" + std::to_string(port); }
};

class NodePool {
public:
    std::map<std::string, std::shared_ptr<Node>> nodes;
    std::mutex mtx;
    std::atomic<size_t> round_robin_idx{0};

    std::shared_ptr<Node> register_node(const std::string& host, int port,
                                        const std::string& job_id = "",
                                        int task_id = 0) {
        std::lock_guard<std::mutex> lock(mtx);
        auto node = std::make_shared<Node>();
        node->host = host;
        node->port = port;
        node->job_id = job_id;
        node->task_id = task_id;
        node->registered_at = time(nullptr);
        node->last_health = time(nullptr);
        nodes[node->key()] = node;
        std::cerr << "[COORD] Registered: " << node->key() << " (job=" << job_id << ")\n";
        return node;
    }

    bool unregister_node(const std::string& host, int port) {
        std::lock_guard<std::mutex> lock(mtx);
        std::string key = host + ":" + std::to_string(port);
        auto it = nodes.find(key);
        if (it != nodes.end()) {
            nodes.erase(it);
            std::cerr << "[COORD] Unregistered: " << key << "\n";
            return true;
        }
        return false;
    }

    std::vector<std::shared_ptr<Node>> get_healthy() {
        std::lock_guard<std::mutex> lock(mtx);
        std::vector<std::shared_ptr<Node>> result;
        for (auto& [k, n] : nodes) {
            if (n->healthy) result.push_back(n);
        }
        return result;
    }

    std::shared_ptr<Node> select_least_loaded() {
        auto healthy = get_healthy();
        if (healthy.empty()) return nullptr;
        return *std::min_element(healthy.begin(), healthy.end(),
            [](const auto& a, const auto& b) {
                return a->active_requests.load() < b->active_requests.load();
            });
    }

    std::shared_ptr<Node> select_round_robin() {
        auto healthy = get_healthy();
        if (healthy.empty()) return nullptr;
        size_t idx = round_robin_idx.fetch_add(1) % healthy.size();
        return healthy[idx];
    }

    std::string to_json() {
        std::lock_guard<std::mutex> lock(mtx);
        std::ostringstream ss;
        ss << "{\"nodes\":[";
        bool first = true;
        for (auto& [k, n] : nodes) {
            if (!first) ss << ",";
            first = false;
            ss << "{\"host\":\"" << n->host << "\","
               << "\"port\":" << n->port << ","
               << "\"job_id\":\"" << n->job_id << "\","
               << "\"healthy\":" << (n->healthy ? "true" : "false") << ","
               << "\"active_requests\":" << n->active_requests.load() << ","
               << "\"total_requests\":" << n->total_requests.load() << "}";
        }
        ss << "],\"total\":" << nodes.size() << "}";
        return ss.str();
    }

    void remove_stale(int timeout_sec = 300) {
        std::lock_guard<std::mutex> lock(mtx);
        time_t now = time(nullptr);
        for (auto it = nodes.begin(); it != nodes.end(); ) {
            if (now - it->second->last_health > timeout_sec) {
                std::cerr << "[COORD] Removing stale: " << it->first << "\n";
                it = nodes.erase(it);
            } else {
                ++it;
            }
        }
    }
};

static NodePool g_pool;

// ----------------------------------------------------------------------------
// Simple HTTP Helpers
// ----------------------------------------------------------------------------

std::string http_response(int status, const std::string& body,
                          const std::string& content_type = "application/json") {
    std::ostringstream ss;
    ss << "HTTP/1.1 " << status << " OK\r\n"
       << "Content-Type: " << content_type << "\r\n"
       << "Content-Length: " << body.size() << "\r\n"
       << "Connection: close\r\n"
       << "\r\n"
       << body;
    return ss.str();
}

std::string parse_json_string(const std::string& json, const std::string& key) {
    std::string search = "\"" + key + "\":";
    size_t pos = json.find(search);
    if (pos == std::string::npos) return "";
    pos += search.size();
    while (pos < json.size() && (json[pos] == ' ' || json[pos] == '"')) pos++;
    if (json[pos-1] != '"') return "";
    size_t end = json.find('"', pos);
    if (end == std::string::npos) return "";
    return json.substr(pos, end - pos);
}

int parse_json_int(const std::string& json, const std::string& key) {
    std::string search = "\"" + key + "\":";
    size_t pos = json.find(search);
    if (pos == std::string::npos) return 0;
    pos += search.size();
    while (pos < json.size() && json[pos] == ' ') pos++;
    return std::atoi(json.c_str() + pos);
}

// ----------------------------------------------------------------------------
// HTTP Proxy (forward to node)
// ----------------------------------------------------------------------------

std::string proxy_request(std::shared_ptr<Node> node,
                         const std::string& method,
                         const std::string& path,
                         const std::string& headers,
                         const std::string& body) {
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) return http_response(502, "{\"error\":\"socket failed\"}");

    struct hostent* host = gethostbyname(node->host.c_str());
    if (!host) {
        close(sock);
        return http_response(502, "{\"error\":\"host lookup failed\"}");
    }

    struct sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(node->port);
    memcpy(&addr.sin_addr, host->h_addr_list[0], host->h_length);

    // Set timeout
    struct timeval tv;
    tv.tv_sec = 300;
    tv.tv_usec = 0;
    setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));
    setsockopt(sock, SOL_SOCKET, SO_SNDTIMEO, &tv, sizeof(tv));

    if (connect(sock, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
        close(sock);
        node->healthy = false;
        return http_response(502, "{\"error\":\"connect failed\"}");
    }

    // Build request
    std::ostringstream req;
    req << method << " " << path << " HTTP/1.1\r\n"
        << "Host: " << node->host << ":" << node->port << "\r\n"
        << "Content-Length: " << body.size() << "\r\n"
        << "Content-Type: application/json\r\n"
        << "Connection: close\r\n"
        << "\r\n"
        << body;

    std::string request = req.str();
    send(sock, request.c_str(), request.size(), 0);

    // Read response
    std::string response;
    char buf[8192];
    ssize_t n;
    while ((n = recv(sock, buf, sizeof(buf), 0)) > 0) {
        response.append(buf, n);
    }
    close(sock);

    if (response.empty()) {
        node->healthy = false;
        return http_response(502, "{\"error\":\"empty response\"}");
    }

    node->last_health = time(nullptr);
    return response;
}

// ----------------------------------------------------------------------------
// Request Handler
// ----------------------------------------------------------------------------

void handle_client(int client_sock) {
    char buf[65536];
    ssize_t n = recv(client_sock, buf, sizeof(buf) - 1, 0);
    if (n <= 0) {
        close(client_sock);
        return;
    }
    buf[n] = '\0';
    std::string request(buf);

    // Parse request line
    std::string method, path;
    std::istringstream iss(request);
    iss >> method >> path;

    // Find body
    std::string body;
    size_t body_pos = request.find("\r\n\r\n");
    if (body_pos != std::string::npos) {
        body = request.substr(body_pos + 4);
    }

    std::string response;

    // Route handlers
    if (method == "POST" && path == "/register") {
        std::string host = parse_json_string(body, "host");
        int port = parse_json_int(body, "port");
        std::string job_id = parse_json_string(body, "job_id");
        int task_id = parse_json_int(body, "task_id");
        g_pool.register_node(host, port, job_id, task_id);
        response = http_response(200, "{\"status\":\"registered\"}");
    }
    else if (method == "DELETE" && path == "/unregister") {
        std::string host = parse_json_string(body, "host");
        int port = parse_json_int(body, "port");
        bool ok = g_pool.unregister_node(host, port);
        response = http_response(200, ok ? "{\"status\":\"unregistered\"}" : "{\"status\":\"not_found\"}");
    }
    else if (method == "GET" && path == "/nodes") {
        response = http_response(200, g_pool.to_json());
    }
    else if (method == "GET" && path == "/health") {
        auto healthy = g_pool.get_healthy();
        std::ostringstream ss;
        ss << "{\"status\":\"ok\",\"nodes_total\":" << g_pool.nodes.size()
           << ",\"nodes_healthy\":" << healthy.size() << "}";
        response = http_response(200, ss.str());
    }
    else if (method == "GET" && path == "/v1/models") {
        response = http_response(200,
            "{\"object\":\"list\",\"data\":["
            "{\"id\":\"zeta-cognitive\",\"object\":\"model\",\"owned_by\":\"zeta\"},"
            "{\"id\":\"zeta-coder\",\"object\":\"model\",\"owned_by\":\"zeta\"}"
            "]}");
    }
    else if (path.rfind("/v1/", 0) == 0 || path.rfind("/zeta/", 0) == 0) {
        // Proxy to node
        auto node = g_pool.select_least_loaded();
        if (!node) {
            response = http_response(503, "{\"error\":\"no healthy nodes\"}");
        } else {
            node->active_requests++;
            node->total_requests++;
            response = proxy_request(node, method, path, "", body);
            node->active_requests--;
        }
    }
    else {
        response = http_response(404, "{\"error\":\"not found\"}");
    }

    send(client_sock, response.c_str(), response.size(), 0);
    close(client_sock);
}

// ----------------------------------------------------------------------------
// Health Check Thread
// ----------------------------------------------------------------------------

void health_check_loop() {
    while (true) {
        std::this_thread::sleep_for(std::chrono::seconds(30));

        auto nodes = g_pool.get_healthy();
        for (auto& node : nodes) {
            int sock = socket(AF_INET, SOCK_STREAM, 0);
            if (sock < 0) continue;

            struct timeval tv;
            tv.tv_sec = 5;
            tv.tv_usec = 0;
            setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));
            setsockopt(sock, SOL_SOCKET, SO_SNDTIMEO, &tv, sizeof(tv));

            struct hostent* host = gethostbyname(node->host.c_str());
            if (!host) {
                node->healthy = false;
                close(sock);
                continue;
            }

            struct sockaddr_in addr{};
            addr.sin_family = AF_INET;
            addr.sin_port = htons(node->port);
            memcpy(&addr.sin_addr, host->h_addr_list[0], host->h_length);

            if (connect(sock, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
                node->healthy = false;
            } else {
                std::string req = "GET /health HTTP/1.1\r\nHost: " + node->host + "\r\nConnection: close\r\n\r\n";
                send(sock, req.c_str(), req.size(), 0);
                char buf[1024];
                ssize_t n = recv(sock, buf, sizeof(buf), 0);
                node->healthy = (n > 0 && strstr(buf, "200") != nullptr);
                if (node->healthy) node->last_health = time(nullptr);
            }
            close(sock);
        }

        // Remove stale nodes
        g_pool.remove_stale(300);
    }
}

// ----------------------------------------------------------------------------
// Main
// ----------------------------------------------------------------------------

int main(int argc, char* argv[]) {
    int port = 8000;
    const char* host = "0.0.0.0";

    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--port") == 0 && i + 1 < argc) {
            port = atoi(argv[++i]);
        } else if (strcmp(argv[i], "--host") == 0 && i + 1 < argc) {
            host = argv[++i];
        }
    }

    int server_sock = socket(AF_INET, SOCK_STREAM, 0);
    if (server_sock < 0) {
        perror("socket");
        return 1;
    }

    int opt = 1;
    setsockopt(server_sock, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    struct sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    inet_pton(AF_INET, host, &addr.sin_addr);

    if (bind(server_sock, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
        perror("bind");
        return 1;
    }

    if (listen(server_sock, 128) < 0) {
        perror("listen");
        return 1;
    }

    std::cerr << "[COORD] Z.E.T.A. Coordinator starting on " << host << ":" << port << "\n";

    // Start health check thread
    std::thread health_thread(health_check_loop);
    health_thread.detach();

    while (true) {
        struct sockaddr_in client_addr;
        socklen_t client_len = sizeof(client_addr);
        int client_sock = accept(server_sock, (struct sockaddr*)&client_addr, &client_len);
        if (client_sock < 0) continue;

        std::thread(handle_client, client_sock).detach();
    }

    return 0;
}

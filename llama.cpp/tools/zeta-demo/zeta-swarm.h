// Z.E.T.A. Swarm Intelligence
// Distributed Consensus & Task Offloading for Jetson Orin Clusters
//
// Capabilities:
// 1. Distributed Dreaming: Offload dream generation to idle nodes
// 2. Swarm Consensus: Ternary voting (True/False/Uncertain) on complex facts
// 3. Shared Memory: (Future) Distributed graph storage
//
// Z.E.T.A.(TM) | Patent Pending | (C) 2025

#ifndef ZETA_SWARM_H
#define ZETA_SWARM_H

#include <string>
#include <vector>
#include <mutex>
#include <thread>
#include <atomic>
#include <map>
#include <chrono>
#include <algorithm>
#include <cpp-httplib/httplib.h>

// Ternary Vector for Consensus
struct ZetaTernaryVector {
    float t; // True
    float f; // False
    float u; // Uncertain
};

// Swarm Node Definition
struct ZetaSwarmNode {
    std::string id;
    std::string host; // IP or Hostname
    int port;
    bool is_active;
    time_t last_seen;
    float current_load;
    std::string role; // "worker", "critic", "dreamer"
};

class ZetaSwarmManager {
private:
    std::vector<ZetaSwarmNode> nodes;
    std::mutex nodes_mutex;
    std::atomic<bool> running{false};
    std::thread health_thread;
    
    // Voting storage: proposal_id -> (voter_id -> vote)
    std::map<std::string, std::map<std::string, int>> votes;
    std::mutex votes_mutex;

    // Helper to send HTTP request to a node
    std::string send_request(const ZetaSwarmNode& node, const std::string& endpoint, const std::string& payload) {
        httplib::Client cli(node.host, node.port);
        cli.set_connection_timeout(2); // Fast fail
        cli.set_read_timeout(60);      // Allow generation time (longer for offloading)
        
        auto res = cli.Post(endpoint, payload, "application/json");
        if (res && res->status == 200) {
            return res->body;
        }
        return "";
    }

public:
    ZetaSwarmManager() {}
    
    ~ZetaSwarmManager() {
        stop();
    }

    void start() {
        if (running) return;
        running = true;
        health_thread = std::thread(&ZetaSwarmManager::health_monitor, this);
        fprintf(stderr, "[SWARM] Swarm Manager started.\n");
    }

    void stop() {
        running = false;
        if (health_thread.joinable()) health_thread.join();
    }

    // Register or update a node
    void register_node(const std::string& id, const std::string& host, int port) {
        std::lock_guard<std::mutex> lock(nodes_mutex);
        for (auto& n : nodes) {
            if (n.id == id) {
                n.host = host;
                n.port = port;
                n.is_active = true;
                n.last_seen = time(NULL);
                return;
            }
        }
        
        ZetaSwarmNode node;
        node.id = id;
        node.host = host;
        node.port = port;
        node.is_active = true;
        node.last_seen = time(NULL);
        node.current_load = 0.0f;
        node.role = "worker"; // Default
        nodes.push_back(node);
    }

    void update_heartbeat(const std::string& id, float load) {
        std::lock_guard<std::mutex> lock(nodes_mutex);
        for (auto& n : nodes) {
            if (n.id == id) {
                n.is_active = true;
                n.last_seen = time(NULL);
                n.current_load = load;
                return;
            }
        }
    }

    std::vector<ZetaSwarmNode> get_active_nodes() {
        std::lock_guard<std::mutex> lock(nodes_mutex);
        std::vector<ZetaSwarmNode> active;
        time_t now = time(NULL);
        for (const auto& n : nodes) {
            if (n.is_active && (now - n.last_seen < 60)) { // 60s timeout
                active.push_back(n);
            }
        }
        return active;
    }

    // Health Monitor Thread
    void health_monitor() {
        while (running) {
            std::this_thread::sleep_for(std::chrono::seconds(10));
            std::lock_guard<std::mutex> lock(nodes_mutex);
            time_t now = time(NULL);
            for (auto& node : nodes) {
                if (now - node.last_seen > 60) {
                    if (node.is_active) {
                        fprintf(stderr, "[SWARM] Node lost: %s (last seen %lds ago)\n", 
                                node.id.c_str(), (long)(now - node.last_seen));
                    }
                    node.is_active = false;
                }
            }
        }
    }

    // Submit a vote for a proposal
    void submit_vote(const std::string& proposal_id, const std::string& voter_id, int vote) {
        std::lock_guard<std::mutex> lock(votes_mutex);
        votes[proposal_id][voter_id] = vote;
    }

    // Swarm Consensus: Ask all nodes to evaluate a statement
    ZetaTernaryVector get_consensus(const std::string& statement) {
        ZetaTernaryVector consensus = {0.0f, 0.0f, 0.0f};
        std::vector<std::future<int>> futures;
        
        // Payload for the nodes
        std::string prompt = "{\"prompt\": \"[SYSTEM] Evaluate the truth of this statement. Reply ONLY with 'TRUE', 'FALSE', or 'UNCERTAIN'. Statement: " + statement + "\", \"max_tokens\": 10}";

        std::vector<ZetaSwarmNode> active_nodes = get_active_nodes();
        
        for (const auto& node : active_nodes) {
            // Launch async request
            futures.push_back(std::async(std::launch::async, [this, node, prompt]() {
                std::string response = const_cast<ZetaSwarmManager*>(this)->send_request(node, "/generate", prompt);
                // Parse response
                if (response.find("TRUE") != std::string::npos) return 1;
                if (response.find("FALSE") != std::string::npos) return -1;
                return 0; // Uncertain or error
            }));
        }

        // Collect results
        int active_count = 0;
        for (auto& f : futures) {
            int vote = f.get();
            if (vote == 1) consensus.t += 1.0f;
            else if (vote == -1) consensus.f += 1.0f;
            else consensus.u += 1.0f;
            active_count++;
        }

        // Normalize
        if (active_count > 0) {
            consensus.t /= active_count;
            consensus.f /= active_count;
            consensus.u /= active_count;
        }

        return consensus;
    }

    // Offload Dream: Ask a specific node to generate a dream
    std::string offload_dream_task(const std::string& node_id, const std::string& prompt) {
        ZetaSwarmNode target;
        bool found = false;
        
        {
            std::lock_guard<std::mutex> lock(nodes_mutex);
            for (const auto& n : nodes) {
                if (n.id == node_id) {
                    target = n;
                    found = true;
                    break;
                }
            }
        }

        if (!found) return "";

        std::string payload = "{\"prompt\": \"" + prompt + "\", \"max_tokens\": 512}";
        return send_request(target, "/generate", payload);
    }
};

#endif // ZETA_SWARM_H

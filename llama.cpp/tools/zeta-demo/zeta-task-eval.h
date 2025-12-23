#ifndef ZETA_TASK_EVAL_H
#define ZETA_TASK_EVAL_H

#include <string>
#include <map>
#include <vector>
#include <iostream>
#include <random>

// Task Evaluator
// Dynamically allocates resources between HRM and TRM based on task type
class ZetaTaskEvaluator {
public:
    struct TaskScores {
        float hrm_score; // Efficiency for HRM
        float trm_score; // Efficiency for TRM
    };

    ZetaTaskEvaluator() {
        // Initialize with default efficiencies based on dream
        efficiency_map["decision-making"] = {0.8f, 0.6f};
        efficiency_map["learning"] = {0.7f, 0.9f};
        efficiency_map["problem-solving"] = {0.9f, 0.8f};
        efficiency_map["creative"] = {0.95f, 0.4f};
        efficiency_map["temporal"] = {0.2f, 0.95f};
    }

    // Evaluate task and return recommended component ("HRM", "TRM", or "BOTH")
    std::string evaluate_task(const std::string& task_type, float complexity) {
        if (efficiency_map.find(task_type) == efficiency_map.end()) {
            return "BOTH"; // Default to both if unknown
        }

        TaskScores scores = efficiency_map[task_type];
        
        float hrm_prob = scores.hrm_score;
        float trm_prob = scores.trm_score;

        // Adjust for complexity (simple heuristic)
        // High complexity might favor using BOTH
        if (complexity > 0.8f) return "BOTH";

        // Simple logic: if one is significantly better, pick it. If close, pick BOTH.
        if (hrm_prob > trm_prob + 0.2f) return "HRM";
        if (trm_prob > hrm_prob + 0.2f) return "TRM";
        
        return "BOTH";
    }

    void update_efficiency(const std::string& task_type, bool hrm_success, bool trm_success) {
        // Reinforcement learning stub
        if (efficiency_map.find(task_type) != efficiency_map.end()) {
            if (hrm_success) efficiency_map[task_type].hrm_score += 0.01f;
            else efficiency_map[task_type].hrm_score -= 0.01f;

            if (trm_success) efficiency_map[task_type].trm_score += 0.01f;
            else efficiency_map[task_type].trm_score -= 0.01f;
            
            // Clamp values
            if (efficiency_map[task_type].hrm_score > 1.0f) efficiency_map[task_type].hrm_score = 1.0f;
            if (efficiency_map[task_type].hrm_score < 0.0f) efficiency_map[task_type].hrm_score = 0.0f;
            if (efficiency_map[task_type].trm_score > 1.0f) efficiency_map[task_type].trm_score = 1.0f;
            if (efficiency_map[task_type].trm_score < 0.0f) efficiency_map[task_type].trm_score = 0.0f;
        }
    }

private:
    std::map<std::string, TaskScores> efficiency_map;
};

#endif // ZETA_TASK_EVAL_H

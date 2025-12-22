#ifndef ZETA_TERNARY_H
#define ZETA_TERNARY_H

#include <vector>
#include <string>
#include <cmath>
#include <algorithm>
#include <iostream>

// ============================================================================
// Z.E.T.A. Ternary Computation Emulation
// ============================================================================
// Implements Balanced Ternary Logic (-1, 0, +1)
//
// Values:
//   +1 (TRUE / POSITIVE / CORROBORATION)
//    0 (UNKNOWN / NEUTRAL / IRRELEVANT)
//   -1 (FALSE / NEGATIVE / CONTRADICTION)
//
// This is superior to binary logic for AI reasoning because it explicitly
// models "Unknown" and "Contradiction" states, which are critical for
// robust knowledge graph operations.
// ============================================================================

namespace ZetaTernary {

    // The fundamental unit: Trit
    enum class Trit : int8_t {
        False = -1,
        Unknown = 0,
        True = 1
    };

    // ========================================================================
    // Basic Gates (Kleene Logic / Balanced Ternary)
    // ========================================================================

    // NOT: Inverts the truth value
    // +1 -> -1
    //  0 ->  0
    // -1 -> +1
    static inline Trit NOT(Trit t) {
        return static_cast<Trit>(-static_cast<int>(t));
    }

    // AND: Minimum value (Strong consensus required)
    // min(T, F) = F
    // min(T, U) = U
    static inline Trit AND(Trit a, Trit b) {
        return static_cast<Trit>(std::min(static_cast<int>(a), static_cast<int>(b)));
    }

    // OR: Maximum value (Optimistic)
    // max(T, F) = T
    // max(F, U) = U
    static inline Trit OR(Trit a, Trit b) {
        return static_cast<Trit>(std::max(static_cast<int>(a), static_cast<int>(b)));
    }

    // IMPLIES: A -> B  (equivalent to max(-A, B))
    static inline Trit IMPLIES(Trit a, Trit b) {
        return OR(NOT(a), b);
    }

    // ========================================================================
    // Advanced Ternary Operations for AI
    // ========================================================================

    // CONSENSUS: Merges two opinions
    // If they agree, return value.
    // If one is Unknown, return the other.
    // If they disagree (T vs F), return Unknown (Conflict).
    static inline Trit CONSENSUS(Trit a, Trit b) {
        if (a == b) return a;
        if (a == Trit::Unknown) return b;
        if (b == Trit::Unknown) return a;
        return Trit::Unknown; // Conflict (+1 vs -1) -> 0
    }

    // CONFIDENCE: Returns 1 if known (T/F), 0 if Unknown
    static inline int CONFIDENCE(Trit t) {
        return std::abs(static_cast<int>(t));
    }

    // ========================================================================
    // Ternary Vector (TritVector)
    // ========================================================================
    // Used for "Semantic Fingerprints" where dimensions are concepts.
    // +1 = Concept Applies
    // -1 = Concept Does Not Apply
    //  0 = Irrelevant/Unknown
    
    class TritVector {
    public:
        std::vector<Trit> data;

        TritVector(size_t size, Trit init = Trit::Unknown) : data(size, init) {}

        // Convert float embedding to TritVector using thresholds
        // > threshold -> +1
        // < -threshold -> -1
        // else -> 0
        static TritVector from_float(const std::vector<float>& vec, float threshold = 0.1f) {
            TritVector tv(vec.size());
            for (size_t i = 0; i < vec.size(); i++) {
                if (vec[i] > threshold) tv.data[i] = Trit::True;
                else if (vec[i] < -threshold) tv.data[i] = Trit::False;
                else tv.data[i] = Trit::Unknown;
            }
            return tv;
        }

        // Dot Product (Similarity)
        // Matches (+1/+1 or -1/-1) add 1
        // Mismatches (+1/-1) subtract 1
        // Unknowns (0) contribute 0
        int dot(const TritVector& other) const {
            if (data.size() != other.data.size()) return 0;
            int sum = 0;
            for (size_t i = 0; i < data.size(); i++) {
                sum += static_cast<int>(data[i]) * static_cast<int>(other.data[i]);
            }
            return sum;
        }

        // Hamming Distance (Logic Distance)
        // Count of positions where values differ
        int distance(const TritVector& other) const {
            if (data.size() != other.data.size()) return -1;
            int dist = 0;
            for (size_t i = 0; i < data.size(); i++) {
                if (data[i] != other.data[i]) dist++;
            }
            return dist;
        }
        
        // Merge two vectors using Consensus
        TritVector merge(const TritVector& other) const {
            if (data.size() != other.data.size()) return *this;
            TritVector result(data.size());
            for (size_t i = 0; i < data.size(); i++) {
                result.data[i] = CONSENSUS(data[i], other.data[i]);
            }
            return result;
        }
        
        std::string to_string() const {
            std::string s = "[";
            for (const auto& t : data) {
                switch(t) {
                    case Trit::True: s += "+"; break;
                    case Trit::False: s += "-"; break;
                    case Trit::Unknown: s += "0"; break;
                }
            }
            s += "]";
            return s;
        }
    };

    // ========================================================================
    // Ternary Decision Tree Node
    // ========================================================================
    struct TernaryNode {
        std::string question;
        Trit value; // If leaf, this is the decision
        bool is_leaf;
        
        TernaryNode* left;   // Path for -1 (False)
        TernaryNode* middle; // Path for 0 (Unknown)
        TernaryNode* right;  // Path for +1 (True)

        TernaryNode(std::string q) : question(q), value(Trit::Unknown), is_leaf(false), left(nullptr), middle(nullptr), right(nullptr) {}
        TernaryNode(Trit v) : question(""), value(v), is_leaf(true), left(nullptr), middle(nullptr), right(nullptr) {}
    };

} // namespace ZetaTernary

#endif // ZETA_TERNARY_H

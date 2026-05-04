#pragma once
#include <bits/stdc++.h>
using namespace std;

//  ALGORITHM 1 — MIN-HEAP (PRIORITY QUEUE)

struct BallEvent {
    int    over;          // which over (0-indexed)
    int    ballInOver;    // which ball (1 to 6)
    string type;          // "DOT", "RUN", "FOUR", "SIX", "WICKET", "WIDE", "NOBALL", "BYE", "LEGBYE"
    string batsmanId;
    string bowlerId;
    int    runsScored;    // runs credited to the batsman
    int    extras;        // extras (wides, no-balls, byes, leg-byes)
    string wicketType;    // "BOWLED", "CAUGHT", "LBW", "RUN_OUT", "STUMPED", "HIT_WICKET"
    string fielder;       // name of fielder involved (for catches, run-outs, stumpings)
    string commentary;    // human-readable description of the ball
    bool operator>(const BallEvent& other) const {
        if (over != other.over) {
            return over > other.over;
        }
        return ballInOver > other.ballInOver;
    }
};

class BallQueue {
private:
    priority_queue<BallEvent, vector<BallEvent>, greater<BallEvent>> heap;

public:
    void push(const BallEvent& ball) {
        heap.push(ball);
    }

    BallEvent pop() {
        if (heap.empty()) {
            throw runtime_error("Cannot pop from an empty ball queue.");
        }
        BallEvent top = heap.top();
        heap.pop();
        return top;
    }

    bool isEmpty() const {
        return heap.empty();
    }

    int size() const {
        return (int)heap.size();
    }
};

//  ALGORITHM 2 — SEGMENT TREE


class SegmentTree {
private:
    int         capacity;   // number of leaves (overs we can track)
    vector<int> tree;       // internal tree nodes, size = 4 * capacity

    void buildTree(vector<int>& data, int node, int start, int end) {
        if (start == end) {
            // Leaf node: just store the value at this index
            tree[node] = data[start];
            return;
        }

        int mid        = (start + end) / 2;
        int leftChild  = 2 * node;
        int rightChild = 2 * node + 1;

        buildTree(data, leftChild,  start, mid);
        buildTree(data, rightChild, mid + 1, end);
        tree[node] = tree[leftChild] + tree[rightChild];
    }

    void updateTree(int node, int start, int end, int index, int value) {
        if (start == end) {
            tree[node] += value;
            return;
        }

        int mid        = (start + end) / 2;
        int leftChild  = 2 * node;
        int rightChild = 2 * node + 1;

        if (index <= mid) {
            updateTree(leftChild,  start, mid,     index, value);
        } else {
            updateTree(rightChild, mid + 1, end,   index, value);
        }
        tree[node] = tree[leftChild] + tree[rightChild];
    }

    int queryTree(int node, int start, int end, int queryLeft, int queryRight) {
        
        if (queryRight < start || end < queryLeft) {
            return 0;
        }
        if (queryLeft <= start && end <= queryRight) {
            return tree[node];
        }
        int mid        = (start + end) / 2;
        int leftChild  = 2 * node;
        int rightChild = 2 * node + 1;

        int leftSum  = queryTree(leftChild,  start,   mid, queryLeft, queryRight);
        int rightSum = queryTree(rightChild, mid + 1, end, queryLeft, queryRight);

        return leftSum + rightSum;
    }

public:
    SegmentTree(int size) : capacity(size), tree(4 * size, 0) {}
    void update(int index, int value) {
        if (index < 0 || index >= capacity) {
            return;  
        }
        updateTree(1, 0, capacity - 1, index, value);
    }
    int query(int left, int right) {
        if (left > right || left < 0 || right >= capacity) {
            return 0;
        }
        return queryTree(1, 0, capacity - 1, left, right);
    }
};


//  ALGORITHM 3 — FENWICK TREE (Binary Indexed Tree)

class FenwickTree {
private:
    int         capacity;
    vector<int> data;  

public:
    FenwickTree(int size) : capacity(size), data(size + 2, 0) {}
    void update(int index, int delta) {
        int pos = index + 1;
        while (pos <= capacity) {
            data[pos] += delta;
            pos = pos + (pos & (-pos));  
        }
    }

    // Return the sum of all values from position 0 up to 'index' (inclusive)
    int prefixSum(int index) {
        int pos    = index + 1;   // convert to 1-indexed
        int total  = 0;

        // Move down the tree, collecting partial sums
        while (pos > 0) {
            total = total + data[pos];
            pos   = pos - (pos & (-pos));   // move to parent node
        }

        return total;
    }

    // Return the sum in range [left, right] (0-indexed)
    int rangeSum(int left, int right) {
        if (left > right) {
            return 0;
        }
        if (left == 0) {
            return prefixSum(right);
        }
        return prefixSum(right) - prefixSum(left - 1);
    }
};


//  ALGORITHM 4 — TRIE (Prefix Tree)

struct TrieNode {
    // Each child maps one character to the next TrieNode
    unordered_map<char, TrieNode*> children;

    // True if a complete player name ends exactly at this node
    bool isEndOfName;

    // All player IDs whose name ends at or passes through this node
    vector<string> playerIds;

    TrieNode() : isEndOfName(false) {}
};

class Trie {
private:
    TrieNode* root;

    // Helper: collect all player IDs reachable from 'node'
    void collectAllBelow(TrieNode* node, vector<string>& results) {
        if (node->isEndOfName) {
            for (string& id : node->playerIds) {
                results.push_back(id);
            }
        }

        for (auto& pair : node->children) {
            collectAllBelow(pair.second, results);
        }
    }

    // Helper: recursively delete all nodes (called in destructor)
    void deleteAll(TrieNode* node) {
        for (auto& pair : node->children) {
            deleteAll(pair.second);
        }
        delete node;
    }

public:
    Trie() : root(new TrieNode()) {}

    ~Trie() {
        deleteAll(root);
    }

    // Insert a player's full name, linked to their ID
    void insert(const string& fullName, const string& playerId) {
        TrieNode* current = root;

        for (char ch : fullName) {
            char lower = tolower(ch);

            // Create the child node if it doesn't exist yet
            if (current->children.find(lower) == current->children.end()) {
                current->children[lower] = new TrieNode();
            }

            current = current->children[lower];
        }

        // Mark the end of this name and store the player ID
        current->isEndOfName = true;
        current->playerIds.push_back(playerId);
    }

    // Return all player IDs whose name starts with 'prefix'
    vector<string> searchByPrefix(const string& prefix) {
        TrieNode* current = root;

        // Walk down the trie following each character of the prefix
        for (char ch : prefix) {
            char lower = tolower(ch);

            if (current->children.find(lower) == current->children.end()) {
                // Prefix not found — no matches
                return {};
            }

            current = current->children[lower];
        }

        // Collect all names that continue from this node
        vector<string> results;
        collectAllBelow(current, results);
        return results;
    }
};


//  ALGORITHM 5 — KMP STRING MATCHING


class KMP {
public:
    static vector<int> buildLPSTable(const string& pattern) {
        int length = pattern.size();
        vector<int>  lps(length, 0);
        int   prefixLen = 0;
        int   i = 1;

        while (i < length) {
            if (pattern[i] == pattern[prefixLen]) {
                prefixLen++;
                lps[i] = prefixLen;
                i++;
            } else if (prefixLen > 0) {
                prefixLen = lps[prefixLen - 1];
            } else {
                lps[i] = 0;
                i++;
            }
        }

        return lps;
    }
    static vector<int> search(const string& text, const string& pattern) {
        if (pattern.empty()) {
            return {};
        }

        string lowerText    = text;
        string lowerPattern = pattern;
        transform(lowerText.begin(),    lowerText.end(),    lowerText.begin(),    ::tolower);
        transform(lowerPattern.begin(), lowerPattern.end(), lowerPattern.begin(), ::tolower);

        vector<int> lps     = buildLPSTable(lowerPattern);
        vector<int> matches;

        int textIdx    = 0;
        int patternIdx = 0;

        while (textIdx < (int)lowerText.size()) {
            if (lowerText[textIdx] == lowerPattern[patternIdx]) {
                // Characters match — advance both pointers
                textIdx++;
                patternIdx++;
            }

            if (patternIdx == (int)lowerPattern.size()) {
                // Full pattern matched
                matches.push_back(textIdx - patternIdx);
                // Use LPS to find the next possible match efficiently
                patternIdx = lps[patternIdx - 1];

            } else if (textIdx < (int)lowerText.size()
                    && lowerText[textIdx] != lowerPattern[patternIdx]) {

                if (patternIdx != 0) {
                    // Fall back in pattern using LPS
                    patternIdx = lps[patternIdx - 1];
                } else {
                    // No fallback — advance in text
                    textIdx++;
                }
            }
        }

        return matches;
    }
};

//  ALGORITHM 6 & 7 — GRAPH + DIJKSTRA + BFS + DFS

class PartnershipGraph {
private:
    // Adjacency list: player ID → list of (partner ID, runs together)
    unordered_map<string, vector<pair<string, int>>> adjacency;

    // Tracks all unique node names (for DFS cluster detection)
    unordered_set<string> allNodes;

public:
    // Add or update the partnership edge between two batsmen
    void addPartnership(const string& playerA, const string& playerB, int runs) {
        if (runs <= 0) {
            return;
        }
        allNodes.insert(playerA);
        allNodes.insert(playerB);

        // Check if an edge already exists between A and B
        bool foundAtoB = false;
        for (auto& edge : adjacency[playerA]) {
            if (edge.first == playerB) {
                edge.second = edge.second + runs;
                foundAtoB   = true;
                break;
            }
        }
        if (!foundAtoB) {
            adjacency[playerA].push_back({playerB, runs});
        }

        // Check the reverse direction too (undirected graph)
        bool foundBtoA = false;
        for (auto& edge : adjacency[playerB]) {
            if (edge.first == playerA) {
                edge.second = edge.second + runs;
                foundBtoA   = true;
                break;
            }
        }
        if (!foundBtoA) {
            adjacency[playerB].push_back({playerA, runs});
        }
    }

    // ---- ALGORITHM 6: Dijkstra ----

    pair<int, vector<string>> dijkstra(const string& source, const string& destination) {
        // dist[node] = best (lowest cost) path found so far to that node
        unordered_map<string, double> dist;
        unordered_map<string, string> previousNode;

        // Initialise all distances to infinity
        for (const string& node : allNodes) {
            dist[node] = 1e18;
        }
        dist[source] = 0.0;

        // Min-heap: (cost, node)
        priority_queue<
            pair<double, string>,
            vector<pair<double, string>>,
            greater<pair<double, string>>
        > minHeap;

        minHeap.push({0.0, source});

        while (!minHeap.empty()) {
            auto [currentCost, currentNode] = minHeap.top();
            minHeap.pop();

            // Skip if we already found a better path
            if (currentCost > dist[currentNode]) {
                continue;
            }

            // Explore all neighbours
            if (adjacency.find(currentNode) != adjacency.end()) {
                for (auto& edge : adjacency[currentNode]) {
                    string neighbour = edge.first;
                    int    runs      = edge.second;

                    // Invert weight: more runs = lower cost = preferred path
                    double edgeCost = 1.0 / (runs + 1);
                    double newCost  = currentCost + edgeCost;

                    if (newCost < dist[neighbour]) {
                        dist[neighbour]        = newCost;
                        previousNode[neighbour] = currentNode;
                        minHeap.push({newCost, neighbour});
                    }
                }
            }
        }

        // Reconstruct the path by walking backwards from destination
        vector<string> path;
        string current = destination;

        while (current != source && previousNode.find(current) != previousNode.end()) {
            path.push_back(current);
            current = previousNode[current];
        }
        path.push_back(source);
        reverse(path.begin(), path.end());

        // Verify a path actually exists
        if (path.front() != source || (path.size() == 1 && source != destination)) {
            return {0, {}};
        }

        // Convert cost back to a "strength score" (higher = stronger)
        int strength = 0;
        if (dist[destination] < 1e17) {
            strength = (int)(1.0 / dist[destination]);
        }

        return {strength, path};
    }

    // ---- ALGORITHM 7a: BFS ----
    // Finds all batsmen reachable from 'startPlayer' by following
    // partnership edges (breadth-first — explores nearest first).
    vector<string> bfsReachable(const string& startPlayer) {
        unordered_set<string> visited;
        queue<string>         toVisit;
        vector<string>        reachable;

        toVisit.push(startPlayer);
        visited.insert(startPlayer);

        while (!toVisit.empty()) {
            string current = toVisit.front();
            toVisit.pop();

            // Explore all partners of 'current'
            if (adjacency.find(current) != adjacency.end()) {
                for (auto& edge : adjacency[current]) {
                    string partner = edge.first;

                    if (visited.find(partner) == visited.end()) {
                        visited.insert(partner);
                        toVisit.push(partner);
                        reachable.push_back(partner);
                    }
                }
            }
        }

        return reachable;
    }

    // ---- ALGORITHM 7b: DFS helper ----
    void dfsExplore(const string& current,
                    unordered_set<string>& visited,
                    vector<string>& component)
    {
        visited.insert(current);
        component.push_back(current);

        if (adjacency.find(current) != adjacency.end()) {
            for (auto& edge : adjacency[current]) {
                string neighbour = edge.first;

                if (visited.find(neighbour) == visited.end()) {
                    dfsExplore(neighbour, visited, component);
                }
            }
        }
    }

    // ---- ALGORITHM 7b: DFS — find all connected components ----
    // Each component = a group of batsmen who are all connected
    // (directly or indirectly) through partnerships.
    vector<vector<string>> findBattingClusters() {
        unordered_set<string>   visited;
        vector<vector<string>>  clusters;

        for (const string& node : allNodes) {
            if (visited.find(node) == visited.end()) {
                vector<string> component;
                dfsExplore(node, visited, component);
                clusters.push_back(component);
            }
        }

        return clusters;
    }

    // Print the full partnership network
    void printNetwork() const {
        cout << "\n  Partnership Network:\n";
        unordered_set<string> printed;

        for (auto& entry : adjacency) {
            string playerA = entry.first;

            for (auto& edge : entry.second) {
                string playerB = edge.first;
                int    runs    = edge.second;

                // Print each edge only once
                string key = (playerA < playerB) ? (playerA + "|" + playerB)
                                                 : (playerB + "|" + playerA);
                if (printed.find(key) == printed.end()) {
                    cout << "    " << playerA << "  &  " << playerB
                         << "  →  " << runs << " runs together\n";
                    printed.insert(key);
                }
            }
        }
    }

    bool hasNode(const string& id) const {
        return allNodes.find(id) != allNodes.end();
    }
};

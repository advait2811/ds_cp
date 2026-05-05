#pragma once
#include <bits/stdc++.h>
using namespace std;

//  ALGORITHM 1 — MIN-HEAP (PRIORITY QUEUE)
//  Processes deliveries in chronological order.
//  Push: O(log n)  |  Pop: O(log n)

struct BallEvent {
    int    over;
    int    ballInOver;
    string type;         // "DOT","RUN","FOUR","SIX","WICKET","WIDE","NOBALL","BYE","LEGBYE"
    string batsmanId;
    string bowlerId;
    int    runsScored;
    int    extras;
    string wicketType;   // "BOWLED","CAUGHT","LBW","RUN_OUT","STUMPED","HIT_WICKET"
    string fielder;
    string commentary;
    bool operator>(const BallEvent& other) const {
        if (over != other.over) { return over > other.over; }
        return ballInOver > other.ballInOver;
    }
    // Required by std::greater<BallEvent> used in priority_queue
    bool operator<(const BallEvent& other) const {
        if (over != other.over) { return over < other.over; }
        return ballInOver < other.ballInOver;
    }
    bool operator==(const BallEvent& other) const {
        return over == other.over && ballInOver == other.ballInOver;
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
//  Range sum queries for runs/wickets over any over window.
//  Update: O(log n)  |  Query: O(log n)

class SegmentTree {
private:
    int         capacity;
    vector<int> tree;
    void buildTree(vector<int>& data, int node, int start, int end) {
        if (start == end) {
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
//  Prefix sums for run-rate at any over.
//  Update: O(log n)  |  Prefix query: O(log n)

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
    int prefixSum(int index) {
        int pos   = index + 1;
        int total = 0;
        while (pos > 0) {
            total = total + data[pos];
            pos   = pos - (pos & (-pos));
        }
        return total;
    }
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
//  Player name autocomplete.
//  Insert: O(m)  |  Search: O(m + k)  where m = prefix length


struct TrieNode {
    unordered_map<char, TrieNode*> children;
    bool           isEndOfName;
    vector<string> playerIds;
    TrieNode() : isEndOfName(false) {}
};

class Trie {
private:
    TrieNode* root;
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
    void insert(const string& fullName, const string& playerId) {
        TrieNode* current = root;
        for (char ch : fullName) {
            char lower = tolower(ch);
            if (current->children.find(lower) == current->children.end()) {
                current->children[lower] = new TrieNode();
            }
            current = current->children[lower];
        }
        current->isEndOfName = true;
        current->playerIds.push_back(playerId);
    }
    vector<string> searchByPrefix(const string& prefix) {
        TrieNode* current = root;
        for (char ch : prefix) {
            char lower = tolower(ch);
            if (current->children.find(lower) == current->children.end()) {
                return {};
            }
            current = current->children[lower];
        }
        vector<string> results;
        collectAllBelow(current, results);
        return results;
    }
};


//  ALGORITHM 5 — KMP STRING MATCHING
//  Commentary log search. Finds all occurrences in O(n + m).


class KMP {
public:
    static vector<int> buildLPSTable(const string& pattern) {
        int length = pattern.size();
        vector<int> lps(length, 0);
        int prefixLen = 0;
        int i = 1;
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
                textIdx++;
                patternIdx++;
            }
            if (patternIdx == (int)lowerPattern.size()) {
                matches.push_back(textIdx - patternIdx);
                patternIdx = lps[patternIdx - 1];
            } else if (textIdx < (int)lowerText.size()
                    && lowerText[textIdx] != lowerPattern[patternIdx]) {
                if (patternIdx != 0) {
                    patternIdx = lps[patternIdx - 1];
                } else {
                    textIdx++;
                }
            }
        }
        return matches;
    }
};


//  ALGORITHM 6 & 7 — GRAPH + DIJKSTRA + BFS + DFS
//  Partnership graph. Dijkstra finds strongest chain; BFS/DFS
//  traverse reachability and clustering.

class PartnershipGraph {
private:
    unordered_map<string, vector<pair<string, int>>> adjacency;
    unordered_set<string> allNodes;
public:
    void addPartnership(const string& playerA, const string& playerB, int runs) {
        if (runs <= 0) {
            return;
        }
        allNodes.insert(playerA);
        allNodes.insert(playerB);
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
        unordered_map<string, double> dist;
        unordered_map<string, string> previousNode;
        for (const string& node : allNodes) {
            dist[node] = 1e18;
        }
        dist[source] = 0.0;
        priority_queue<
            pair<double, string>,
            vector<pair<double, string>>,
            greater<pair<double, string>>
        > minHeap;
        minHeap.push({0.0, source});
        while (!minHeap.empty()) {
            auto [currentCost, currentNode] = minHeap.top();
            minHeap.pop();
            if (currentCost > dist[currentNode]) {
                continue;
            }
            if (adjacency.find(currentNode) != adjacency.end()) {
                for (auto& edge : adjacency[currentNode]) {
                    string neighbour = edge.first;
                    int    runs      = edge.second;
                    double edgeCost  = 1.0 / (runs + 1);
                    double newCost   = currentCost + edgeCost;
                    if (newCost < dist[neighbour]) {
                        dist[neighbour]         = newCost;
                        previousNode[neighbour] = currentNode;
                        minHeap.push({newCost, neighbour});
                    }
                }
            }
        }
        vector<string> path;
        string current = destination;
        while (current != source && previousNode.find(current) != previousNode.end()) {
            path.push_back(current);
            current = previousNode[current];
        }
        path.push_back(source);
        reverse(path.begin(), path.end());
        if (path.front() != source || (path.size() == 1 && source != destination)) {
            return {0, {}};
        }
        int strength = 0;
        if (dist[destination] < 1e17) {
            strength = (int)(1.0 / dist[destination]);
        }
        return {strength, path};
    }
    // ---- ALGORITHM 7a: BFS ----
    vector<string> bfsReachable(const string& startPlayer) {
        unordered_set<string> visited;
        queue<string>         toVisit;
        vector<string>        reachable;
        toVisit.push(startPlayer);
        visited.insert(startPlayer);
        while (!toVisit.empty()) {
            string current = toVisit.front();
            toVisit.pop();
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
    vector<vector<string>> findBattingClusters() {
        unordered_set<string>  visited;
        vector<vector<string>> clusters;
        for (const string& node : allNodes) {
            if (visited.find(node) == visited.end()) {
                vector<string> component;
                dfsExplore(node, visited, component);
                clusters.push_back(component);
            }
        }
        return clusters;
    }
    void printNetwork() const {
        cout << "\n  Partnership Network:\n";
        unordered_set<string> printed;
        for (auto& entry : adjacency) {
            string playerA = entry.first;
            for (auto& edge : entry.second) {
                string playerB = edge.first;
                int    runs    = edge.second;
                string key = (playerA < playerB) ? (playerA + "|" + playerB)
                                                 : (playerB + "|" + playerA);
                if (printed.find(key) == printed.end()) {
                    cout << "    " << playerA << "  &  " << playerB
                         << "  ->  " << runs << " runs together\n";
                    printed.insert(key);
                }
            }
        }
    }
    // Returns all edges sorted by runs (highest first) — mirrors JS getAllEdges()
    vector<tuple<string, string, int>> getAllEdges() const {
        unordered_set<string> printed;
        vector<tuple<string, string, int>> edges;
        for (auto& entry : adjacency) {
            string playerA = entry.first;
            for (auto& edge : entry.second) {
                string playerB = edge.first;
                int    runs    = edge.second;
                string key = (playerA < playerB) ? (playerA + "|" + playerB)
                                                 : (playerB + "|" + playerA);
                if (printed.find(key) == printed.end()) {
                    edges.push_back({playerA, playerB, runs});
                    printed.insert(key);
                }
            }
        }
        sort(edges.begin(), edges.end(), [](const auto& a, const auto& b) {
            return get<2>(a) > get<2>(b);
        });
        return edges;
    }
    bool hasNode(const string& id) const {
        return allNodes.find(id) != allNodes.end();
    }
};


//  ALGORITHM 8 — AVL TREE (Self-Balancing BST Leaderboard)
//  Keeps the batting leaderboard sorted by runs.
//  Insert/Delete/Search: O(log n) guaranteed.

struct AVLNodeData {
    string playerId;
    string name;
    string teamId;
    int    runs;
    int    balls;
    double strikeRate() const {
        if (balls == 0) { return 0.0; }
        return (100.0 * runs) / balls;
    }
};

struct AVLNode {
    int         key;      // runs (sort key)
    string      playerId; // tiebreaker for uniqueness
    AVLNodeData data;
    AVLNode*    left;
    AVLNode*    right;
    int         height;
    AVLNode(int k, const string& pid, const AVLNodeData& d)
        : key(k), playerId(pid), data(d), left(nullptr), right(nullptr), height(1) {}
};

class AVLTree {
private:
    AVLNode* root;
    int height(AVLNode* n) {
        if (n == nullptr) { return 0; }
        return n->height;
    }
    void updateHeight(AVLNode* n) {
        if (n != nullptr) {
            n->height = 1 + max(height(n->left), height(n->right));
        }
    }
    int balanceFactor(AVLNode* n) {
        if (n == nullptr) { return 0; }
        return height(n->left) - height(n->right);
    }
    AVLNode* rotateRight(AVLNode* y) {
        AVLNode* x  = y->left;
        AVLNode* T2 = x->right;
        x->right = y;
        y->left  = T2;
        updateHeight(y);
        updateHeight(x);
        return x;
    }
    AVLNode* rotateLeft(AVLNode* x) {
        AVLNode* y  = x->right;
        AVLNode* T2 = y->left;
        y->left  = x;
        x->right = T2;
        updateHeight(x);
        updateHeight(y);
        return y;
    }
    AVLNode* balance(AVLNode* node) {
        updateHeight(node);
        int bf = balanceFactor(node);
        // Left-Left
        if (bf > 1 && balanceFactor(node->left) >= 0) {
            return rotateRight(node);
        }
        // Left-Right
        if (bf > 1 && balanceFactor(node->left) < 0) {
            node->left = rotateLeft(node->left);
            return rotateRight(node);
        }
        // Right-Right
        if (bf < -1 && balanceFactor(node->right) <= 0) {
            return rotateLeft(node);
        }
        // Right-Left
        if (bf < -1 && balanceFactor(node->right) > 0) {
            node->right = rotateRight(node->right);
            return rotateLeft(node);
        }
        return node;
    }
    AVLNode* insertNode(AVLNode* node, int key, const string& playerId, const AVLNodeData& data) {
        if (node == nullptr) {
            return new AVLNode(key, playerId, data);
        }
        if (key < node->key || (key == node->key && playerId < node->playerId)) {
            node->left  = insertNode(node->left,  key, playerId, data);
        } else {
            node->right = insertNode(node->right, key, playerId, data);
        }
        return balance(node);
    }
    // Find minimum node in subtree (used by delete)
    AVLNode* findMin(AVLNode* node) {
        while (node->left != nullptr) {
            node = node->left;
        }
        return node;
    }
    AVLNode* deleteNode(AVLNode* node, const string& playerId) {
        if (node == nullptr) {
            return nullptr;
        }
        if (playerId == node->playerId) {
            if (node->left == nullptr) {
                AVLNode* right = node->right;
                delete node;
                return right;
            }
            if (node->right == nullptr) {
                AVLNode* left = node->left;
                delete node;
                return left;
            }
            // Two children: replace with in-order successor
            AVLNode* succ  = findMin(node->right);
            node->key      = succ->key;
            node->playerId = succ->playerId;
            node->data     = succ->data;
            node->right    = deleteNode(node->right, succ->playerId);
        } else {
            node->left  = deleteNode(node->left,  playerId);
            node->right = deleteNode(node->right, playerId);
        }
        return balance(node);
    }
    void inOrderCollect(AVLNode* node, vector<AVLNodeData>& result) {
        if (node == nullptr) { return; }
        inOrderCollect(node->left, result);
        result.push_back(node->data);
        inOrderCollect(node->right, result);
    }
    void deleteAll(AVLNode* node) {
        if (node == nullptr) { return; }
        deleteAll(node->left);
        deleteAll(node->right);
        delete node;
    }
public:
    AVLTree() : root(nullptr) {}
    ~AVLTree() {
        deleteAll(root);
    }
    // Insert or update a player (removes old entry first, then re-inserts)
    void upsert(const AVLNodeData& data) {
        root = deleteNode(root, data.playerId);
        root = insertNode(root, data.runs, data.playerId, data);
    }
    // Remove a player from the tree
    void remove(const string& playerId) {
        root = deleteNode(root, playerId);
    }
    // Returns players sorted by runs descending (highest first)
    vector<AVLNodeData> getLeaderboard() {
        vector<AVLNodeData> result;
        inOrderCollect(root, result);
        reverse(result.begin(), result.end());
        return result;
    }
    int getHeight() {
        return height(root);
    }
};


//  ALGORITHM 9 — HASH TABLE WITH CHAINING
//  Maps player IDs -> stats. djb2 hash, explicit chaining.
//  Average O(1) insert / lookup; O(n) worst case on collision.

struct PlayerStatEntry {
    string playerId;
    string name;
    string teamId;
    int    runs;
    int    balls;
    int    wickets;
    double economy;
};

class HashTable {
private:
    static const int DEFAULT_SIZE = 16;
    int size;
    vector<vector<pair<string, PlayerStatEntry>>> buckets;
    int count;
    // djb2-style hash function
    int hash(const string& key) const {
        unsigned int h = 5381;
        for (char ch : key) {
            h = ((h << 5) + h) + (unsigned char)ch;
        }
        return (int)(h % (unsigned int)size);
    }
public:
    HashTable(int sz = DEFAULT_SIZE) : size(sz), buckets(sz), count(0) {}
    void set(const string& key, const PlayerStatEntry& value) {
        int idx = hash(key);
        for (auto& entry : buckets[idx]) {
            if (entry.first == key) {
                entry.second = value;
                return;
            }
        }
        buckets[idx].push_back({key, value});
        count++;
    }
    // Returns pointer to entry, or nullptr if not found
    PlayerStatEntry* get(const string& key) {
        int idx = hash(key);
        for (auto& entry : buckets[idx]) {
            if (entry.first == key) {
                return &entry.second;
            }
        }
        return nullptr;
    }
    bool remove(const string& key) {
        int idx = hash(key);
        auto& chain = buckets[idx];
        for (int i = 0; i < (int)chain.size(); i++) {
            if (chain[i].first == key) {
                chain.erase(chain.begin() + i);
                count--;
                return true;
            }
        }
        return false;
    }
    double loadFactor() const {
        return (double)count / size;
    }
    int getCount() const {
        return count;
    }
    // Print the entire hash table (shows chains for each bucket)
    void printTable() const {
        cout << "\n  [Hash Table] " << count << " entries, load factor = "
             << fixed << setprecision(2) << loadFactor() << "\n";
        for (int i = 0; i < size; i++) {
            if (buckets[i].empty()) { continue; }
            cout << "  Bucket[" << setw(2) << i << "]: ";
            for (int j = 0; j < (int)buckets[i].size(); j++) {
                const auto& entry = buckets[i][j];
                cout << entry.first
                     << " (runs=" << entry.second.runs
                     << ", wkts=" << entry.second.wickets << ")";
                if (j + 1 < (int)buckets[i].size()) { cout << " -> "; }
            }
            cout << "\n";
        }
    }
    // Return all entries as a flat vector
    vector<PlayerStatEntry> getAllEntries() const {
        vector<PlayerStatEntry> result;
        for (auto& chain : buckets) {
            for (auto& entry : chain) {
                result.push_back(entry.second);
            }
        }
        return result;
    }
};


//  ALGORITHM 10 — INTERVAL TREE (Augmented BST)
//  Stores batting "spells" as intervals [entryOver, dismissalOver].
//  Answers stabbing query: "who was batting at over X?"
//  Query pruned by maxEnd — O(log n + k) on average.

struct BattingInterval {
    int    start;     // over when batsman came to crease (1-indexed)
    int    end;       // over of dismissal or last over (1-indexed)
    string playerId;
    int    runs;      // runs scored during this spell
};

struct IntervalNode {
    BattingInterval interval;
    int             maxEnd;   // max 'end' in this subtree — used for pruning
    IntervalNode*   left;
    IntervalNode*   right;
    IntervalNode(const BattingInterval& iv)
        : interval(iv), maxEnd(iv.end), left(nullptr), right(nullptr) {}
};

class IntervalTree {
private:
    IntervalNode* root;
    IntervalNode* insertNode(IntervalNode* node, const BattingInterval& iv) {
        if (node == nullptr) {
            return new IntervalNode(iv);
        }
        if (iv.start < node->interval.start) {
            node->left  = insertNode(node->left,  iv);
        } else {
            node->right = insertNode(node->right, iv);
        }
        // Update maxEnd for pruning
        int leftMax  = node->left  ? node->left->maxEnd  : -1;
        int rightMax = node->right ? node->right->maxEnd : -1;
        node->maxEnd = max({node->interval.end, leftMax, rightMax});
        return node;
    }
    // Find all intervals overlapping [qStart, qEnd]
    void queryNode(IntervalNode* node, int qStart, int qEnd,
                   vector<BattingInterval>& results)
    {
        if (node == nullptr) { return; }
        // Prune: if maxEnd of subtree < qStart, no match possible
        if (node->maxEnd < qStart) { return; }
        if (node->interval.start <= qEnd && node->interval.end >= qStart) {
            results.push_back(node->interval);
        }
        queryNode(node->left,  qStart, qEnd, results);
        queryNode(node->right, qStart, qEnd, results);
    }
    void collectAll(IntervalNode* node, vector<BattingInterval>& result) {
        if (node == nullptr) { return; }
        result.push_back(node->interval);
        collectAll(node->left,  result);
        collectAll(node->right, result);
    }
    void deleteAll(IntervalNode* node) {
        if (node == nullptr) { return; }
        deleteAll(node->left);
        deleteAll(node->right);
        delete node;
    }
public:
    IntervalTree() : root(nullptr) {}
    ~IntervalTree() {
        deleteAll(root);
    }
    void insert(const BattingInterval& iv) {
        root = insertNode(root, iv);
    }
    // Stabbing query: all batsmen active at 'over' (1-indexed)
    vector<BattingInterval> query(int qStart, int qEnd) {
        vector<BattingInterval> results;
        queryNode(root, qStart, qEnd, results);
        return results;
    }
    vector<BattingInterval> getAllIntervals() {
        vector<BattingInterval> result;
        collectAll(root, result);
        return result;
    }
};
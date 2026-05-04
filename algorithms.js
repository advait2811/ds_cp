// ================================================================
//  algorithms.js
//  All DSA implementations for Cricket Stats Analyser
//  Algorithms 1–9
// ================================================================


// ---- ALGORITHM 1: MIN-HEAP (PRIORITY QUEUE) ----
// Processes deliveries in chronological order.
class MinHeap {
  constructor() { this.data = []; }

  parentIndex(i)    { return Math.floor((i - 1) / 2); }
  leftChildIndex(i) { return 2 * i + 1; }
  rightChildIndex(i){ return 2 * i + 2; }

  isHigherPriority(a, b) {
    if (a.over !== b.over) return a.over < b.over;
    return a.ballInOver < b.ballInOver;
  }

  push(ball) {
    this.data.push(ball);
    this.bubbleUp(this.data.length - 1);
  }

  bubbleUp(index) {
    while (index > 0) {
      const parent = this.parentIndex(index);
      if (this.isHigherPriority(this.data[index], this.data[parent])) {
        [this.data[index], this.data[parent]] = [this.data[parent], this.data[index]];
        index = parent;
      } else break;
    }
  }

  pop() {
    if (this.data.length === 0) throw new Error('Heap is empty');
    const top  = this.data[0];
    const last = this.data.pop();
    if (this.data.length > 0) { this.data[0] = last; this.sinkDown(0); }
    return top;
  }

  sinkDown(index) {
    const len = this.data.length;
    while (true) {
      let smallest = index;
      const left   = this.leftChildIndex(index);
      const right  = this.rightChildIndex(index);
      if (left  < len && this.isHigherPriority(this.data[left],  this.data[smallest])) smallest = left;
      if (right < len && this.isHigherPriority(this.data[right], this.data[smallest])) smallest = right;
      if (smallest !== index) {
        [this.data[index], this.data[smallest]] = [this.data[smallest], this.data[index]];
        index = smallest;
      } else break;
    }
  }

  isEmpty() { return this.data.length === 0; }
}


// ---- ALGORITHM 2: SEGMENT TREE ----
// Range queries for runs/wickets over any over window. O(log n).
class SegmentTree {
  constructor(size) {
    this.capacity = size;
    this.tree     = new Array(4 * size).fill(0);
  }

  update(index, value) {
    if (index < 0 || index >= this.capacity) return;
    this._updateNode(1, 0, this.capacity - 1, index, value);
  }

  _updateNode(node, start, end, index, value) {
    if (start === end) { this.tree[node] += value; return; }
    const mid = Math.floor((start + end) / 2);
    if (index <= mid) this._updateNode(2 * node,     start,   mid, index, value);
    else              this._updateNode(2 * node + 1, mid + 1, end, index, value);
    this.tree[node] = this.tree[2 * node] + this.tree[2 * node + 1];
  }

  query(left, right) {
    if (left > right || left < 0 || right >= this.capacity) return 0;
    return this._queryNode(1, 0, this.capacity - 1, left, right);
  }

  _queryNode(node, start, end, left, right) {
    if (right < start || end < left) return 0;
    if (left <= start && end <= right) return this.tree[node];
    const mid = Math.floor((start + end) / 2);
    return this._queryNode(2 * node,     start,   mid, left, right)
         + this._queryNode(2 * node + 1, mid + 1, end, left, right);
  }
}


// ---- ALGORITHM 3: FENWICK TREE ----
// Prefix sums for run-rate at any over. O(log n) update + query.
class FenwickTree {
  constructor(size) {
    this.capacity = size;
    this.data     = new Array(size + 2).fill(0);
  }

  update(index, delta) {
    let pos = index + 1;
    while (pos <= this.capacity) { this.data[pos] += delta; pos += pos & (-pos); }
  }

  prefixSum(index) {
    let pos = index + 1, total = 0;
    while (pos > 0) { total += this.data[pos]; pos -= pos & (-pos); }
    return total;
  }

  rangeSum(left, right) {
    if (left > right) return 0;
    if (left === 0)   return this.prefixSum(right);
    return this.prefixSum(right) - this.prefixSum(left - 1);
  }
}


// ---- ALGORITHM 4: TRIE ----
// Player name autocomplete.
class TrieNode {
  constructor() { this.children = {}; this.isEndOfName = false; this.playerIds = []; }
}

class Trie {
  constructor() { this.root = new TrieNode(); }

  insert(fullName, playerId) {
    let cur = this.root;
    for (const ch of fullName.toLowerCase()) {
      if (!cur.children[ch]) cur.children[ch] = new TrieNode();
      cur = cur.children[ch];
    }
    cur.isEndOfName = true;
    cur.playerIds.push(playerId);
  }

  searchByPrefix(prefix) {
    let cur = this.root;
    for (const ch of prefix.toLowerCase()) {
      if (!cur.children[ch]) return [];
      cur = cur.children[ch];
    }
    const results = [];
    this._collectAll(cur, results);
    return results;
  }

  _collectAll(node, results) {
    if (node.isEndOfName) results.push(...node.playerIds);
    for (const ch in node.children) this._collectAll(node.children[ch], results);
  }
}


// ---- ALGORITHM 5: KMP STRING MATCHING ----
// Commentary log search.
class KMP {
  static buildLPS(pattern) {
    const lps = new Array(pattern.length).fill(0);
    let prefixLen = 0, i = 1;
    while (i < pattern.length) {
      if (pattern[i] === pattern[prefixLen]) { lps[i++] = ++prefixLen; }
      else if (prefixLen > 0) { prefixLen = lps[prefixLen - 1]; }
      else { lps[i++] = 0; }
    }
    return lps;
  }

  static search(text, pattern) {
    if (!pattern) return [];
    const t = text.toLowerCase(), p = pattern.toLowerCase();
    const lps = KMP.buildLPS(p);
    const matches = [];
    let ti = 0, pi = 0;
    while (ti < t.length) {
      if (t[ti] === p[pi]) { ti++; pi++; }
      if (pi === p.length) { matches.push(ti - pi); pi = lps[pi - 1]; }
      else if (ti < t.length && t[ti] !== p[pi]) {
        if (pi !== 0) pi = lps[pi - 1]; else ti++;
      }
    }
    return matches;
  }
}


// ---- ALGORITHM 6 & 7: PARTNERSHIP GRAPH (Dijkstra + BFS + DFS) ----
class PartnershipGraph {
  constructor() { this.adjacency = {}; this.allNodes = new Set(); }

  addPartnership(playerA, playerB, runs) {
    if (runs <= 0) return;
    this.allNodes.add(playerA);
    this.allNodes.add(playerB);
    if (!this.adjacency[playerA]) this.adjacency[playerA] = [];
    if (!this.adjacency[playerB]) this.adjacency[playerB] = [];

    const edgeAB = this.adjacency[playerA].find(e => e.partner === playerB);
    if (edgeAB) edgeAB.runs += runs;
    else        this.adjacency[playerA].push({ partner: playerB, runs });

    const edgeBA = this.adjacency[playerB].find(e => e.partner === playerA);
    if (edgeBA) edgeBA.runs += runs;
    else        this.adjacency[playerB].push({ partner: playerA, runs });
  }

  // Dijkstra: strongest partnership chain between two players
  dijkstra(source, destination) {
    const distMap = {}, prevMap = {}, visited = new Set();
    for (const n of this.allNodes) distMap[n] = Infinity;
    distMap[source] = 0;

    const queue = [{ cost: 0, node: source }];

    while (queue.length > 0) {
      queue.sort((a, b) => a.cost - b.cost);
      const { cost, node } = queue.shift();
      if (visited.has(node)) continue;
      visited.add(node);

      for (const edge of (this.adjacency[node] || [])) {
        const edgeCost = 1.0 / (edge.runs + 1);
        const newCost  = cost + edgeCost;
        if (newCost < distMap[edge.partner]) {
          distMap[edge.partner] = newCost;
          prevMap[edge.partner] = node;
          queue.push({ cost: newCost, node: edge.partner });
        }
      }
    }

    const path = [];
    let cur = destination;
    while (cur) { path.unshift(cur); cur = prevMap[cur]; if (cur === source) { path.unshift(cur); break; } }

    let strength = 0;
    for (let i = 0; i < path.length - 1; i++) {
      const edge = (this.adjacency[path[i]] || []).find(e => e.partner === path[i + 1]);
      if (edge) strength += edge.runs;
    }

    return { path: distMap[destination] < Infinity ? path : [], strength };
  }

  // BFS: all reachable partners from a player
  bfsReachable(source) {
    if (!this.adjacency[source]) return [];
    const visited = new Set([source]);
    const queue   = [source];
    const result  = [];
    while (queue.length > 0) {
      const node = queue.shift();
      for (const edge of (this.adjacency[node] || [])) {
        if (!visited.has(edge.partner)) {
          visited.add(edge.partner);
          queue.push(edge.partner);
          result.push(edge.partner);
        }
      }
    }
    return result;
  }

  // DFS: find connected batting clusters
  findBattingClusters() {
    const visited  = new Set();
    const clusters = [];

    const dfs = (node, cluster) => {
      visited.add(node);
      cluster.push(node);
      for (const edge of (this.adjacency[node] || [])) {
        if (!visited.has(edge.partner)) dfs(edge.partner, cluster);
      }
    };

    for (const node of this.allNodes) {
      if (!visited.has(node)) {
        const cluster = [];
        dfs(node, cluster);
        clusters.push(cluster);
      }
    }
    return clusters;
  }

  getAllEdges() {
    const edges   = [];
    const printed = new Set();
    for (const [playerA, neighbours] of Object.entries(this.adjacency)) {
      for (const edge of neighbours) {
        const key = [playerA, edge.partner].sort().join('|');
        if (!printed.has(key)) {
          edges.push({ playerA, playerB: edge.partner, runs: edge.runs });
          printed.add(key);
        }
      }
    }
    return edges.sort((a, b) => b.runs - a.runs);
  }
}


// ---- ALGORITHM 8: AVL TREE (Self-Balancing BST Leaderboard) ----
// Keeps the batting leaderboard sorted by runs in O(log n) per update.
class AVLNode {
  constructor(key, value) {
    this.key    = key;   // runs (sort key)
    this.value  = value; // { playerId, name, team, runs, balls }
    this.left   = null;
    this.right  = null;
    this.height = 1;
  }
}

class AVLTree {
  constructor() { this.root = null; }

  _height(n) { return n ? n.height : 0; }

  _updateHeight(n) {
    if (n) n.height = 1 + Math.max(this._height(n.left), this._height(n.right));
  }

  _balanceFactor(n) { return n ? this._height(n.left) - this._height(n.right) : 0; }

  _rotateRight(y) {
    const x = y.left, T2 = x.right;
    x.right = y; y.left = T2;
    this._updateHeight(y); this._updateHeight(x);
    return x;
  }

  _rotateLeft(x) {
    const y = x.right, T2 = y.left;
    y.left = x; x.right = T2;
    this._updateHeight(x); this._updateHeight(y);
    return y;
  }

  _balance(node) {
    this._updateHeight(node);
    const bf = this._balanceFactor(node);

    // Left-Left
    if (bf > 1 && this._balanceFactor(node.left) >= 0)
      return this._rotateRight(node);

    // Left-Right
    if (bf > 1 && this._balanceFactor(node.left) < 0) {
      node.left = this._rotateLeft(node.left);
      return this._rotateRight(node);
    }

    // Right-Right
    if (bf < -1 && this._balanceFactor(node.right) <= 0)
      return this._rotateLeft(node);

    // Right-Left
    if (bf < -1 && this._balanceFactor(node.right) > 0) {
      node.right = this._rotateRight(node.right);
      return this._rotateLeft(node);
    }

    return node;
  }

  // Insert or update by playerId (remove old entry, insert new one)
  insert(playerId, value) {
    // Remove existing entry for this player first
    this.root = this._delete(this.root, playerId);
    // Insert with key = runs (tie-break by playerId for uniqueness)
    this.root = this._insert(this.root, value.runs, playerId, value);
  }

  _insert(node, key, playerId, value) {
    if (!node) return new AVLNode(key, value);

    // Use playerId as tiebreaker
    if (key < node.key || (key === node.key && playerId < node.value.playerId))
      node.left  = this._insert(node.left,  key, playerId, value);
    else
      node.right = this._insert(node.right, key, playerId, value);

    return this._balance(node);
  }

  _delete(node, playerId) {
    if (!node) return null;
    if (playerId === node.value.playerId) {
      if (!node.left)  return node.right;
      if (!node.right) return node.left;
      // Find in-order successor (smallest in right subtree)
      let succ = node.right;
      while (succ.left) succ = succ.left;
      node.key   = succ.key;
      node.value = succ.value;
      node.right = this._delete(node.right, succ.value.playerId);
    } else {
      node.left  = this._delete(node.left,  playerId);
      node.right = this._delete(node.right, playerId);
    }
    return this._balance(node);
  }

  // In-order traversal (ascending). Reverse for descending leaderboard.
  inOrder() {
    const result = [];
    const traverse = (node) => {
      if (!node) return;
      traverse(node.left);
      result.push(node.value);
      traverse(node.right);
    };
    traverse(this.root);
    return result.reverse(); // descending by runs
  }

  // Height of tree (for visual)
  getHeight() { return this._height(this.root); }
}


// ---- ALGORITHM 9: HASH TABLE WITH CHAINING ----
// Maps player IDs to stats. Shows explicit chaining on collision.
class HashTable {
  constructor(size = 16) {
    this.size    = size;
    this.buckets = Array.from({ length: size }, () => []);
    this.count   = 0;
  }

  // djb2-style hash
  _hash(key) {
    let h = 5381;
    for (const ch of key) h = ((h << 5) + h + ch.charCodeAt(0)) & 0xffffffff;
    return Math.abs(h) % this.size;
  }

  set(key, value) {
    const idx  = this._hash(key);
    const chain = this.buckets[idx];
    const existing = chain.find(e => e.key === key);
    if (existing) { existing.value = value; }
    else          { chain.push({ key, value }); this.count++; }
  }

  get(key) {
    const chain = this.buckets[this._hash(key)];
    return chain.find(e => e.key === key)?.value;
  }

  loadFactor() { return (this.count / this.size).toFixed(2); }

  // Returns array of { index, chain } for visualisation
  getBuckets() {
    return this.buckets.map((chain, i) => ({ index: i, chain }));
  }
}


// ---- ALGORITHM 10 (BONUS): INTERVAL TREE ----
// Stores batting "spells" (over ranges where a batsman was active).
// Answers stabbing queries: which batsmen were active at over X?
class Interval {
  constructor(start, end, playerId, runs) {
    this.start    = start;
    this.end      = end;
    this.playerId = playerId;
    this.runs     = runs;
  }
}

class IntervalNode {
  constructor(interval) {
    this.interval = interval;
    this.maxEnd   = interval.end;
    this.left     = null;
    this.right    = null;
  }
}

class IntervalTree {
  constructor() { this.root = null; }

  insert(interval) {
    this.root = this._insert(this.root, interval);
  }

  _insert(node, interval) {
    if (!node) return new IntervalNode(interval);

    if (interval.start < node.interval.start)
      node.left  = this._insert(node.left,  interval);
    else
      node.right = this._insert(node.right, interval);

    node.maxEnd = Math.max(node.maxEnd,
                           node.left  ? node.left.maxEnd  : -Infinity,
                           node.right ? node.right.maxEnd : -Infinity);
    return node;
  }

  // Find all intervals that overlap with [qStart, qEnd]
  query(qStart, qEnd) {
    const results = [];
    this._query(this.root, qStart, qEnd, results);
    return results;
  }

  _query(node, qStart, qEnd, results) {
    if (!node || node.maxEnd < qStart) return;
    if (node.interval.start <= qEnd && node.interval.end >= qStart)
      results.push(node.interval);
    this._query(node.left,  qStart, qEnd, results);
    this._query(node.right, qStart, qEnd, results);
  }

  // All intervals (for rendering)
  getAllIntervals() {
    const result = [];
    const traverse = (node) => {
      if (!node) return;
      result.push(node.interval);
      traverse(node.left);
      traverse(node.right);
    };
    traverse(this.root);
    return result;
  }
}

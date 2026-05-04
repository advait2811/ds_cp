// ================================================================
//  render.js
//  All UI rendering and algorithm interaction handlers
// ================================================================

function renderAll() {
  renderScoreHero();
  renderBattingCard('IND', IND_ORDER, 'ind-bat-body', 'ind-extras');
  renderBattingCard('AUS', AUS_ORDER, 'aus-bat-body', 'aus-extras');
  renderBowlingCard();
  renderCommentary();
  renderPartnershipList();
  renderLeaderboard();
  renderHashTable();
  renderResult();
  updateDijkstraPlayerList();
}

// ── SCORE HERO ────────────────────────────────────────────────
function renderScoreHero() {
  const inn1 = state.inn[1];
  const inn2 = state.inn[2];

  document.getElementById('ind-score').textContent = `${inn1.runs}/${inn1.wickets}`;
  document.getElementById('ind-overs').textContent = `(${Math.floor(inn1.legalBalls/6)}.${inn1.legalBalls%6} ov)`;

  if (state.innings === 1) {
    document.getElementById('aus-score').textContent = '—';
    document.getElementById('aus-overs').textContent  = 'yet to bat';
    document.getElementById('rr').textContent         = runRate(inn1).toFixed(2);
    document.getElementById('rrrItem').style.display   = 'none';
    document.getElementById('targetItem').style.display = 'none';
    document.getElementById('needItem').style.display   = 'none';
    document.getElementById('inningsLabel').textContent = '1st · India bat';
    document.getElementById('inningsBadge').textContent = 'Innings 1';
  } else {
    document.getElementById('aus-score').textContent = `${inn2.runs}/${inn2.wickets}`;
    document.getElementById('aus-overs').textContent  = `(${Math.floor(inn2.legalBalls/6)}.${inn2.legalBalls%6} ov)`;
    document.getElementById('rr').textContent         = runRate(inn2).toFixed(2);

    if (state.matchOver) {
      // Match done — hide chase meta
      document.getElementById('rrrItem').style.display    = 'none';
      document.getElementById('needItem').style.display   = 'none';
      document.getElementById('targetItem').style.display = 'flex';
      document.getElementById('target').textContent = state.target;
    } else {
      const need      = state.target - inn2.runs;
      const ballsLeft = 120 - inn2.legalBalls;
      const rrr       = need > 0 && ballsLeft > 0 ? (need / (ballsLeft / 6)) : 0;

      document.getElementById('rrrItem').style.display    = 'flex';
      document.getElementById('targetItem').style.display = 'flex';
      document.getElementById('needItem').style.display   = 'flex';
      document.getElementById('rrr').textContent    = rrr.toFixed(2);
      document.getElementById('target').textContent = state.target;
      document.getElementById('need').textContent   = `${need} off ${ballsLeft}b`;
    }

    document.getElementById('inningsLabel').textContent = '2nd · Australia chase';
    document.getElementById('inningsBadge').textContent = 'Innings 2';
  }
}

// ── BATTING CARD ──────────────────────────────────────────────
function renderBattingCard(teamId, order, tbodyId, extrasId) {
  const tbody   = document.getElementById(tbodyId);
  const extrasEl = document.getElementById(extrasId);
  const inn     = teamId === 'IND' ? state.inn[1] : state.inn[2];
  tbody.innerHTML = '';

  for (const id of order) {
    const b = state.bat[id];
    if (!b.didBat) continue;

    const isStriker    = id === state.striker    && state.innings === (teamId === 'IND' ? 1 : 2);
    const isNonStriker = id === state.nonStriker && state.innings === (teamId === 'IND' ? 1 : 2);
    const sr = b.balls > 0 ? (100 * b.runs / b.balls).toFixed(1) : '0.0';

    const tr = document.createElement('tr');
    if (isStriker || isNonStriker) tr.className = 'on-strike';

    let nameCell = `<span class="bat-name">${PLAYERS[id].name}</span>`;
    if (isStriker)    nameCell += ` <span style="color:var(--crimson);font-size:10px">★</span>`;
    if (isNonStriker) nameCell += ` <span style="color:var(--text-muted);font-size:10px">◇</span>`;

    let dismissalCell = '';
    if (!isStriker && !isNonStriker) {
      dismissalCell = `<br><span class="bat-dismissal">${b.dismissal}</span>`;
    }

    tr.innerHTML = `
      <td>${nameCell}${dismissalCell}</td>
      <td>${b.runs}</td><td>${b.balls}</td><td>${b.fours}</td><td>${b.sixes}</td><td>${sr}</td>
    `;
    tbody.appendChild(tr);
  }

  if (inn) {
    extrasEl.textContent = `Extras: ${inn.extras}  (w ${inn.wides}, nb ${inn.noBalls}, b ${inn.byes})  ·  Total: ${inn.runs}/${inn.wickets}  (RR: ${runRate(inn).toFixed(2)})`;
  }
}

// ── BOWLING CARD ──────────────────────────────────────────────
function renderBowlingCard() {
  const tbody   = document.getElementById('bowl-body');
  const bowlers = state.innings === 1 ? AUS_BOWLERS : IND_BOWLERS;
  tbody.innerHTML = '';

  for (const id of bowlers) {
    const b = state.bowl[id];
    if (b.overs === 0 && b.balls === 0) continue;
    const econ = (b.overs + b.balls/6) > 0 ? (b.runs / (b.overs + b.balls/6)).toFixed(2) : '—';
    const tr = document.createElement('tr');
    tr.innerHTML = `
      <td>${PLAYERS[id].name}</td>
      <td>${b.overs}.${b.balls}</td><td>${b.maidens}</td><td>${b.runs}</td><td>${b.wickets}</td><td>${econ}</td>
    `;
    tbody.appendChild(tr);
  }
}

// ── COMMENTARY ────────────────────────────────────────────────
function renderCommentary() {
  const feed = document.getElementById('commentaryFeed');
  feed.innerHTML = '';

  const recent = [...state.commentary].reverse().slice(0, 60);
  for (const entry of recent) {
    const div = document.createElement('div');
    div.className = 'ball-line';

    let dotClass = 'dot', dotText = '•';
    if (entry.type === 'FOUR')   { dotClass = 'four';   dotText = '4'; }
    if (entry.type === 'SIX')    { dotClass = 'six';    dotText = '6'; }
    if (entry.type === 'WICKET') { dotClass = 'wicket'; dotText = 'W'; }
    if (entry.type === 'WIDE')   { dotClass = 'wide';   dotText = 'wd'; }
    if (entry.type === 'NOBALL') { dotClass = 'noball'; dotText = 'nb'; }
    if (entry.type === 'RUN')    { dotClass = 'run1';   dotText = '1'; }
    if (entry.type === 'BYE')    { dotClass = 'run1';   dotText = 'b'; }

    let textClass = 'ball-text';
    if (entry.type === 'WICKET') textClass += ' wicket';
    if (entry.type === 'FOUR')   textClass += ' four';
    if (entry.type === 'SIX')    textClass += ' six';

    const overMatch  = entry.line.match(/\[(\d+\.\d+)\]/);
    const overLabel  = overMatch ? overMatch[1] : '';
    const scoreMatch = entry.line.match(/\|\s+(.+)$/);
    const score      = scoreMatch ? scoreMatch[1] : '';
    const comment    = entry.line.replace(/\[[\d.]+\]\s+/, '').replace(/\s+\|.*$/, '');

    div.innerHTML = `
      <div class="ball-dot ${dotClass}">${dotText}</div>
      <div class="ball-over">${overLabel}</div>
      <div class="${textClass}">${comment}</div>
      <div class="ball-score">${score}</div>
    `;
    feed.appendChild(div);
  }
}

// ── PARTNERSHIP LIST ──────────────────────────────────────────
function renderPartnershipList() {
  const el = document.getElementById('partnership-list');
  const edges = state.graph.getAllEdges(); // only sealed (post-wicket) partnerships

  // Build live partnership row if match is in progress
  const liveNameA = PLAYERS[state.striker]?.name    || state.striker;
  const liveNameB = PLAYERS[state.nonStriker]?.name || state.nonStriker;
  const liveRuns  = state.partRuns;
  const hasLive   = !state.matchOver && liveRuns >= 0;

  if (edges.length === 0 && !hasLive) {
    el.innerHTML = '<div style="color:var(--text-muted);font-family:var(--mono);font-size:12px">Partnerships will appear as the match progresses.</div>';
    return;
  }

  // Max runs for bar scaling (include live)
  const maxRuns = Math.max(
    edges.length > 0 ? edges[0].runs : 0,
    liveRuns,
    1
  );

  el.innerHTML = '';

  // Live partnership at top (highlighted)
  if (hasLive) {
    const pct = (liveRuns / maxRuns) * 100;
    const row = document.createElement('div');
    row.className = 'partnership-row';
    row.innerHTML = `
      <div class="partnership-names" style="color:var(--crimson)">
        ${liveNameA} & ${liveNameB}
        <span style="font-size:9px;color:var(--text-muted);margin-left:4px">● live</span>
      </div>
      <div class="partnership-bar-wrap">
        <div class="partnership-bar" style="width:${pct}%;background:var(--crimson)"></div>
      </div>
      <div class="partnership-runs">${liveRuns}</div>
    `;
    el.appendChild(row);
  }

  // Sealed partnerships below
  for (const edge of edges) {
    const nameA = PLAYERS[edge.playerA]?.name || edge.playerA;
    const nameB = PLAYERS[edge.playerB]?.name || edge.playerB;
    const pct   = (edge.runs / maxRuns) * 100;

    const row = document.createElement('div');
    row.className = 'partnership-row';
    row.innerHTML = `
      <div class="partnership-names">${nameA} & ${nameB}</div>
      <div class="partnership-bar-wrap">
        <div class="partnership-bar" style="width:${pct}%"></div>
      </div>
      <div class="partnership-runs">${edge.runs}</div>
    `;
    el.appendChild(row);
  }
}

// ── LEADERBOARD (AVL Tree) ────────────────────────────────────
function renderLeaderboard() {
  const el = document.getElementById('avl-leaderboard');
  const entries = state.avl.inOrder().slice(0, 10);

  if (entries.length === 0) {
    el.innerHTML = '<tr><td colspan="5" style="color:var(--text-muted);text-align:center;padding:12px">No batting data yet.</td></tr>';
    return;
  }

  const medals = ['🥇', '🥈', '🥉'];
  el.innerHTML = entries.map((e, i) => {
    const sr = e.balls > 0 ? (100 * e.runs / e.balls).toFixed(0) : '0';
    const rowClass = i < 3 ? `rank-${i+1}` : '';
    return `<tr class="${rowClass}">
      <td><span class="rank-medal">${medals[i] || (i+1)}</span></td>
      <td>${e.name}</td>
      <td>${e.team}</td>
      <td>${e.runs}(${e.balls})</td>
      <td>${sr}</td>
    </tr>`;
  }).join('');
}

// ── HASH TABLE VISUAL ─────────────────────────────────────────
function renderHashTable() {
  const el = document.getElementById('hash-grid');
  if (!el) return; 
  const buckets = state.hashTable.getBuckets();
  el.innerHTML = '';

  for (const { index, chain } of buckets) {
    const cell = document.createElement('div');
    cell.className = 'hash-cell' + (chain.length > 0 ? ' occupied' : '');
    cell.title = chain.length > 0
      ? chain.map(e => `${e.key}: ${e.value.runs}r`).join('\n')
      : `Bucket ${index} — empty`;

    let html = `<div class="bucket-idx">#${index}</div>`;
    if (chain.length > 0) {
      const first = chain[0];
      html += `<div class="bucket-val">${first.value.runs}r</div>`;
      if (chain.length > 1) {
        html += `<div class="chain-val">+${chain.length - 1}</div>`;
      }
    }
    cell.innerHTML = html;
    el.appendChild(cell);
  }

  document.getElementById('hash-stats').textContent =
    `Load factor: ${state.hashTable.loadFactor()}  ·  ${state.hashTable.count} entries / ${state.hashTable.size} buckets`;
}

// ── INTERVAL TREE SPELL LIST ──────────────────────────────────
// No chart — just renders the stored spells as a clean table so
// the query result below is easy to understand.
function renderIntervalChart() {
  const el = document.getElementById('interval-spell-list');
  if (!el) return;

  const intervals = state.intervalTree.getAllIntervals();
  if (intervals.length === 0) {
    el.innerHTML = '<div style="color:var(--text-muted)">Spells will appear as wickets fall.</div>';
    return;
  }

  // Sort by start over
  const sorted = [...intervals].sort((a, b) => a.start - b.start || a.playerId.localeCompare(b.playerId));

  el.innerHTML = sorted.map(iv => {
    const name = PLAYERS[iv.playerId]?.name || iv.playerId;
    const team = PLAYERS[iv.playerId]?.team || '';
    const span = iv.end - iv.start;
    const overs = span === 0
      ? `Over ${iv.start + 1}`
      : `Overs ${iv.start + 1} – ${iv.end + 1}`;
    return `
      <div style="display:flex;justify-content:space-between;align-items:center;
                  padding:6px 0;border-bottom:1px solid var(--border-light);
                  font-family:var(--mono);font-size:12px;">
        <div>
          <span style="color:var(--text)">${name}</span>
          <span style="color:var(--text-muted);font-size:10px;margin-left:6px">${team}</span>
        </div>
        <div style="text-align:right">
          <span style="color:var(--text-muted)">${overs}</span>
          <span style="color:var(--gold);margin-left:10px">${iv.runs}r</span>
        </div>
      </div>`;
  }).join('');
}

// ── RESULT BANNER ─────────────────────────────────────────────
function renderResult() {
  const banner = document.getElementById('resultBanner');

  if (!state.matchOver) {
    banner.classList.remove('show');
    document.getElementById('statusPill').innerHTML = '<span class="live-dot"></span> Live';
    document.getElementById('statusPill').className = 'pill pill-live';
    return;
  }

  banner.classList.add('show');
  document.getElementById('statusPill').textContent = 'Complete';
  document.getElementById('statusPill').className   = 'pill pill-done';

  const inn1  = state.inn[1];
  const inn2  = state.inn[2];
  const title = document.getElementById('resultTitle');
  const sub   = document.getElementById('resultSub');

  if (inn2.runs >= state.target) {
    title.textContent = 'Australia won!';
    sub.textContent   = `by ${10 - inn2.wickets} wicket(s)  ·  India ${inn1.runs}/${inn1.wickets}  vs  Australia ${inn2.runs}/${inn2.wickets}`;
  } else {
    title.textContent = 'India won!';
    sub.textContent   = `by ${inn1.runs - inn2.runs} run(s)  ·  India ${inn1.runs}/${inn1.wickets}  vs  Australia ${inn2.runs}/${inn2.wickets}`;
  }
}

// ── DIJKSTRA PLAYER LIST ──────────────────────────────────────
function updateDijkstraPlayerList() {
  const el    = document.getElementById('dijk-ids');
  const nodes = [...state.graph.allNodes];
  if (nodes.length === 0) { el.textContent = 'No partnerships yet.'; return; }
  el.textContent = 'Available IDs: ' + nodes.join(', ');
}

// ================================================================
//  ALGORITHM INTERACTION HANDLERS
// ================================================================

// Alg 2 — Segment Tree
function runSegTree() {
  const from = parseInt(document.getElementById('seg-from').value) || 1;
  const to   = parseInt(document.getElementById('seg-to').value)   || 6;
  const f    = Math.max(1, Math.min(from, 20));
  const t    = Math.max(f, Math.min(to, 20));

  const slot1Start = (f - 1), slot1End = (t - 1);
  const runs1 = state.segRuns.query(slot1Start, slot1End);
  const wkts1 = state.segWkts.query(slot1Start, slot1End);

  const slot2Start = 20 + (f - 1), slot2End = 20 + (t - 1);
  const runs2 = state.segRuns.query(slot2Start, slot2End);
  const wkts2 = state.segWkts.query(slot2Start, slot2End);

  let html = `<span class="muted">Query: overs ${f} → ${t}</span>\n\n`;
  html += `<span class="highlight">India (Inn 1)</span>  Runs: <span class="good">${runs1}</span>   Wkts: <span class="bad">${wkts1}</span>\n`;
  if (state.innings === 2) {
    html += `<span class="highlight">Australia (Inn 2)</span>  Runs: <span class="good">${runs2}</span>   Wkts: <span class="bad">${wkts2}</span>`;
  }

  document.getElementById('seg-result').innerHTML = html;
  renderSegChart(f, t);
}

function renderSegChart(from, to) {
  const chart = document.getElementById('seg-chart');
  chart.innerHTML = '';
  let maxRuns = 1;
  for (let i = 0; i < 40; i++) maxRuns = Math.max(maxRuns, state.segRuns.query(i, i));

  for (let ov = 0; ov < 20; ov++) {
    const runs   = state.segRuns.query(ov, ov);
    const wkts   = state.segWkts.query(ov, ov);
    const pct    = (runs / maxRuns) * 100;
    const inRange = (ov + 1 >= from && ov + 1 <= to);

    const wrap = document.createElement('div');
    wrap.className = 'bar-wrap';

    const bar = document.createElement('div');
    bar.className = 'bar' + (inRange ? ' highlighted' : '') + (wkts > 0 ? ' wicket-bar' : '');
    bar.style.height = `${Math.max(pct, 2)}%`;
    bar.title = `Over ${ov+1}: ${runs}r${wkts ? ', '+wkts+'W' : ''}`;

    const lbl = document.createElement('div');
    lbl.className   = 'bar-label';
    lbl.textContent = ov + 1;

    wrap.appendChild(bar);
    wrap.appendChild(lbl);
    chart.appendChild(wrap);
  }
}

// Alg 3 — Fenwick Tree
function runFenwick() {
  const ov = Math.max(1, Math.min(parseInt(document.getElementById('fenwick-over').value) || 1, 20));

  const runs1 = state.fenRuns.rangeSum(0, ov - 1);
  const rr1   = (runs1 / ov).toFixed(2);
  const runs2 = state.innings === 2 ? state.fenRuns.rangeSum(20, 20 + ov - 1) : 0;
  const rr2   = (runs2 / ov).toFixed(2);

  let html = `<span class="muted">Prefix sum after over ${ov}:</span>\n\n`;
  html += `<span class="highlight">India</span>  Runs: <span class="good">${runs1}</span>   RR: <span class="highlight">${rr1}</span>\n`;
  if (state.innings === 2) {
    html += `<span class="highlight">Australia</span>  Runs: <span class="good">${runs2}</span>   RR: <span class="highlight">${rr2}</span>`;
  }
  document.getElementById('fenwick-result').innerHTML = html;
}

// Alg 4 — Trie
function runTrie() {
  const prefix = document.getElementById('trie-input').value.trim();
  const el     = document.getElementById('trie-result');

  if (prefix.length < 1) {
    el.innerHTML = '<span style="color:var(--text-muted)">Start typing to search players.</span>';
    return;
  }

  const ids = state.trie.searchByPrefix(prefix);
  if (ids.length === 0) {
    el.innerHTML = `<span style="color:var(--text-muted)">No players found for "${prefix}".</span>`;
    return;
  }

  el.innerHTML = '';
  for (const id of ids) {
    const p  = PLAYERS[id];
    const b  = state.bat[id];
    const bw = state.bowl[id];
    const sr = b.balls > 0 ? (100 * b.runs / b.balls).toFixed(0) : '—';

    const div = document.createElement('div');
    div.className = 'player-card';
    div.innerHTML = `
      <div>
        <div class="player-card-name">${p.name}</div>
        <div class="player-card-team">${p.team} · ${p.role}</div>
        <div style="font-family:var(--mono);font-size:10px;color:var(--text-muted);margin-top:2px">ID: ${id}</div>
      </div>
      <div class="player-card-stats">
        ${b.didBat ? `<div>Bat: ${b.runs}(${b.balls}) SR:${sr}</div>` : '<div style="color:var(--text-muted)">DNB</div>'}
        ${bw.overs > 0 || bw.balls > 0 ? `<div>Bowl: ${bw.overs}.${bw.balls}-${bw.wickets}-${bw.runs}</div>` : ''}
      </div>
    `;
    el.appendChild(div);
  }
}

// Alg 5 — KMP
function runKMP() {
  const keyword = document.getElementById('kmp-input').value.trim();
  const el      = document.getElementById('kmp-result');

  if (!keyword) { el.innerHTML = '<span style="color:var(--text-muted)">Enter a keyword to search.</span>'; return; }

  const matches = state.commentary.filter(entry => KMP.search(entry.line, keyword).length > 0);
  if (matches.length === 0) {
    el.innerHTML = `<span style="color:var(--text-muted)">No matches for "${keyword}".</span>`;
    return;
  }

  const escaped = keyword.replace(/[.*+?^${}()|[\]\\]/g, '\\$&');
  const regex   = new RegExp(`(${escaped})`, 'gi');

  el.innerHTML = '';
  const count = document.createElement('div');
  count.innerHTML = `<span style="color:var(--good);font-size:10px;margin-bottom:6px;display:block">→ ${matches.length} result(s) via KMP</span>`;
  el.appendChild(count);

  for (const entry of [...matches].reverse()) {
    const highlighted = entry.line.replace(regex, '<span style="color:var(--gold);font-weight:500">$1</span>');
    const div = document.createElement('div');
    div.style.cssText = 'margin-bottom:6px;padding-bottom:6px;border-bottom:1px solid var(--border-light);line-height:1.5;font-size:11px;';
    div.innerHTML = highlighted;
    el.appendChild(div);
  }
}

// Alg 6 — Dijkstra
function runDijkstra() {
  const a  = document.getElementById('dijk-a').value.trim();
  const b  = document.getElementById('dijk-b').value.trim();
  const el = document.getElementById('dijk-result');

  if (!a || !b) { el.innerHTML = '<span style="color:var(--text-muted)">Enter two player IDs.</span>'; return; }
  if (!PLAYERS[a] || !PLAYERS[b]) { el.innerHTML = '<span style="color:var(--crimson)">Unknown player ID.</span>'; return; }

  const result = state.graph.dijkstra(a, b);
  if (result.path.length === 0) {
    el.innerHTML = `<span style="color:var(--text-muted)">No chain found between these players yet. Simulate more overs.</span>`;
    return;
  }

  const pathNames = result.path.map(id => PLAYERS[id]?.name || id);
  let html = `<span class="muted">Source: ${PLAYERS[a].name}</span>\n`;
  html += `<span class="muted">Dest: ${PLAYERS[b].name}</span>\n\n`;
  html += `<span class="highlight">Chain: </span>${pathNames.join(' → ')}\n\n`;
  html += `<span class="good">Strength: ${result.strength} runs</span>`;
  el.innerHTML = html;
}

// Alg 7 — BFS
function runBFS() {
  const pid = document.getElementById('bfs-input').value.trim();
  const el  = document.getElementById('bfs-result');

  if (!pid) { el.innerHTML = '<span style="color:var(--text-muted)">Enter a player ID.</span>'; return; }
  if (!PLAYERS[pid]) { el.innerHTML = '<span style="color:var(--crimson)">Unknown player ID.</span>'; return; }

  const reachable = state.graph.bfsReachable(pid);
  if (reachable.length === 0) {
    el.innerHTML = `<span style="color:var(--text-muted)">${PLAYERS[pid].name} has no partnerships yet.</span>`;
    return;
  }

  let html = `<span class="highlight">BFS from ${PLAYERS[pid].name}:</span>\n\n`;
  for (const id of reachable) {
    html += `  → <span class="good">${PLAYERS[id]?.name || id}</span>\n`;
  }
  el.innerHTML = html;
}

// Alg 7 — DFS
function runDFS() {
  const el       = document.getElementById('bfs-result');
  const clusters = state.graph.findBattingClusters().filter(c => c.length > 1);

  if (clusters.length === 0) {
    el.innerHTML = '<span style="color:var(--text-muted)">No clusters yet. Simulate more overs.</span>';
    return;
  }

  let html = `<span class="highlight">DFS Batting Clusters:</span>\n\n`;
  clusters.forEach((cluster, i) => {
    html += `<span class="muted">Cluster ${i + 1}:</span>\n`;
    cluster.forEach(id => { html += `  · <span class="good">${PLAYERS[id]?.name || id}</span>\n`; });
    html += '\n';
  });
  el.innerHTML = html;
}

// Alg 9 — Hash Table lookup
function runHashLookup() {
  const key = document.getElementById('hash-input').value.trim();
  const el  = document.getElementById('hash-result');

  if (!key) { el.innerHTML = '<span style="color:var(--text-muted)">Enter a player ID.</span>'; return; }

  const val = state.hashTable.get(key);
  if (!val) {
    el.innerHTML = `<span style="color:var(--crimson)">Key "${key}" not found in hash table.</span>`;
    return;
  }

  const bucket = state.hashTable._hash(key);
  const chain  = state.hashTable.buckets[bucket];

  let html = `<span class="muted">Key: ${key}</span>\n`;
  html += `<span class="muted">Hash → Bucket #${bucket}</span>\n`;
  html += `<span class="muted">Chain length: ${chain.length}</span>\n\n`;
  html += `<span class="highlight">${val.name}</span> (${val.team})\n`;
  html += `  Runs: <span class="good">${val.runs}</span>   Balls: ${val.balls}`;
  el.innerHTML = html;
}

// Alg 10 — Interval Tree query
function runIntervalQuery() {
  const over = parseInt(document.getElementById('interval-over').value) || 1;
  const ov   = Math.max(1, Math.min(over, 20)) - 1; // 0-indexed
  const el   = document.getElementById('interval-result');

  const results = state.intervalTree.query(ov, ov);

  if (results.length === 0) {
    el.innerHTML = `<span style="color:var(--text-muted)">No batting spells recorded at over ${over} yet.</span>`;
    return;
  }

  let html = `<span class="muted">Batsmen active at over ${over}:</span>\n\n`;
  for (const iv of results) {
    html += `  <span class="highlight">${PLAYERS[iv.playerId]?.name || iv.playerId}</span>`;
    html += `  overs ${iv.start+1}–${iv.end+1}  <span class="good">${iv.runs}r</span>\n`;
  }
  el.innerHTML = html;
}

// ── TABS ───────────────────────────────────────────────────────
function switchTab(group, activeId) {
  const activeEl = document.getElementById(activeId);
  const parent   = activeEl.closest('.card');
  parent.querySelectorAll('.tab-panel').forEach(p => p.classList.remove('active'));
  parent.querySelectorAll('.tab-btn').forEach(b => b.classList.remove('active'));
  activeEl.classList.add('active');
  event.target.classList.add('active');
}
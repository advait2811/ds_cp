// ================================================================
//  simulation.js
//  Cricket match simulation engine
// ================================================================

// ---- PLAYERS ----
const PLAYERS = {
  ind_rohit:    { name: 'Rohit Sharma',     team: 'IND', role: 'BAT' },
  ind_gill:     { name: 'Shubman Gill',     team: 'IND', role: 'BAT' },
  ind_kohli:    { name: 'Virat Kohli',      team: 'IND', role: 'BAT' },
  ind_surya:    { name: 'Suryakumar Yadav', team: 'IND', role: 'BAT' },
  ind_hardik:   { name: 'Hardik Pandya',    team: 'IND', role: 'ALL' },
  ind_jadeja:   { name: 'Ravindra Jadeja',  team: 'IND', role: 'ALL' },
  ind_karthik:  { name: 'Dinesh Karthik',   team: 'IND', role: 'WK'  },
  ind_ashwin:   { name: 'R Ashwin',         team: 'IND', role: 'BWL' },
  ind_bumrah:   { name: 'Jasprit Bumrah',   team: 'IND', role: 'BWL' },
  ind_shami:    { name: 'Mohammed Shami',   team: 'IND', role: 'BWL' },
  ind_arshdeep: { name: 'Arshdeep Singh',   team: 'IND', role: 'BWL' },

  aus_warner:   { name: 'David Warner',     team: 'AUS', role: 'BAT' },
  aus_finch:    { name: 'Aaron Finch',      team: 'AUS', role: 'BAT' },
  aus_smith:    { name: 'Steve Smith',      team: 'AUS', role: 'BAT' },
  aus_maxwell:  { name: 'Glenn Maxwell',    team: 'AUS', role: 'ALL' },
  aus_stoinis:  { name: 'Marcus Stoinis',   team: 'AUS', role: 'ALL' },
  aus_inglis:   { name: 'Josh Inglis',      team: 'AUS', role: 'WK'  },
  aus_timdavid: { name: 'Tim David',        team: 'AUS', role: 'BAT' },
  aus_starc:    { name: 'Mitchell Starc',   team: 'AUS', role: 'BWL' },
  aus_hazel:    { name: 'Josh Hazlewood',   team: 'AUS', role: 'BWL' },
  aus_cummins:  { name: 'Pat Cummins',      team: 'AUS', role: 'BWL' },
  aus_zampa:    { name: 'Adam Zampa',       team: 'AUS', role: 'BWL' },
};

const IND_ORDER   = ['ind_rohit','ind_gill','ind_kohli','ind_surya','ind_hardik','ind_jadeja','ind_karthik','ind_ashwin','ind_bumrah','ind_shami','ind_arshdeep'];
const AUS_ORDER   = ['aus_warner','aus_finch','aus_smith','aus_maxwell','aus_stoinis','aus_inglis','aus_timdavid','aus_starc','aus_hazel','aus_cummins','aus_zampa'];
const AUS_BOWLERS = ['aus_cummins','aus_hazel','aus_starc','aus_zampa','aus_maxwell','aus_stoinis'];
const IND_BOWLERS = ['ind_bumrah','ind_shami','ind_arshdeep','ind_jadeja','ind_ashwin','ind_hardik'];

// ---- MATCH STATE ----
let state = {};

function initState() {
  state = {
    innings: 1,
    target: 0,
    matchOver: false,

    bat:   {},
    bowl:  {},
    field: {},

    inn: [
      null,
      { runs: 0, wickets: 0, legalBalls: 0, extras: 0, wides: 0, noBalls: 0, byes: 0, legByes: 0 },
      { runs: 0, wickets: 0, legalBalls: 0, extras: 0, wides: 0, noBalls: 0, byes: 0, legByes: 0 }
    ],

    striker: IND_ORDER[0],
    nonStriker: IND_ORDER[1],
    nextBatIndex: { IND: 2, AUS: 2 },

    partRuns: 0,
    partBalls: 0,

    commentary: [],

    // Algorithms
    heap:       new MinHeap(),
    segRuns:    new SegmentTree(40),
    segWkts:    new SegmentTree(40),
    fenRuns:    new FenwickTree(40),
    trie:       new Trie(),
    graph:      new PartnershipGraph(),
    avl:        new AVLTree(),
    hashTable:  new HashTable(16),
    intervalTree: new IntervalTree(),

    // For interval tree: track when each batsman first came to crease
    batSpellStart: {},   // { playerId: overNumber }
    batSpellRuns:  {},   // { playerId: runsAtStart }
  };

  for (const id of Object.keys(PLAYERS)) {
    state.bat[id]   = { runs: 0, balls: 0, fours: 0, sixes: 0, isOut: false, didBat: false, dismissal: 'not out' };
    state.bowl[id]  = { overs: 0, balls: 0, maidens: 0, runs: 0, wickets: 0, wides: 0, noBalls: 0, runsThisOver: 0 };
    state.field[id] = { catches: 0, runOuts: 0, stumpings: 0 };
  }

  state.bat[IND_ORDER[0]].didBat = true;
  state.bat[IND_ORDER[1]].didBat = true;

  // Trie
  for (const [id, p] of Object.entries(PLAYERS)) {
    state.trie.insert(p.name, id);
  }

  // Hash table: seed all players
  for (const [id, p] of Object.entries(PLAYERS)) {
    state.hashTable.set(id, { name: p.name, team: p.team, runs: 0, balls: 0 });
  }

  // Track openers' spell start
  state.batSpellStart[IND_ORDER[0]] = 0;
  state.batSpellStart[IND_ORDER[1]] = 0;
  state.batSpellRuns[IND_ORDER[0]]  = 0;
  state.batSpellRuns[IND_ORDER[1]]  = 0;

  renderAll();
}

// ---- BALL GENERATOR ----
// Fixed: chasing team no longer has unfair advantage — both innings use same base probs.
// Chase pressure only slightly increases risk (more wicket chance too), not free runs.
function generateBall(overNumber, batsmanRuns, bowlerWickets, isChasing, reqRate, isFast) {
  // Base phase probabilities
  let dot, single, two, three, four, six, wicket, bye;

  if (overNumber <= 5) {
    // Powerplay
    dot=0.28; single=0.28; two=0.10; three=0.02; four=0.16; six=0.07; wicket=0.08; bye=0.01;
  } else if (overNumber >= 15) {
    // Death
    dot=0.22; single=0.22; two=0.09; three=0.02; four=0.16; six=0.14; wicket=0.13; bye=0.02;
  } else {
    // Middle
    dot=0.33; single=0.30; two=0.12; three=0.02; four=0.11; six=0.05; wicket=0.06; bye=0.01;
  }

  // Batsman confidence
  if (batsmanRuns >= 50)      { six+=0.04; four+=0.02; dot-=0.03; single-=0.03; }
  else if (batsmanRuns <= 5)  { dot+=0.05; single+=0.02; four-=0.02; six-=0.02; wicket+=0.02; }

  // Dangerous bowler
  if (bowlerWickets >= 2)     { wicket+=0.04; dot+=0.02; six-=0.01; }

  // Chase pressure: slightly more attacking but also riskier — net effect neutral
  // Removed the unbalanced bonus that gave chasing team free runs
  if (isChasing && reqRate > 12) {
    // Desperate chase: more attacking shots but also much riskier
    six+=0.03; four+=0.02; wicket+=0.04; dot-=0.02; single-=0.03;
  } else if (isChasing && reqRate > 8) {
    // Moderate pressure: slightly more aggressive, proportionally more risk
    six+=0.01; four+=0.01; wicket+=0.02; dot-=0.01; single-=0.01;
  }
  // If reqRate is low (AUS cruising), no bonus — keep it fair

  // Bowler type
  if (isFast) { wicket+=0.02; dot+=0.01; six-=0.01; }
  else        { wicket+=0.01; two+=0.01; }

  // Extras
  const wideChance   = 0.06;
  const noBallChance = 0.03;

  let r = Math.random();

  if (r < wideChance)  return { type:'WIDE',  runs:0, extras:1, wicketType:'', commentary:'Wide ball — down the leg side.' };
  r -= wideChance;
  if (r < noBallChance) {
    const nbRuns = Math.floor(Math.random() * 4);
    return { type:'NOBALL', runs:nbRuns, extras:1, wicketType:'', commentary:`No ball! +${1+nbRuns} extras.` };
  }
  r -= noBallChance;

  // Normalise
  dot=Math.max(dot,0.05); single=Math.max(single,0.05); four=Math.max(four,0.02);
  six=Math.max(six,0.01); wicket=Math.max(wicket,0.02);

  const total = dot+single+two+three+four+six+wicket+bye;
  dot/=total; single/=total; two/=total; three/=total; four/=total; six/=total; wicket/=total; bye/=total;

  let cum = 0;
  cum += dot;    if (r < cum) return { type:'DOT',    runs:0, extras:0, wicketType:'', commentary: pick(['Dot ball. Defended solidly.','Good length — no shot offered.','Beaten outside off!','Pushed to mid-on.']) };
  cum += single; if (r < cum) return { type:'RUN',    runs:1, extras:0, wicketType:'', commentary: pick(['Nudged to mid-wicket, one.','Quick single to fine leg.','Pushed through covers.','Flicked to square leg.']) };
  cum += two;    if (r < cum) return { type:'RUN',    runs:2, extras:0, wicketType:'', commentary: pick(['Driven to deep cover, two.','Punched through the gap, two.','Cut to deep point, two.']) };
  cum += three;  if (r < cum) return { type:'RUN',    runs:3, extras:0, wicketType:'', commentary: 'Good running — three!' };
  cum += four;   if (r < cum) return { type:'FOUR',   runs:4, extras:0, wicketType:'', commentary: pick(['FOUR! Driven beautifully!','FOUR! Cut hard past point!','FOUR! Flicked off the pads!','FOUR! Pulled powerfully!','FOUR! Whipped through covers!']) };
  cum += six;    if (r < cum) return { type:'SIX',    runs:6, extras:0, wicketType:'', commentary: pick(['SIX! Launched into the stands!','SIX! Cleared the rope!','SIX! Lofted down the ground!','SIX! Slog sweep, maximum!']) };
  cum += wicket; if (r < cum) {
    const wt = pickWicketType(isFast);
    return { type:'WICKET', runs:0, extras:0, wicketType:wt, commentary: wicketCommentary(wt) };
  }
  const byeRuns = Math.random() < 0.5 ? 1 : 2;
  return { type:'BYE', runs:0, extras:byeRuns, wicketType:'', commentary:`Bye! Missed by keeper — ${byeRuns} run(s).` };
}

function pick(arr)  { return arr[Math.floor(Math.random() * arr.length)]; }

function pickWicketType(isFast) {
  const r = Math.random();
  if (isFast) {
    if (r < 0.25) return 'BOWLED';
    if (r < 0.55) return 'CAUGHT';
    if (r < 0.70) return 'LBW';
    if (r < 0.85) return 'CAUGHT';
    return 'RUN_OUT';
  } else {
    if (r < 0.20) return 'BOWLED';
    if (r < 0.50) return 'CAUGHT';
    if (r < 0.65) return 'LBW';
    if (r < 0.80) return 'STUMPED';
    return 'CAUGHT';
  }
}

function wicketCommentary(wt) {
  if (wt === 'BOWLED')  return pick(['WICKET! Bowled — stumps shattered!','OUT! Through the gate!','GONE! Off stump knocked back!']);
  if (wt === 'CAUGHT')  return pick(['WICKET! Caught — mistimed drive!','OUT! Caught behind!','CAUGHT! Top edge, taken at fine leg.']);
  if (wt === 'LBW')     return pick(['OUT! LBW — plumb in front!','GIVEN! LBW — missing leg.','OUT! LBW — skidded on.']);
  if (wt === 'STUMPED') return pick(['STUMPED! Danced down and missed!','OUT! Stumped — lost his ground.']);
  if (wt === 'RUN_OUT') return pick(['RUN OUT! Direct hit — terrible mix-up!','RUN OUT! Brilliant fielding!']);
  return 'OUT!';
}

// ---- MATCH HELPERS ----
function liveInn()          { return state.inn[state.innings]; }
function battingTeam()      { return state.innings === 1 ? IND_ORDER : AUS_ORDER; }
function fieldingBowlers()  { return state.innings === 1 ? AUS_BOWLERS : IND_BOWLERS; }
function battingTeamId()    { return state.innings === 1 ? 'IND' : 'AUS'; }
function overSlot(over)     { return (state.innings - 1) * 20 + over; }

function selectBowler(overNumber) {
  const bowlers = fieldingBowlers();
  return bowlers[overNumber % bowlers.length];
}

function isFastBowler(id) {
  return ['ind_bumrah','ind_shami','ind_arshdeep','aus_starc','aus_hazel','aus_cummins'].includes(id);
}

function getFielder(bowlerId) {
  const ft = state.innings === 1 ? AUS_ORDER : IND_ORDER;
  const candidates = ft.filter(id => id !== bowlerId);
  return candidates[Math.floor(Math.random() * candidates.length)];
}

function getWicketkeeper() { return state.innings === 1 ? 'aus_inglis' : 'ind_karthik'; }

function getNextBatsman() {
  const teamId = battingTeamId();
  const order  = state.innings === 1 ? IND_ORDER : AUS_ORDER;
  const idx    = state.nextBatIndex[teamId];
  if (idx >= order.length) return null;
  state.nextBatIndex[teamId]++;
  return order[idx];
}

function isInningsOver() {
  const inn = liveInn();
  if (inn.wickets >= 10) return true;
  if (inn.legalBalls >= 120) return true;
  if (state.innings === 2 && inn.runs >= state.target) return true;
  return false;
}

// ---- APPLY ONE DELIVERY ----
function applyDelivery(ball) {
  const inn        = liveInn();
  const bowlerStat = state.bowl[ball.bowlerId];
  const batStat    = state.bat[ball.batsmanId];
  const isLegal    = (ball.type !== 'WIDE' && ball.type !== 'NOBALL');
  const totalRuns  = ball.runs + ball.extras;
  const slot       = overSlot(ball.over);

  inn.runs    += totalRuns;
  inn.extras  += ball.extras;
  if (ball.type === 'WIDE')   inn.wides   += ball.extras;
  if (ball.type === 'NOBALL') inn.noBalls += ball.extras;
  if (ball.type === 'BYE')    inn.byes    += ball.extras;

  if (isLegal) { inn.legalBalls++; state.partBalls++; }

  batStat.didBat = true;
  if (isLegal) batStat.balls++;
  if (ball.type !== 'WIDE' && ball.type !== 'BYE') {
    batStat.runs += ball.runs;
    state.partRuns += ball.runs;
    if (ball.type === 'FOUR') batStat.fours++;
    if (ball.type === 'SIX')  batStat.sixes++;
  }

  if (ball.type === 'WIDE') {
    bowlerStat.wides++;
    bowlerStat.runs += ball.extras;
    bowlerStat.runsThisOver += ball.extras;
  } else if (ball.type === 'NOBALL') {
    bowlerStat.noBalls++;
    bowlerStat.runs += totalRuns;
    bowlerStat.runsThisOver += totalRuns;
  } else {
    bowlerStat.runs += totalRuns;
    bowlerStat.runsThisOver += totalRuns;
    bowlerStat.balls++;
    if (bowlerStat.balls === 6) {
      if (bowlerStat.runsThisOver === 0) bowlerStat.maidens++;
      bowlerStat.overs++;
      bowlerStat.balls = 0;
      bowlerStat.runsThisOver = 0;
    }
  }

  if (ball.type === 'WICKET' && ball.wicketType !== 'RUN_OUT') {
    bowlerStat.wickets++;
  }

  // Alg 2: Segment Tree
  state.segRuns.update(slot, totalRuns);
  if (ball.type === 'WICKET') state.segWkts.update(slot, 1);

  // Alg 3: Fenwick Tree
  state.fenRuns.update(slot, totalRuns);

  // Alg 8: AVL Tree leaderboard
  const p = PLAYERS[ball.batsmanId];
  if (p && batStat.didBat) {
    state.avl.insert(ball.batsmanId, {
      playerId: ball.batsmanId,
      name: p.name,
      team: p.team,
      runs: batStat.runs,
      balls: batStat.balls,
      fours: batStat.fours,
      sixes: batStat.sixes,
    });
  }

  // Alg 9: Hash Table update
  state.hashTable.set(ball.batsmanId, {
    name: PLAYERS[ball.batsmanId]?.name,
    team: PLAYERS[ball.batsmanId]?.team,
    runs: batStat.runs,
    balls: batStat.balls,
  });

  // Handle wicket
  if (ball.type === 'WICKET') {
    // Alg 10: Record batting spell in interval tree
    const spellStart = state.batSpellStart[ball.batsmanId] ?? ball.over;
    state.intervalTree.insert(new Interval(
      spellStart, ball.over, ball.batsmanId, batStat.runs
    ));
    delete state.batSpellStart[ball.batsmanId];

    if (state.partRuns > 0) {
      state.graph.addPartnership(state.striker, state.nonStriker, state.partRuns);
    }
    state.partRuns = 0; state.partBalls = 0;

    const bowlerName = PLAYERS[ball.bowlerId]?.name || ball.bowlerId;
    let dismissal = '';
    if      (ball.wicketType === 'BOWLED')   { dismissal = `b ${bowlerName}`; }
    else if (ball.wicketType === 'LBW')      { dismissal = `lbw b ${bowlerName}`; }
    else if (ball.wicketType === 'CAUGHT')   {
      const fielder = getFielder(ball.bowlerId);
      state.field[fielder].catches++;
      dismissal = `c ${PLAYERS[fielder]?.name||fielder} b ${bowlerName}`;
    } else if (ball.wicketType === 'STUMPED') {
      const keeper = getWicketkeeper();
      state.field[keeper].stumpings++;
      dismissal = `st ${PLAYERS[keeper]?.name||keeper} b ${bowlerName}`;
    } else if (ball.wicketType === 'RUN_OUT') { dismissal = 'run out'; }

    batStat.isOut = true; batStat.dismissal = dismissal;
    inn.wickets++;

    const next = getNextBatsman();
    if (next && inn.wickets < 10) {
      if (state.striker === ball.batsmanId) state.striker    = next;
      else                                   state.nonStriker = next;
      state.bat[next].didBat = true;

      // Track new batsman's spell start
      state.batSpellStart[next] = ball.over;
    }
  }

  // Strike rotation
  if (ball.type !== 'WIDE' && ball.type !== 'NOBALL') {
    if (ball.runs % 2 === 1) [state.striker, state.nonStriker] = [state.nonStriker, state.striker];
  }

  // End of over: swap ends
  if (isLegal && inn.legalBalls % 6 === 0) {
    [state.striker, state.nonStriker] = [state.nonStriker, state.striker];
  }

  // Commentary
  const overLabel = `${ball.over + 1}.${ball.ballInOver}`;
  const logLine = `[${overLabel}] ${ball.commentary}  |  ${inn.runs}/${inn.wickets} (${Math.floor(inn.legalBalls/6)}.${inn.legalBalls%6} ov)`;
  state.commentary.push({ line: logLine, type: ball.type, over: ball.over, ball: ball.ballInOver });
}

// ---- SIMULATE N OVERS ----
function simulateOvers(forceN) {
  if (state.matchOver) { alert('Match is already over!'); return; }

  if (state.innings === 1 && isInningsOver()) startInnings2();
  if (state.matchOver) return;

  const n = forceN || parseInt(document.getElementById('oversInput').value) || 1;
  let oversSimulated = 0;

  while (oversSimulated < n) {
    if (isInningsOver()) break;

    const inn         = liveInn();
    const currentOver = Math.floor(inn.legalBalls / 6);
    const bowlerId    = selectBowler(currentOver);
    const isFast      = isFastBowler(bowlerId);
    let legalThisOver = 0;

    while (legalThisOver < 6) {
      if (isInningsOver()) break;

      const batStat    = state.bat[state.striker];
      const bowlerStat = state.bowl[bowlerId];
      const inn2       = liveInn();
      const isChasing  = state.innings === 2;
      const totalOvers = 20;
      const reqRate    = isChasing && state.target > 0
        ? (state.target - inn2.runs) / Math.max(1, (totalOvers * 6 - inn2.legalBalls) / 6)
        : 0;

      const delivery = generateBall(currentOver, batStat.runs, bowlerStat.wickets, isChasing, reqRate, isFast);

      const ball = {
        over: currentOver,
        ballInOver: legalThisOver + 1,
        batsmanId: state.striker,
        bowlerId,
        ...delivery
      };

      // Alg 1: Heap (push + immediate pop — demonstrates ordering)
      state.heap.push(ball);
      const processed = state.heap.pop();
      applyDelivery(processed);

      if (delivery.type !== 'WIDE' && delivery.type !== 'NOBALL') legalThisOver++;

      // End innings 2 immediately when target reached or overs/wickets done
      if (state.innings === 2 && isInningsOver()) {
        state.matchOver = true; break;
      }
    }

    oversSimulated++;
  }

  // The live (unfinished) partnership is NOT added to the graph here —
  // that would accumulate on every simulate click. It's shown separately in renderPartnershipList.
  // Partnerships are only committed to the graph when a wicket falls (in applyDelivery).

  if (state.innings === 1 && isInningsOver()) {
    // Seal the final unfinished partnership at innings end
    if (state.partRuns > 0) {
      state.graph.addPartnership(state.striker, state.nonStriker, state.partRuns);
      state.partRuns = 0; state.partBalls = 0;
    }
    for (const [pid, startOver] of Object.entries(state.batSpellStart)) {
      const currentOver = Math.floor(liveInn().legalBalls / 6);
      state.intervalTree.insert(new Interval(startOver, currentOver, pid, state.bat[pid].runs));
    }
    state.batSpellStart = {};
    startInnings2();
  }

  if (state.matchOver) {
    // Seal partnership and spells for innings 2 end
    if (state.partRuns > 0) {
      state.graph.addPartnership(state.striker, state.nonStriker, state.partRuns);
      state.partRuns = 0;
    }
    for (const [pid, startOver] of Object.entries(state.batSpellStart)) {
      const currentOver = Math.floor(liveInn().legalBalls / 6);
      state.intervalTree.insert(new Interval(startOver, currentOver, pid, state.bat[pid].runs));
    }
    state.batSpellStart = {};
  }

  renderAll();
}

function startInnings2() {
  state.innings     = 2;
  state.target      = state.inn[1].runs + 1;
  state.striker     = AUS_ORDER[0];
  state.nonStriker  = AUS_ORDER[1];
  state.bat[AUS_ORDER[0]].didBat = true;
  state.bat[AUS_ORDER[1]].didBat = true;
  state.partRuns = 0; state.partBalls = 0;

  // Track openers' spell start for innings 2
  state.batSpellStart[AUS_ORDER[0]] = 0;
  state.batSpellStart[AUS_ORDER[1]] = 0;
}

function resetMatch() {
  initState();
}

function runRate(inn) {
  const overs = inn.legalBalls / 6;
  return overs > 0 ? inn.runs / overs : 0;
}
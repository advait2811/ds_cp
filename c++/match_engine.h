#pragma once
#include "models.h"
#include "algorithms.h"
#include "ball_generator.h"

// ================================================================
//  MATCH ENGINE
//
//  Manages a full T20 cricket match:
//    - Proper cricket rules (legal balls, over counting, all-out)
//    - Random ball generation
//    - Interactive mid-match analysis using all 10 algorithms
//    - Real-time scorecard dashboard
// ================================================================

class MatchEngine {
private:
    // ---- Teams ----
    Team team1;   // bats first
    Team team2;   // bats second

    // ---- Match state ----
    int          totalOvers;
    int          inningsNumber;  // 1 or 2
    int          targetRuns;

    InningsState innings1;
    InningsState innings2;
    InningsState* liveInnings;

    // ---- Live crease state ----
    string strikerID;
    string nonStrikerID;
    string currentBowlerID;

    int currentPartnershipRuns  = 0;
    int currentPartnershipBalls = 0;

    // ---- Algorithm 1: Min-Heap ----
    BallQueue ballQueue;

    // ---- Algorithm 2: Segment Tree ----
    // Slots 0-19 = innings 1, slots 20-39 = innings 2
    SegmentTree runsPerOver;
    SegmentTree wicketsPerOver;

    // ---- Algorithm 3: Fenwick Tree ----
    FenwickTree runsPrefix;
    FenwickTree ballsPrefix;

    // ---- Algorithm 4: Trie ----
    Trie playerTrie;

    // ---- Algorithm 5: KMP ----
    vector<string> commentaryLog;

    // ---- Algorithm 6 & 7: Dijkstra + BFS + DFS ----
    PartnershipGraph partnershipGraph;

    // ---- Algorithm 8: AVL Tree leaderboard ----
    AVLTree battingLeaderboard;

    // ---- Algorithm 9: Hash Table ----
    HashTable playerStatCache;

    // ---- Algorithm 10: Interval Tree ----
    // Tracks batting spells: [entryOver, dismissalOver]
    IntervalTree spellTree;
    // entryOver for each batsman currently at the crease
    unordered_map<string, int> batsmanEntryOver;

    // ---- Ball generator (seeded mt19937 — NO rand() anywhere) ----
    BallGenerator generator;
    mt19937       rng;  // used for fielder selection

    // ---- Display helpers ----
    string sep(int width = 65) const {
        return string(width, '-');
    }
    string doubleSep(int width = 65) const {
        return string(width, '=');
    }
    void printHeader(const string& title) const {
        cout << "\n" << doubleSep() << "\n";
        cout << "  " << title << "\n";
        cout << doubleSep() << "\n";
    }

    int getOverSlot(int overNumber, int innings) const {
        return (innings - 1) * 20 + overNumber;
    }

    // ----------------------------------------------------------------
    //  Sync a player's latest stats into both the AVL leaderboard
    //  and the Hash Table (call after every legal delivery).
    // ----------------------------------------------------------------
    void syncPlayerStats(const string& playerId, Team& battingTeam, Team& fieldingTeam) {
        // ---- AVL leaderboard (batting) ----
        Player* bp = battingTeam.getPlayer(playerId);
        if (bp != nullptr && bp->bat.didBat) {
            AVLNodeData d;
            d.playerId   = bp->id;
            d.name       = bp->name;
            d.teamId     = bp->teamId;
            d.runs       = bp->bat.runs;
            d.balls      = bp->bat.ballsFaced;
            battingLeaderboard.upsert(d);
        }
        // ---- Hash Table (combined batting + bowling stats) ----
        // Update batting side
        if (bp != nullptr) {
            PlayerStatEntry entry;
            entry.playerId = bp->id;
            entry.name     = bp->name;
            entry.teamId   = bp->teamId;
            entry.runs     = bp->bat.runs;
            entry.balls    = bp->bat.ballsFaced;
            // Keep bowling stats from cache if they exist
            PlayerStatEntry* existing = playerStatCache.get(bp->id);
            entry.wickets  = existing ? existing->wickets : 0;
            entry.economy  = existing ? existing->economy : 0.0;
            playerStatCache.set(bp->id, entry);
        }
        // Update bowling side for current bowler
        Player* bwl = fieldingTeam.getPlayer(currentBowlerID);
        if (bwl != nullptr) {
            PlayerStatEntry entry;
            entry.playerId = bwl->id;
            entry.name     = bwl->name;
            entry.teamId   = bwl->teamId;
            entry.wickets  = bwl->bowl.wickets;
            entry.economy  = bwl->bowl.economy();
            PlayerStatEntry* existing = playerStatCache.get(bwl->id);
            entry.runs  = existing ? existing->runs  : 0;
            entry.balls = existing ? existing->balls : 0;
            playerStatCache.set(bwl->id, entry);
        }
    }

    // ----------------------------------------------------------------
    //  Apply a generated delivery to the match state.
    // ----------------------------------------------------------------
    void applyDelivery(const BallEvent& event) {
        Team& battingTeam  = (inningsNumber == 1) ? team1 : team2;
        Team& fieldingTeam = (inningsNumber == 1) ? team2 : team1;

        Player* striker = battingTeam.getPlayer(event.batsmanId);
        Player* bowler  = fieldingTeam.getPlayer(event.bowlerId);

        bool isLegalBall = (event.type != "WIDE" && event.type != "NOBALL");
        int  over        = event.over;
        int  overSlot    = getOverSlot(over, inningsNumber);
        int  totalRuns   = event.runsScored + event.extras;

        // ---- Innings totals ----
        liveInnings->totalRuns += totalRuns;
        liveInnings->extras    += event.extras;
        if (event.type == "WIDE")   { liveInnings->wides   += event.extras; }
        if (event.type == "NOBALL") { liveInnings->noBalls += event.extras; }
        if (event.type == "BYE")    { liveInnings->byes    += event.extras; }
        if (event.type == "LEGBYE") { liveInnings->legByes += event.extras; }

        // ---- Legal ball count ----
        if (isLegalBall) {
            liveInnings->legalBalls++;
            currentPartnershipBalls++;
        }

        // ---- Batsman stats ----
        if (striker != nullptr) {
            striker->bat.didBat = true;
            if (isLegalBall) {
                striker->bat.ballsFaced++;
            }
            if (event.type != "WIDE" && event.type != "BYE" && event.type != "LEGBYE") {
                striker->bat.runs += event.runsScored;
                currentPartnershipRuns += event.runsScored;
                if (event.type == "FOUR") { striker->bat.fours++; }
                if (event.type == "SIX")  { striker->bat.sixes++; }
            }
        }

        // ---- Bowler stats ----
        if (bowler != nullptr) {
            if (event.type == "WIDE") {
                bowler->bowl.recordWide(event.extras);
            } else if (event.type == "NOBALL") {
                bowler->bowl.recordNoBall(event.extras);
                bowler->bowl.runsConceded += event.runsScored;
            } else {
                bowler->bowl.recordLegalBall(totalRuns);
            }
            if (event.type == "WICKET" && event.wicketType != "RUN_OUT") {
                bowler->bowl.wickets++;
            }
        }

        // ---- Algorithm 2: Segment Tree ----
        runsPerOver.update(overSlot, totalRuns);
        if (event.type == "WICKET") {
            wicketsPerOver.update(overSlot, 1);
        }

        // ---- Algorithm 3: Fenwick Tree ----
        runsPrefix.update(overSlot, totalRuns);
        if (isLegalBall) {
            ballsPrefix.update(overSlot, 1);
        }

        // ---- Handle wicket ----
        if (event.type == "WICKET") {
            handleWicket(event, battingTeam, fieldingTeam);
        }

        // ---- Strike rotation ----
        if (isLegalBall) {
            if (event.runsScored % 2 == 1) {
                swap(strikerID, nonStrikerID);
            }
        }

        // ---- Algorithm 8 & 9: sync stats ----
        if (striker != nullptr) {
            syncPlayerStats(striker->id, battingTeam, fieldingTeam);
        }

        // ---- Commentary log (Algorithm 5: KMP) ----
        string overLabel = to_string(over + 1) + "." + to_string(event.ballInOver);
        string logLine   = "[" + overLabel + "] " + event.commentary
                         + "  |  " + liveInnings->scoreString()
                         + " " + liveInnings->overString();
        commentaryLog.push_back(logLine);

        cout << "  " << logLine << "\n";
    }

    // ----------------------------------------------------------------
    //  Handle a wicket.
    // ----------------------------------------------------------------
    void handleWicket(const BallEvent& event, Team& battingTeam, Team& fieldingTeam) {
        Player* outBatsman = battingTeam.getPlayer(event.batsmanId);
        Player* bowler     = fieldingTeam.getPlayer(event.bowlerId);

        // Save partnership before resetting
        if (!strikerID.empty() && !nonStrikerID.empty() && currentPartnershipRuns > 0) {
            partnershipGraph.addPartnership(strikerID, nonStrikerID, currentPartnershipRuns);
        }
        currentPartnershipRuns  = 0;
        currentPartnershipBalls = 0;

        // ---- Algorithm 10: close the dismissed batsman's spell ----
        if (outBatsman != nullptr) {
            int entryOver = 1;
            auto it = batsmanEntryOver.find(outBatsman->id);
            if (it != batsmanEntryOver.end()) {
                entryOver = it->second;
                batsmanEntryOver.erase(it);
            }
            int dismissalOver = liveInnings->currentOverNumber() + 1; // 1-indexed
            BattingInterval spell;
            spell.start    = entryOver;
            spell.end      = dismissalOver;
            spell.playerId = outBatsman->id;
            spell.runs     = outBatsman->bat.runs;
            spellTree.insert(spell);
        }

        if (outBatsman != nullptr) {
            outBatsman->bat.isOut = true;
        }

        // Build dismissal string
        string dismissal = "";
        if (event.wicketType == "BOWLED") {
            dismissal = "b " + (bowler ? bowler->name : event.bowlerId);
        } else if (event.wicketType == "CAUGHT") {
            string fielderName = pickRandomFielder(fieldingTeam, event.bowlerId);
            dismissal = "c " + fielderName + " b " + (bowler ? bowler->name : event.bowlerId);
        } else if (event.wicketType == "LBW") {
            dismissal = "lbw b " + (bowler ? bowler->name : event.bowlerId);
        } else if (event.wicketType == "STUMPED") {
            string keeperName = getWicketkeeper(fieldingTeam);
            dismissal = "st " + keeperName + " b " + (bowler ? bowler->name : event.bowlerId);
            Player* keeper = findWicketkeeper(fieldingTeam);
            if (keeper != nullptr) {
                keeper->field.stumpings++;
            }
        } else if (event.wicketType == "RUN_OUT") {
            dismissal = "run out";
        } else {
            dismissal = event.wicketType;
        }

        if (outBatsman != nullptr) {
            outBatsman->bat.dismissalText = dismissal;
        }

        liveInnings->totalWickets++;

        // Bring in next batsman
        string nextBatsman = battingTeam.getNextBatsman();
        if (!nextBatsman.empty() && liveInnings->totalWickets < 10) {
            if (strikerID == event.batsmanId) {
                strikerID = nextBatsman;
            } else {
                nonStrikerID = nextBatsman;
            }
            Player* incoming = battingTeam.getPlayer(nextBatsman);
            if (incoming != nullptr) {
                incoming->bat.didBat = true;
            }
            // ---- Algorithm 10: record entry over for new batsman ----
            int entryOver = liveInnings->currentOverNumber() + 1;
            batsmanEntryOver[nextBatsman] = entryOver;

            cout << "  *** " << (incoming ? incoming->name : nextBatsman)
                 << " comes in at number " << (liveInnings->totalWickets + 1) << " ***\n";
        }
    }

    // ---- Pick a random fielder (uses seeded rng — NOT rand()) ----
    string pickRandomFielder(Team& team, const string& bowlerId) {
        vector<string> candidates;
        for (auto& entry : team.players) {
            if (entry.first != bowlerId) {
                candidates.push_back(entry.second.name);
            }
        }
        if (candidates.empty()) { return "fielder"; }
        uniform_int_distribution<int> dist(0, (int)candidates.size() - 1);
        return candidates[dist(rng)];
    }

    string getWicketkeeper(Team& team) {
        for (auto& entry : team.players) {
            if (entry.second.role == Role::WICKETKEEPER) {
                return entry.second.name;
            }
        }
        return "keeper";
    }

    Player* findWicketkeeper(Team& team) {
        for (auto& entry : team.players) {
            if (entry.second.role == Role::WICKETKEEPER) {
                return &entry.second;
            }
        }
        return nullptr;
    }

    string selectBowler(Team& fieldingTeam, int overNumber) {
        vector<string> bowlerIds;
        for (auto& entry : fieldingTeam.players) {
            Role r = entry.second.role;
            if (r == Role::BOWLER || r == Role::ALLROUNDER) {
                bowlerIds.push_back(entry.first);
            }
        }
        sort(bowlerIds.begin(), bowlerIds.end());
        int index = overNumber % (int)bowlerIds.size();
        currentBowlerID = bowlerIds[index];
        return bowlerIds[index];
    }

    bool isFastBowler(const string& bowlerId, Team& team) {
        Player* p = team.getPlayer(bowlerId);
        if (p == nullptr) { return true; }
        for (int i = 0; i < (int)team.battingOrder.size(); i++) {
            if (team.battingOrder[i] == bowlerId) {
                return (i % 2 == 0);
            }
        }
        return true;
    }

public:
    MatchEngine(Team t1, Team t2, int overs = 20)
        : team1(t1), team2(t2),
          totalOvers(overs),
          inningsNumber(1),
          targetRuns(0),
          runsPerOver(40),
          wicketsPerOver(40),
          runsPrefix(40),
          ballsPrefix(40)
    {
        random_device rd;
        rng = mt19937(rd());

        liveInnings = &innings1;

        // ---- Algorithm 4: Build Trie ----
        for (auto& entry : team1.players) {
            playerTrie.insert(entry.second.name, entry.second.id);
        }
        for (auto& entry : team2.players) {
            playerTrie.insert(entry.second.name, entry.second.id);
        }

        // Initialise openers
        strikerID    = team1.getNextBatsman();
        nonStrikerID = team1.getNextBatsman();

        Player* s = team1.getPlayer(strikerID);
        Player* n = team1.getPlayer(nonStrikerID);
        if (s != nullptr) { s->bat.didBat = true; }
        if (n != nullptr) { n->bat.didBat = true; }

        // ---- Algorithm 10: record entry over for openers (over 1) ----
        batsmanEntryOver[strikerID]    = 1;
        batsmanEntryOver[nonStrikerID] = 1;
    }

    // ================================================================
    //  SIMULATE NEXT N OVERS
    // ================================================================
    void simulateNextOvers(int howManyOvers) {
        int oversSimulated = 0;

        while (oversSimulated < howManyOvers) {
            // Auto-transition to innings 2 if innings 1 just ended
            if (inningsNumber == 1 && isInningsOneOver()) {
                sealInningsOneAndTransition();
                // Don't count the transition as a simulated over
                continue;
            }

            Team& battingTeam  = (inningsNumber == 1) ? team1 : team2;
            Team& fieldingTeam = (inningsNumber == 1) ? team2 : team1;

            if (liveInnings->isAllOut()) {
                cout << "\n  *** ALL OUT! Innings over. ***\n";
                break;
            }
            if (liveInnings->hasCompletedOvers(totalOvers)) {
                cout << "\n  *** " << totalOvers << " overs completed. Innings over. ***\n";
                break;
            }
            if (inningsNumber == 2 && liveInnings->totalRuns >= targetRuns) {
                cout << "\n  *** Target reached! Match over. ***\n";
                break;
            }

            int    currentOver = liveInnings->currentOverNumber();
            string bowlerId    = selectBowler(fieldingTeam, currentOver);
            bool   isFast      = isFastBowler(bowlerId, fieldingTeam);
            Player* bowler     = fieldingTeam.getPlayer(bowlerId);
            Player* striker    = battingTeam.getPlayer(strikerID);

            cout << "\n  --- Over " << (currentOver + 1) << " | "
                 << (bowler ? bowler->name : bowlerId) << " to bowl ---\n";

            int legalBallsThisOver = 0;

            while (legalBallsThisOver < 6) {
                if (liveInnings->isAllOut()) { break; }
                if (inningsNumber == 2 && liveInnings->totalRuns >= targetRuns) { break; }

                int    batsmanRuns   = striker ? striker->bat.runs : 0;
                int    bowlerWickets = bowler  ? bowler->bowl.wickets : 0;
                bool   isChasing     = (inningsNumber == 2);
                double reqRate       = isChasing
                                     ? liveInnings->requiredRunRate(targetRuns, totalOvers)
                                     : 0.0;

                // ---- Algorithm 1: push to min-heap, immediately pop ----
                int ballInOver = legalBallsThisOver + 1;
                BallGenerator::DeliveryResult delivery =
                    generator.generateBall(currentOver, batsmanRuns,
                                           bowlerWickets, isChasing, reqRate, isFast);

                BallEvent event;
                event.over        = currentOver;
                event.ballInOver  = ballInOver;
                event.type        = delivery.type;
                event.batsmanId   = strikerID;
                event.bowlerId    = bowlerId;
                event.runsScored  = delivery.runsScored;
                event.extras      = delivery.extras;
                event.wicketType  = delivery.wicketType;
                event.commentary  = delivery.commentary;

                ballQueue.push(event);
                BallEvent processed = ballQueue.pop();
                applyDelivery(processed);

                if (delivery.type != "WIDE" && delivery.type != "NOBALL") {
                    legalBallsThisOver++;
                }

                striker = battingTeam.getPlayer(strikerID);
                bowler  = fieldingTeam.getPlayer(bowlerId);
            }

            // End of over: swap ends
            swap(strikerID, nonStrikerID);
            oversSimulated++;
        }

        // Seal the live partnership into the graph (only if it hasn't been
        // committed yet — wickets commit their own partnerships in handleWicket).
        // We track with a flag to avoid double-counting across multiple simulate calls.
        if (!strikerID.empty() && !nonStrikerID.empty() && currentPartnershipRuns > 0) {
            partnershipGraph.addPartnership(strikerID, nonStrikerID, currentPartnershipRuns);
            currentPartnershipRuns  = 0;   // reset so next call doesn't re-add
            currentPartnershipBalls = 0;
        }
    }

    // ----------------------------------------------------------------
    //  Seal innings 1 and automatically start innings 2.
    //  Called from simulateNextOvers when innings 1 ends mid-simulate.
    // ----------------------------------------------------------------
    void sealInningsOneAndTransition() {
        // Close open spells for not-out batsmen
        for (auto& entry : batsmanEntryOver) {
            Player* p = team1.getPlayer(entry.first);
            if (p != nullptr && p->bat.didBat && !p->bat.isOut) {
                BattingInterval spell;
                spell.start    = entry.second;
                spell.end      = totalOvers;
                spell.playerId = p->id;
                spell.runs     = p->bat.runs;
                spellTree.insert(spell);
            }
        }
        batsmanEntryOver.clear();

        // Commit the unfinished partnership
        if (currentPartnershipRuns > 0 && !strikerID.empty() && !nonStrikerID.empty()) {
            partnershipGraph.addPartnership(strikerID, nonStrikerID, currentPartnershipRuns);
            currentPartnershipRuns  = 0;
            currentPartnershipBalls = 0;
        }

        startInningsTwo();
    }

    // ================================================================
    //  START INNINGS 2
    // ================================================================
    void startInningsTwo() {
        inningsNumber           = 2;
        liveInnings             = &innings2;
        targetRuns              = innings1.totalRuns + 1;
        currentPartnershipRuns  = 0;
        currentPartnershipBalls = 0;

        strikerID    = team2.getNextBatsman();
        nonStrikerID = team2.getNextBatsman();

        Player* s = team2.getPlayer(strikerID);
        Player* n = team2.getPlayer(nonStrikerID);
        if (s != nullptr) { s->bat.didBat = true; }
        if (n != nullptr) { n->bat.didBat = true; }

        // ---- Algorithm 10: entry over for innings 2 openers ----
        batsmanEntryOver[strikerID]    = 1;
        batsmanEntryOver[nonStrikerID] = 1;

        printHeader("INNINGS 2: " + team2.name + " chasing " + to_string(targetRuns));
    }

    bool isInningsOneOver() const {
        return innings1.isAllOut() || innings1.hasCompletedOvers(totalOvers);
    }

    bool isMatchOver() const {
        if (inningsNumber < 2) { return false; }
        return innings2.isAllOut()
            || innings2.hasCompletedOvers(totalOvers)
            || innings2.totalRuns >= targetRuns;
    }

    int getInningsNumber() const { return inningsNumber; }

    // ================================================================
    //  LIVE DASHBOARD
    // ================================================================
    void printDashboard() const {
        const Team& battingTeam  = (inningsNumber == 1) ? team1 : team2;
        const Team& fieldingTeam = (inningsNumber == 1) ? team2 : team1;

        printHeader("LIVE SCORECARD — " + battingTeam.name + " vs " + fieldingTeam.name);

        cout << "\n  " << battingTeam.name << "  "
             << liveInnings->scoreString() << "  "
             << liveInnings->overString()
             << "  |  RR: " << fixed << setprecision(2) << liveInnings->runRate() << "\n";

        if (inningsNumber == 2) {
            int runsNeeded = targetRuns - liveInnings->totalRuns;
            int ballsLeft  = (totalOvers * 6) - liveInnings->legalBalls;
            cout << "  Target: " << targetRuns
                 << "  |  Need: " << runsNeeded << " off " << ballsLeft << " balls"
                 << "  |  RRR: " << fixed << setprecision(2)
                 << liveInnings->requiredRunRate(targetRuns, totalOvers) << "\n";
        }

        // Batting card
        cout << "\n  BATTING\n";
        cout << "  " << sep(63) << "\n";
        cout << "  " << left << setw(22) << "Batsman"
             << right << setw(6) << "R"
             << setw(5) << "B"
             << setw(5) << "4s"
             << setw(5) << "6s"
             << setw(8) << "SR"
             << "  Status\n";
        cout << "  " << sep(63) << "\n";

        for (const string& pid : battingTeam.battingOrder) {
            auto it = battingTeam.players.find(pid);
            if (it == battingTeam.players.end()) { continue; }
            const Player& p = it->second;
            if (!p.bat.didBat) { continue; }

            string status = "";
            if (pid == strikerID)      { status = "* (on strike)"; }
            else if (pid == nonStrikerID) { status = "  (non-striker)"; }
            else if (p.bat.isOut)      { status = "  " + p.bat.dismissalText; }
            else                       { status = "  not out"; }

            cout << "  " << left << setw(22) << p.name << right
                 << setw(6)  << p.bat.runs
                 << setw(5)  << p.bat.ballsFaced
                 << setw(5)  << p.bat.fours
                 << setw(5)  << p.bat.sixes
                 << setw(8)  << fixed << setprecision(1) << p.bat.strikeRate()
                 << "  " << status << "\n";
        }

        cout << "  " << sep(63) << "\n";
        cout << "  Extras: " << liveInnings->extras
             << "  (w " << liveInnings->wides
             << ", nb " << liveInnings->noBalls
             << ", b "  << liveInnings->byes
             << ", lb " << liveInnings->legByes << ")\n";
        cout << "  TOTAL: " << liveInnings->scoreString()
             << "  " << liveInnings->overString()
             << "  (RR: " << fixed << setprecision(2) << liveInnings->runRate() << ")\n";

        // Bowling card
        cout << "\n  BOWLING\n";
        cout << "  " << sep(63) << "\n";
        cout << "  " << left << setw(22) << "Bowler"
             << right << setw(7) << "O"
             << setw(5) << "M"
             << setw(5) << "R"
             << setw(5) << "W"
             << setw(8) << "Econ" << "\n";
        cout << "  " << sep(63) << "\n";

        for (auto& entry : fieldingTeam.players) {
            const Player& p = entry.second;
            if (p.bowl.completedOvers == 0 && p.bowl.ballsInCurrentOver == 0) { continue; }
            cout << "  " << left << setw(22) << p.name << right
                 << setw(7) << p.bowl.oversString()
                 << setw(5) << p.bowl.maidens
                 << setw(5) << p.bowl.runsConceded
                 << setw(5) << p.bowl.wickets
                 << setw(8) << fixed << setprecision(2) << p.bowl.economy() << "\n";
        }
        cout << "  " << sep(63) << "\n";

        if (!strikerID.empty() && !nonStrikerID.empty() && !liveInnings->isAllOut()) {
            cout << "\n  Current partnership: " << currentPartnershipRuns
                 << " runs off " << currentPartnershipBalls << " balls\n";
        }
    }

    // ================================================================
    //  FINAL SCORECARD
    // ================================================================
    void printFinalScorecard() const {
        auto printInnings = [&](const Team& batTeam, const Team& bwlTeam,
                                const InningsState& inn, int inNum) {
            printHeader("INNINGS " + to_string(inNum) + ": " + batTeam.name);

            cout << "\n  BATTING\n";
            cout << "  " << sep(70) << "\n";
            cout << "  " << left << setw(22) << "Batsman"
                 << setw(26) << "Dismissal"
                 << right << setw(5) << "R"
                 << setw(5) << "B"
                 << setw(5) << "4s"
                 << setw(5) << "6s"
                 << setw(7) << "SR" << "\n";
            cout << "  " << sep(70) << "\n";

            for (const string& pid : batTeam.battingOrder) {
                auto it = batTeam.players.find(pid);
                if (it == batTeam.players.end()) { continue; }
                const Player& p = it->second;
                if (!p.bat.didBat) { continue; }
                cout << "  " << left << setw(22) << p.name
                     << setw(26) << p.bat.dismissalText << right
                     << setw(5) << p.bat.runs
                     << setw(5) << p.bat.ballsFaced
                     << setw(5) << p.bat.fours
                     << setw(5) << p.bat.sixes
                     << setw(7) << fixed << setprecision(1) << p.bat.strikeRate() << "\n";
            }

            cout << "  " << sep(70) << "\n";
            cout << "  Extras: " << inn.extras
                 << "  (w " << inn.wides << ", nb " << inn.noBalls
                 << ", b " << inn.byes << ", lb " << inn.legByes << ")\n";
            cout << "  TOTAL: " << inn.scoreString() << "  " << inn.overString()
                 << "  (RR: " << fixed << setprecision(2) << inn.runRate() << ")\n";

            cout << "\n  BOWLING\n";
            cout << "  " << sep(70) << "\n";
            cout << "  " << left << setw(22) << "Bowler"
                 << right << setw(7) << "O"
                 << setw(5) << "M"
                 << setw(5) << "R"
                 << setw(5) << "W"
                 << setw(8) << "Econ" << "\n";
            cout << "  " << sep(70) << "\n";

            for (auto& entry : bwlTeam.players) {
                const Player& p = entry.second;
                if (p.bowl.completedOvers == 0 && p.bowl.ballsInCurrentOver == 0) { continue; }
                cout << "  " << left << setw(22) << p.name << right
                     << setw(7) << p.bowl.oversString()
                     << setw(5) << p.bowl.maidens
                     << setw(5) << p.bowl.runsConceded
                     << setw(5) << p.bowl.wickets
                     << setw(8) << fixed << setprecision(2) << p.bowl.economy() << "\n";
            }
            cout << "  " << sep(70) << "\n";
        };

        printInnings(team1, team2, innings1, 1);
        if (inningsNumber == 2) {
            printInnings(team2, team1, innings2, 2);
        }
    }

    // ================================================================
    //  ALGORITHM ANALYSIS FUNCTIONS
    // ================================================================

    // ---- Algorithm 2: Segment Tree — runs in over range ----
    void analyzeOversRange(int fromOver, int toOver) {
        if (fromOver < 1) { fromOver = 1; }
        if (toOver > totalOvers) { toOver = totalOvers; }

        cout << "\n  [Segment Tree] Runs & Wickets in overs " << fromOver << " to " << toOver << ":\n";

        int slot1Start = getOverSlot(fromOver - 1, 1);
        int slot1End   = getOverSlot(toOver   - 1, 1);
        int runs1      = runsPerOver.query(slot1Start, slot1End);
        int wkts1      = wicketsPerOver.query(slot1Start, slot1End);
        cout << "    Innings 1 (" << team1.name << "): "
             << runs1 << " runs, " << wkts1 << " wickets\n";

        if (inningsNumber == 2) {
            int slot2Start = getOverSlot(fromOver - 1, 2);
            int slot2End   = getOverSlot(toOver   - 1, 2);
            int runs2      = runsPerOver.query(slot2Start, slot2End);
            int wkts2      = wicketsPerOver.query(slot2Start, slot2End);
            cout << "    Innings 2 (" << team2.name << "): "
                 << runs2 << " runs, " << wkts2 << " wickets\n";
        }
    }

    // ---- Algorithm 3: Fenwick Tree — run rate after over X ----
    void analyzeRunRate(int afterOver) {
        if (afterOver < 1) { afterOver = 1; }
        if (afterOver > totalOvers) { afterOver = totalOvers; }

        cout << "\n  [Fenwick Tree] Cumulative stats after over " << afterOver << ":\n";

        // Innings 1: slots 0 to (afterOver-1), all in innings 1 range
        int slot1Start = getOverSlot(0, 1);
        int slot1End   = getOverSlot(afterOver - 1, 1);
        int runs1      = runsPrefix.rangeSum(slot1Start, slot1End);
        cout << "    Innings 1 (" << team1.name << "): "
             << runs1 << " runs scored, RR = "
             << fixed << setprecision(2) << (runs1 / (double)afterOver) << "\n";

        if (inningsNumber == 2) {
            int slot2Start = getOverSlot(0, 2);
            int slot2End   = getOverSlot(afterOver - 1, 2);
            int runs2      = runsPrefix.rangeSum(slot2Start, slot2End);
            cout << "    Innings 2 (" << team2.name << "): "
                 << runs2 << " runs scored, RR = "
                 << fixed << setprecision(2) << (runs2 / (double)afterOver) << "\n";
        }
    }

    // ---- Algorithm 4: Trie — player autocomplete ----
    void autocomplete(const string& prefix) {
        cout << "\n  [Trie Autocomplete] Players matching \"" << prefix << "\":\n";

        vector<string> matches = playerTrie.searchByPrefix(prefix);

        if (matches.empty()) {
            cout << "    No players found.\n";
            return;
        }

        for (const string& id : matches) {
            Player* p = team1.getPlayer(id);
            if (p == nullptr) { p = team2.getPlayer(id); }
            if (p != nullptr) {
                cout << "    " << p->name << "  [" << p->teamId << "]"
                     << "  |  Bat: " << p->bat.runs << "(" << p->bat.ballsFaced << ")"
                     << "  |  Bowl: " << p->bowl.oversString()
                     << "-" << p->bowl.wickets << "\n";
            }
        }
    }

    // ---- Algorithm 5: KMP — commentary search ----
    void searchCommentary(const string& keyword) {
        cout << "\n  [KMP Search] Commentary lines containing \"" << keyword << "\":\n";

        int found = 0;
        for (const string& line : commentaryLog) {
            vector<int> positions = KMP::search(line, keyword);
            if (!positions.empty()) {
                cout << "    " << line << "\n";
                found++;
            }
        }

        if (found == 0) {
            cout << "    No matching commentary found.\n";
        } else {
            cout << "    --> " << found << " result(s) found.\n";
        }
    }

    // ---- Algorithm 6: Dijkstra — strongest partnership chain ----
    void findStrongestPartnershipChain(const string& playerAId, const string& playerBId) {
        cout << "\n  [Dijkstra] Strongest partnership chain:\n";

        Player* pA = team1.getPlayer(playerAId);
        if (pA == nullptr) { pA = team2.getPlayer(playerAId); }
        Player* pB = team1.getPlayer(playerBId);
        if (pB == nullptr) { pB = team2.getPlayer(playerBId); }

        string nameA = pA ? pA->name : playerAId;
        string nameB = pB ? pB->name : playerBId;

        cout << "    From " << nameA << " to " << nameB << ":\n";

        auto result        = partnershipGraph.dijkstra(playerAId, playerBId);
        int            strength = result.first;
        vector<string> path     = result.second;

        if (path.empty()) {
            cout << "    No partnership connection found yet.\n";
            return;
        }

        cout << "    Chain: ";
        for (int i = 0; i < (int)path.size(); i++) {
            Player* p = team1.getPlayer(path[i]);
            if (p == nullptr) { p = team2.getPlayer(path[i]); }
            cout << (p ? p->name : path[i]);
            if (i + 1 < (int)path.size()) { cout << " -> "; }
        }
        cout << "\n    Strength score: " << strength << " runs\n";
    }

    // ---- Algorithm 7: BFS — all partners ----
    void findAllPartners(const string& playerId) {
        Player* p = team1.getPlayer(playerId);
        if (p == nullptr) { p = team2.getPlayer(playerId); }
        string pName = p ? p->name : playerId;

        cout << "\n  [BFS] All batsmen " << pName << " has batted alongside:\n";

        vector<string> reachable = partnershipGraph.bfsReachable(playerId);

        if (reachable.empty()) {
            cout << "    No partnerships recorded yet.\n";
            return;
        }

        for (const string& id : reachable) {
            Player* partner = team1.getPlayer(id);
            if (partner == nullptr) { partner = team2.getPlayer(id); }
            cout << "    - " << (partner ? partner->name : id) << "\n";
        }
    }

    // ---- Algorithm 7: DFS — batting clusters ----
    void findBattingClusters() {
        cout << "\n  [DFS] Batting partnership clusters:\n";

        vector<vector<string>> clusters = partnershipGraph.findBattingClusters();

        int clusterNum = 1;
        for (auto& cluster : clusters) {
            if (cluster.size() <= 1) { continue; }
            cout << "    Cluster " << clusterNum++ << ": ";
            for (int i = 0; i < (int)cluster.size(); i++) {
                Player* p = team1.getPlayer(cluster[i]);
                if (p == nullptr) { p = team2.getPlayer(cluster[i]); }
                cout << (p ? p->name : cluster[i]);
                if (i + 1 < (int)cluster.size()) { cout << ", "; }
            }
            cout << "\n";
        }
    }

    // ---- Algorithm 8: AVL Tree — batting leaderboard (public) ----
    void printLeaderboard() {
        cout << "\n  [AVL Tree] Batting Leaderboard (sorted by runs, O(log n) updates):\n";
        cout << "    Tree height: " << battingLeaderboard.getHeight() << "\n";

        vector<AVLNodeData> leaders = battingLeaderboard.getLeaderboard();

        if (leaders.empty()) {
            cout << "    No batsmen have scored yet.\n";
            return;
        }

        cout << "  " << sep(55) << "\n";
        cout << "  " << left << setw(22) << "Player"
             << setw(6)  << "Team"
             << right << setw(6) << "Runs"
             << setw(6)  << "Balls"
             << setw(8)  << "SR" << "\n";
        cout << "  " << sep(55) << "\n";

        int rank = 1;
        for (auto& d : leaders) {
            cout << "  " << setw(3) << rank++ << ". "
                 << left << setw(22) << d.name
                 << setw(6) << d.teamId
                 << right << setw(6) << d.runs
                 << setw(6) << d.balls
                 << setw(8) << fixed << setprecision(1) << d.strikeRate() << "\n";
        }
        cout << "  " << sep(55) << "\n";
    }

    // ---- Algorithm 9: Hash Table — player stat lookup ----
    void lookupPlayerStats(const string& playerId) {
        cout << "\n  [Hash Table] Player stat lookup for \"" << playerId << "\":\n";

        PlayerStatEntry* entry = playerStatCache.get(playerId);
        if (entry == nullptr) {
            cout << "    Player not found in cache. Simulate some overs first.\n";
            return;
        }

        cout << "    Name:    " << entry->name    << "\n";
        cout << "    Team:    " << entry->teamId  << "\n";
        cout << "    Runs:    " << entry->runs     << " (" << entry->balls << " balls)\n";
        cout << "    Wickets: " << entry->wickets  << "\n";
        cout << "    Economy: " << fixed << setprecision(2) << entry->economy << "\n";
    }

    void printHashTable() {
        playerStatCache.printTable();
    }

    // ---- Algorithm 10: Interval Tree — who was batting at over X ----
    void queryBattingAtOver(int overNumber) {
        cout << "\n  [Interval Tree] Batsmen active at over " << overNumber << ":\n";

        vector<BattingInterval> results = spellTree.query(overNumber, overNumber);

        if (results.empty()) {
            cout << "    No completed spells overlap that over yet.\n";
            cout << "    (Spells are recorded when a batsman is dismissed.)\n";
            return;
        }

        for (auto& spell : results) {
            Player* p = team1.getPlayer(spell.playerId);
            if (p == nullptr) { p = team2.getPlayer(spell.playerId); }
            string name = p ? p->name : spell.playerId;
            cout << "    " << name
                 << "  |  Overs " << spell.start << "-" << spell.end
                 << "  |  " << spell.runs << " runs\n";
        }
    }

    void printAllSpells() {
        cout << "\n  [Interval Tree] All batting spells recorded:\n";

        vector<BattingInterval> all = spellTree.getAllIntervals();

        if (all.empty()) {
            cout << "    No spells recorded yet.\n";
            return;
        }

        // Sort by start over for readability
        sort(all.begin(), all.end(), [](const auto& a, const auto& b) {
            return a.start < b.start;
        });

        cout << "  " << sep(55) << "\n";
        cout << "  " << left << setw(22) << "Player"
             << setw(12) << "Spell"
             << right << setw(8) << "Runs" << "\n";
        cout << "  " << sep(55) << "\n";

        for (auto& spell : all) {
            Player* p = team1.getPlayer(spell.playerId);
            if (p == nullptr) { p = team2.getPlayer(spell.playerId); }
            string name  = p ? p->name : spell.playerId;
            string range = "Ov " + to_string(spell.start) + "-" + to_string(spell.end);
            cout << "  " << left << setw(22) << name
                 << setw(12) << range
                 << right << setw(8) << spell.runs << "\n";
        }
        cout << "  " << sep(55) << "\n";
    }

    // ---- Partnership network ----
    void printPartnershipNetwork() {
        cout << "\n  [Partnership Network -- used by Algorithms 6 & 7]\n";
        partnershipGraph.printNetwork();
    }

    // ---- Match result ----
    void printResult() const {
        printHeader("MATCH RESULT");

        cout << "\n  " << team1.name << "  " << innings1.scoreString()
             << "  " << innings1.overString() << "\n";

        if (inningsNumber == 2) {
            cout << "  " << team2.name << "  " << innings2.scoreString()
                 << "  " << innings2.overString() << "\n\n";

            if (innings2.totalRuns >= targetRuns) {
                int wicketsLeft = 10 - innings2.totalWickets;
                cout << "  " << team2.name << " WON by " << wicketsLeft << " wicket(s)!\n";
            } else if (innings2.isAllOut() || innings2.hasCompletedOvers(totalOvers)) {
                int runDiff = innings1.totalRuns - innings2.totalRuns;
                cout << "  " << team1.name << " WON by " << runDiff << " run(s)!\n";
            } else {
                cout << "  Match in progress...\n";
            }
        } else {
            cout << "  Innings 1 in progress...\n";
        }
    }
};
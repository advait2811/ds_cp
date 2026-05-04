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
//    - Interactive mid-match analysis using all 7 algorithms
//    - Real-time scorecard dashboard
// ================================================================

class MatchEngine {
private:
    // ---- Teams ----
    Team team1;   // always bats first
    Team team2;   // always bats second

    // ---- Current match state ----
    int  totalOvers;
    int  inningsNumber;   // 1 or 2
    int  targetRuns;      // set after innings 1; team2 needs this many + 1

    InningsState innings1;
    InningsState innings2;
    InningsState* liveInnings;   // points to whichever innings is active

    // ---- Live crease state ----
    string strikerID;      // batsman facing
    string nonStrikerID;   // batsman at the other end
    string currentBowlerID;

    int currentPartnershipRuns  = 0;
    int currentPartnershipBalls = 0;

    // ---- Algorithms ----
    BallQueue        ballQueue;       // Algorithm 1: Min-Heap

    // Segment Trees (Algorithm 2): one for runs, one for wickets.
    // We use 40 slots (20 overs × 2 innings) so we can query either innings.
    SegmentTree      runsPerOver;
    SegmentTree      wicketsPerOver;

    // Fenwick Trees (Algorithm 3): prefix sums for run-rate calculations.
    FenwickTree      runsPrefix;
    FenwickTree      ballsPrefix;

    Trie             playerTrie;       // Algorithm 4: Autocomplete
    vector<string>   commentaryLog;    // Algorithm 5: KMP searches this
    PartnershipGraph partnershipGraph; // Algorithm 6 & 7: Dijkstra + BFS + DFS

    // ---- Ball generator ----
    BallGenerator generator;

    // ---- Scoreboard colour helpers (for the dashboard) ----
    string sep(int width = 65) const {
        return string(width, '-');
    }

    string doubleSep(int width = 65) const {
        return string(width, '=');
    }

    // ---- Display helpers ----
    void printHeader(const string& title) const {
        cout << "\n" << doubleSep() << "\n";
        cout << "  " << title << "\n";
        cout << doubleSep() << "\n";
    }

    // ---- Get the over slot index for the segment/fenwick trees ----
    // Innings 1 uses slots 0-19, Innings 2 uses slots 20-39
    int getOverSlot(int overNumber, int innings) const {
        return (innings - 1) * 20 + overNumber;
    }

    // ---- Apply a generated delivery to the match state ----
    void applyDelivery(const BallEvent& event) {
        // Identify the active teams
        Team& battingTeam = (inningsNumber == 1) ? team1 : team2;
        Team& fieldingTeam = (inningsNumber == 1) ? team2 : team1;

        Player* striker    = battingTeam.getPlayer(event.batsmanId);
        Player* bowler     = fieldingTeam.getPlayer(event.bowlerId);

        bool   isLegalBall = (event.type != "WIDE" && event.type != "NOBALL");
        int    over        = event.over;
        int    overSlot    = getOverSlot(over, inningsNumber);
        int    totalRuns   = event.runsScored + event.extras;

        // ---- Update innings totals ----
        liveInnings->totalRuns += totalRuns;
        liveInnings->extras    += event.extras;

        if (event.type == "WIDE") {
            liveInnings->wides += event.extras;
        } else if (event.type == "NOBALL") {
            liveInnings->noBalls += event.extras;
        } else if (event.type == "BYE") {
            liveInnings->byes += event.extras;
        } else if (event.type == "LEGBYE") {
            liveInnings->legByes += event.extras;
        }

        // ---- Update legal ball count (ONLY on legal deliveries) ----
        if (isLegalBall) {
            liveInnings->legalBalls++;
            currentPartnershipBalls++;
        }

        // ---- Update batsman stats ----
        if (striker != nullptr) {
            striker->bat.didBat = true;

            if (isLegalBall) {
                striker->bat.ballsFaced++;
            }

            if (event.type != "WIDE" && event.type != "BYE" && event.type != "LEGBYE") {
                striker->bat.runs += event.runsScored;
                currentPartnershipRuns += event.runsScored;

                if (event.type == "FOUR") {
                    striker->bat.fours++;
                } else if (event.type == "SIX") {
                    striker->bat.sixes++;
                }
            }
        }

        // ---- Update bowler stats ----
        if (bowler != nullptr) {
            if (event.type == "WIDE") {
                bowler->bowl.recordWide(event.extras);
            } else if (event.type == "NOBALL") {
                bowler->bowl.recordNoBall(event.extras);
                // No-ball runs scored by batsman also count against bowler
                bowler->bowl.runsConceded += event.runsScored;
            } else {
                // Legal delivery: runs + any byes/leg-byes still charged to bowler
                bowler->bowl.recordLegalBall(totalRuns);
            }

            if (event.type == "WICKET") {
                // Bowled, Caught, LBW, Stumped, Hit Wicket → credit the bowler
                if (event.wicketType != "RUN_OUT") {
                    bowler->bowl.wickets++;
                }
            }
        }

        // ---- Algorithm 2: Segment Tree — update runs and wickets per over ----
        runsPerOver.update(overSlot, totalRuns);
        if (event.type == "WICKET") {
            wicketsPerOver.update(overSlot, 1);
        }

        // ---- Algorithm 3: Fenwick Tree — update prefix sums ----
        runsPrefix.update(overSlot, totalRuns);
        if (isLegalBall) {
            ballsPrefix.update(overSlot, 1);
        }

        // ---- Handle wickets ----
        if (event.type == "WICKET") {
            handleWicket(event, battingTeam, fieldingTeam);
        }

        // ---- Handle run-based strike rotation ----
        if (event.type != "WIDE" && event.type != "NOBALL") {
            // Odd runs → striker and non-striker swap ends
            if (event.runsScored % 2 == 1) {
                swap(strikerID, nonStrikerID);
            }
        }

        // ---- Store in commentary log (for Algorithm 5: KMP search) ----
        string overLabel = to_string(over + 1) + "." + to_string(event.ballInOver);
        string logLine   = "[" + overLabel + "] " + event.commentary
                         + "  |  " + liveInnings->scoreString()
                         + " " + liveInnings->overString();
        commentaryLog.push_back(logLine);

        // ---- Print the ball ----
        cout << "  " << logLine << "\n";
    }

    // ---- Handle a wicket: update dismissal text, bring in new batsman ----
    void handleWicket(const BallEvent& event, Team& battingTeam, Team& fieldingTeam) {
        Player* outBatsman = battingTeam.getPlayer(event.batsmanId);
        Player* bowler     = fieldingTeam.getPlayer(event.bowlerId);

        // Save the partnership before resetting
        if (!strikerID.empty() && !nonStrikerID.empty() && currentPartnershipRuns > 0) {
            partnershipGraph.addPartnership(strikerID, nonStrikerID, currentPartnershipRuns);
        }
        currentPartnershipRuns  = 0;
        currentPartnershipBalls = 0;

        // Mark the batsman as out
        if (outBatsman != nullptr) {
            outBatsman->bat.isOut = true;
        }

        // Build the dismissal string (like a real scorecard)
        string dismissal = "";
        if (event.wicketType == "BOWLED") {
            dismissal = "b " + (bowler ? bowler->name : event.bowlerId);

        } else if (event.wicketType == "CAUGHT") {
            // Use a random fielder name from the fielding team (since we
            // don't track specific fielders in random generation)
            string fielderName = pickRandomFielder(fieldingTeam, event.bowlerId);
            dismissal = "c " + fielderName + " b " + (bowler ? bowler->name : event.bowlerId);

        } else if (event.wicketType == "LBW") {
            dismissal = "lbw b " + (bowler ? bowler->name : event.bowlerId);

        } else if (event.wicketType == "STUMPED") {
            string keeperName = getWicketkeeper(fieldingTeam);
            dismissal = "st " + keeperName + " b " + (bowler ? bowler->name : event.bowlerId);

            // Credit the stumping to the keeper
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

        // Update innings wicket count
        liveInnings->totalWickets++;

        // Bring in the next batsman
        string nextBatsman = battingTeam.getNextBatsman();
        if (!nextBatsman.empty() && liveInnings->totalWickets < 10) {
            // The dismissed batsman leaves; next one comes in on the same end
            if (strikerID == event.batsmanId) {
                strikerID = nextBatsman;
            } else {
                nonStrikerID = nextBatsman;
            }

            Player* incoming = battingTeam.getPlayer(nextBatsman);
            if (incoming != nullptr) {
                incoming->bat.didBat = true;
            }

            cout << "  *** " << (incoming ? incoming->name : nextBatsman)
                 << " comes in at number " << (liveInnings->totalWickets + 1) << " ***\n";
        }
    }

    // ---- Pick a random fielder from the fielding team (not the bowler) ----
    string pickRandomFielder(Team& team, const string& bowlerId) {
        vector<string> candidates;
        for (auto& entry : team.players) {
            if (entry.first != bowlerId) {
                candidates.push_back(entry.second.name);
            }
        }
        if (candidates.empty()) {
            return "fielder";
        }
        int index = rand() % candidates.size();
        return candidates[index];
    }

    // ---- Find the wicket-keeper ----
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

    // ---- Select which bowler bowls the current over ----
    // Simple rotation: bowlers alternate, no bowler bowls more than 4 overs
    string selectBowler(Team& fieldingTeam, int overNumber) {
        vector<string> bowlerIds;
        for (auto& entry : fieldingTeam.players) {
            Role r = entry.second.role;
            if (r == Role::BOWLER || r == Role::ALLROUNDER) {
                bowlerIds.push_back(entry.first);
            }
        }

        // Sort for consistent rotation
        sort(bowlerIds.begin(), bowlerIds.end());

        // Pick bowler using modulo rotation (max 4 overs per bowler)
        int index = overNumber % bowlerIds.size();
        return bowlerIds[index];
    }

    bool isFastBowler(const string& bowlerId, Team& team) {
        Player* p = team.getPlayer(bowlerId);
        if (p == nullptr) {
            return true;
        }
        // In this simulation: BOWLER types alternate fast/spin based on index
        // A real system would store bowler type explicitly
        for (int i = 0; i < (int)team.battingOrder.size(); i++) {
            if (team.battingOrder[i] == bowlerId) {
                return (i % 2 == 0);   // even index = fast, odd = spin
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
        liveInnings = &innings1;

        // ---- Build Trie from all players (Algorithm 4) ----
        for (auto& entry : team1.players) {
            playerTrie.insert(entry.second.name, entry.second.id);
        }
        for (auto& entry : team2.players) {
            playerTrie.insert(entry.second.name, entry.second.id);
        }

        // Initialise first two batsmen at the crease
        strikerID    = team1.getNextBatsman();
        nonStrikerID = team1.getNextBatsman();

        // Mark them as having batted
        Player* s = team1.getPlayer(strikerID);
        Player* n = team1.getPlayer(nonStrikerID);
        if (s != nullptr) { s->bat.didBat = true; }
        if (n != nullptr) { n->bat.didBat = true; }
    }

    // ================================================================
    //  SIMULATE NEXT N OVERS
    //
    //  Generates random deliveries for the next N overs and applies
    //  them to the match state. Stops early if:
    //    - All 10 wickets fall (all out)
    //    - The chasing team reaches their target
    //    - The allotted overs are exhausted
    // ================================================================
    void simulateNextOvers(int howManyOvers) {
        Team& battingTeam  = (inningsNumber == 1) ? team1 : team2;
        Team& fieldingTeam = (inningsNumber == 1) ? team2 : team1;

        int oversSimulated = 0;

        while (oversSimulated < howManyOvers) {
            // Check if innings should end
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

            int currentOver = liveInnings->currentOverNumber();
            string bowlerId = selectBowler(fieldingTeam, currentOver);
            bool   isFast   = isFastBowler(bowlerId, fieldingTeam);
            Player* bowler  = fieldingTeam.getPlayer(bowlerId);
            Player* striker = battingTeam.getPlayer(strikerID);

            cout << "\n  --- Over " << (currentOver + 1) << " | "
                 << (bowler ? bowler->name : bowlerId) << " to bowl ---\n";

            // Bowl 6 legal deliveries (wides and no-balls add extra balls)
            int legalBallsThisOver = 0;

            while (legalBallsThisOver < 6) {
                // Check for mid-over match-ending conditions
                if (liveInnings->isAllOut()) {
                    break;
                }
                if (inningsNumber == 2 && liveInnings->totalRuns >= targetRuns) {
                    break;
                }

                // Gather context for ball generation
                int    batsmanRuns   = striker ? striker->bat.runs : 0;
                int    bowlerWickets = bowler  ? bowler->bowl.wickets : 0;
                bool   isChasing     = (inningsNumber == 2);
                double reqRate       = isChasing
                                     ? liveInnings->requiredRunRate(targetRuns, totalOvers)
                                     : 0.0;

                // ---- Algorithm 1: Generate ball and push to min-heap ----
                int ballInOver = legalBallsThisOver + 1;
                BallGenerator::DeliveryResult delivery =
                    generator.generateBall(currentOver, batsmanRuns,
                                           bowlerWickets, isChasing, reqRate, isFast);

                BallEvent event;
                event.over         = currentOver;
                event.ballInOver   = ballInOver;
                event.type         = delivery.type;
                event.batsmanId    = strikerID;
                event.bowlerId     = bowlerId;
                event.runsScored   = delivery.runsScored;
                event.extras       = delivery.extras;
                event.wicketType   = delivery.wicketType;
                event.commentary   = delivery.commentary;

                // Push to the heap first (simulates live event arrival)
                ballQueue.push(event);

                // Then immediately pop (heap re-orders if needed)
                BallEvent processed = ballQueue.pop();
                applyDelivery(processed);

                // Only count legal balls
                if (delivery.type != "WIDE" && delivery.type != "NOBALL") {
                    legalBallsThisOver++;
                }

                // Re-fetch striker pointer in case of wicket
                striker = battingTeam.getPlayer(strikerID);
                bowler  = fieldingTeam.getPlayer(bowlerId);
            }

            // End of over: batsmen swap ends
            swap(strikerID, nonStrikerID);
            oversSimulated++;
        }

        // Save final partnership of the session
        if (!strikerID.empty() && !nonStrikerID.empty() && currentPartnershipRuns > 0) {
            partnershipGraph.addPartnership(strikerID, nonStrikerID, currentPartnershipRuns);
        }
    }

    // ================================================================
    //  START INNINGS 2
    // ================================================================
    void startInningsTwo() {
        // Record any unfinished partnership from innings 1
        if (currentPartnershipRuns > 0 && !strikerID.empty() && !nonStrikerID.empty()) {
            partnershipGraph.addPartnership(strikerID, nonStrikerID, currentPartnershipRuns);
        }

        inningsNumber        = 2;
        liveInnings          = &innings2;
        targetRuns           = innings1.totalRuns + 1;
        currentPartnershipRuns  = 0;
        currentPartnershipBalls = 0;

        // Reset crease
        strikerID    = team2.getNextBatsman();
        nonStrikerID = team2.getNextBatsman();

        Player* s = team2.getPlayer(strikerID);
        Player* n = team2.getPlayer(nonStrikerID);
        if (s != nullptr) { s->bat.didBat = true; }
        if (n != nullptr) { n->bat.didBat = true; }

        printHeader("INNINGS 2: " + team2.name + " chasing " + to_string(targetRuns));
    }

    bool isInningsOneOver() const {
        return innings1.isAllOut() || innings1.hasCompletedOvers(totalOvers);
    }

    bool isMatchOver() const {
        if (inningsNumber < 2) {
            return false;
        }
        return innings2.isAllOut()
            || innings2.hasCompletedOvers(totalOvers)
            || innings2.totalRuns >= targetRuns;
    }

    int getInningsNumber() const { return inningsNumber; }

    // ================================================================
    //  LIVE DASHBOARD — like a real cricket broadcast scoreboard
    // ================================================================
    void printDashboard() const {
        const Team& battingTeam  = (inningsNumber == 1) ? team1 : team2;
        const Team& fieldingTeam = (inningsNumber == 1) ? team2 : team1;

        printHeader("LIVE SCORECARD — " + battingTeam.name + " vs " + fieldingTeam.name);

        // ---- Score summary ----
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

        // ---- Batting card ----
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
            if (it == battingTeam.players.end()) {
                continue;
            }
            const Player& p = it->second;

            if (!p.bat.didBat) {
                continue;   // hasn't batted yet
            }

            string status = "";
            if (pid == strikerID) {
                status = "* (on strike)";
            } else if (pid == nonStrikerID) {
                status = "  (non-striker)";
            } else if (p.bat.isOut) {
                status = "  " + p.bat.dismissalText;
            } else {
                status = "  not out";
            }

            cout << "  " << left << setw(22) << p.name << right
                 << setw(6)  << p.bat.runs
                 << setw(5)  << p.bat.ballsFaced
                 << setw(5)  << p.bat.fours
                 << setw(5)  << p.bat.sixes
                 << setw(8)  << fixed << setprecision(1) << p.bat.strikeRate()
                 << "  " << status << "\n";
        }

        // Extras and total
        cout << "  " << sep(63) << "\n";
        cout << "  Extras: " << liveInnings->extras
             << "  (w " << liveInnings->wides
             << ", nb " << liveInnings->noBalls
             << ", b "  << liveInnings->byes
             << ", lb " << liveInnings->legByes << ")\n";
        cout << "  TOTAL: " << liveInnings->scoreString()
             << "  " << liveInnings->overString()
             << "  (RR: " << fixed << setprecision(2) << liveInnings->runRate() << ")\n";

        // ---- Bowling card ----
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
            if (p.bowl.completedOvers == 0 && p.bowl.ballsInCurrentOver == 0) {
                continue;
            }
            cout << "  " << left << setw(22) << p.name << right
                 << setw(7) << p.bowl.oversString()
                 << setw(5) << p.bowl.maidens
                 << setw(5) << p.bowl.runsConceded
                 << setw(5) << p.bowl.wickets
                 << setw(8) << fixed << setprecision(2) << p.bowl.economy() << "\n";
        }
        cout << "  " << sep(63) << "\n";

        // ---- Current partnership ----
        if (!strikerID.empty() && !nonStrikerID.empty() && !liveInnings->isAllOut()) {
            cout << "\n  Current partnership: " << currentPartnershipRuns
                 << " runs off " << currentPartnershipBalls << " balls\n";
        }
    }

    // Print the full final scorecard for both innings
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
                if (it == batTeam.players.end()) continue;
                const Player& p = it->second;
                if (!p.bat.didBat) continue;

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
                if (p.bowl.completedOvers == 0 && p.bowl.ballsInCurrentOver == 0) continue;
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
    //  ALGORITHM ANALYSIS FUNCTIONS (can be called at any time)
    // ================================================================

    // ---- Algorithm 2: Segment Tree ----
    void analyzeOversRange(int fromOver, int toOver) {
        if (fromOver < 1) { fromOver = 1; }
        if (toOver > totalOvers) { toOver = totalOvers; }

        cout << "\n  [Segment Tree] Runs & Wickets in overs " << fromOver << " to " << toOver << ":\n";

        if (inningsNumber >= 1) {
            int slot1Start = getOverSlot(fromOver - 1, 1);
            int slot1End   = getOverSlot(toOver   - 1, 1);
            int runs1      = runsPerOver.query(slot1Start, slot1End);
            int wkts1      = wicketsPerOver.query(slot1Start, slot1End);
            cout << "    Innings 1 (" << team1.name << "): "
                 << runs1 << " runs, " << wkts1 << " wickets\n";
        }

        if (inningsNumber == 2) {
            int slot2Start = getOverSlot(fromOver - 1, 2);
            int slot2End   = getOverSlot(toOver   - 1, 2);
            int runs2      = runsPerOver.query(slot2Start, slot2End);
            int wkts2      = wicketsPerOver.query(slot2Start, slot2End);
            cout << "    Innings 2 (" << team2.name << "): "
                 << runs2 << " runs, " << wkts2 << " wickets\n";
        }
    }

    // ---- Algorithm 3: Fenwick Tree — Run rate at any point ----
    void analyzeRunRate(int afterOver) {
        if (afterOver < 1) { afterOver = 1; }
        if (afterOver > totalOvers) { afterOver = totalOvers; }

        cout << "\n  [Fenwick Tree] Cumulative stats after over " << afterOver << ":\n";

        int slot1End   = getOverSlot(afterOver - 1, 1);
        int runs1      = runsPrefix.rangeSum(0, slot1End);
        cout << "    Innings 1 (" << team1.name << "): "
             << runs1 << " runs scored, RR = "
             << fixed << setprecision(2) << (runs1 / (double)afterOver) << "\n";

        if (inningsNumber == 2) {
            int slot2End   = getOverSlot(afterOver - 1, 2);
            int slot1Total = runsPrefix.rangeSum(0, getOverSlot(totalOvers - 1, 1));
            int runs2Raw   = runsPrefix.rangeSum(0, slot2End);
            int runsInn2   = runs2Raw - slot1Total;
            if (runsInn2 < 0) { runsInn2 = 0; }
            cout << "    Innings 2 (" << team2.name << "): "
                 << runsInn2 << " runs scored, RR = "
                 << fixed << setprecision(2) << (runsInn2 / (double)afterOver) << "\n";
        }
    }

    // ---- Algorithm 4: Trie ----
    void autocomplete(const string& prefix) {
        cout << "\n  [Trie Autocomplete] Players matching \"" << prefix << "\":\n";

        vector<string> matches = playerTrie.searchByPrefix(prefix);

        if (matches.empty()) {
            cout << "    No players found.\n";
            return;
        }

        for (const string& id : matches) {
            Player* p = team1.getPlayer(id);
            if (p == nullptr) {
                p = team2.getPlayer(id);
            }
            if (p != nullptr) {
                cout << "    " << p->name << "  [" << p->teamId << "]"
                     << "  |  Bat: " << p->bat.runs << "(" << p->bat.ballsFaced << ")"
                     << "  |  Bowl: " << p->bowl.oversString()
                     << "-" << p->bowl.wickets << "\n";
            }
        }
    }

    // ---- Algorithm 5: KMP commentary search ----
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

    // ---- Algorithm 6: Dijkstra ----
    void findStrongestPartnershipChain(const string& playerAId, const string& playerBId) {
        cout << "\n  [Dijkstra] Strongest partnership chain:\n";

        Player* pA = team1.getPlayer(playerAId);
        if (pA == nullptr) { pA = team2.getPlayer(playerAId); }
        Player* pB = team1.getPlayer(playerBId);
        if (pB == nullptr) { pB = team2.getPlayer(playerBId); }

        string nameA = pA ? pA->name : playerAId;
        string nameB = pB ? pB->name : playerBId;

        cout << "    From " << nameA << " to " << nameB << ":\n";

        auto result = partnershipGraph.dijkstra(playerAId, playerBId);
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
            if (i + 1 < (int)path.size()) {
                cout << " → ";
            }
        }
        cout << "\n    Strength score: " << strength << " runs\n";
    }

    // ---- Algorithm 7: BFS ----
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
            if (cluster.size() <= 1) {
                continue;   // skip isolated nodes
            }
            cout << "    Cluster " << clusterNum++ << ": ";
            for (int i = 0; i < (int)cluster.size(); i++) {
                Player* p = team1.getPlayer(cluster[i]);
                if (p == nullptr) { p = team2.getPlayer(cluster[i]); }
                cout << (p ? p->name : cluster[i]);
                if (i + 1 < (int)cluster.size()) {
                    cout << ", ";
                }
            }
            cout << "\n";
        }
    }

    // ---- Print the full partnership graph ----
    void printPartnershipNetwork() {
        cout << "\n  [Partnership Network — used by Algorithms 6 & 7]\n";
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

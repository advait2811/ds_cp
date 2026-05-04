#pragma once
#include <bits/stdc++.h>
using namespace std;

// ================================================================
//  PLAYER ROLE
// ================================================================
enum class Role { BATSMAN, BOWLER, ALLROUNDER, WICKETKEEPER };

string roleToString(Role r) {
    if (r == Role::BATSMAN)      return "BAT";
    if (r == Role::BOWLER)       return "BWL";
    if (r == Role::ALLROUNDER)   return "ALL";
    if (r == Role::WICKETKEEPER) return " WK";
    return "???";
}


// ================================================================
//  BATTING SCORECARD for one innings
// ================================================================
struct BattingCard {
    int    runs         = 0;
    int    ballsFaced   = 0;
    int    fours        = 0;
    int    sixes        = 0;
    bool   isOut        = false;
    bool   didBat       = false;      // came to the crease
    string dismissalText = "not out"; // e.g. "c Maxwell b Starc"

    double strikeRate() const {
        if (ballsFaced == 0) {
            return 0.0;
        }
        return (100.0 * runs) / ballsFaced;
    }
};


// ================================================================
//  BOWLING SCORECARD for one innings
// ================================================================
struct BowlingCard {
    int    completedOvers = 0;
    int    ballsInCurrentOver = 0;   // extra balls beyond full overs
    int    maidens        = 0;
    int    runsConceded   = 0;
    int    wickets        = 0;
    int    wides          = 0;
    int    noBalls        = 0;
    int    runsThisOver   = 0;   // to track maidens

    double economy() const {
        double totalOvers = completedOvers + ballsInCurrentOver / 6.0;
        if (totalOvers <= 0.0) {
            return 0.0;
        }
        return runsConceded / totalOvers;
    }

    string oversString() const {
        return to_string(completedOvers) + "." + to_string(ballsInCurrentOver);
    }

    // Call this when the over ends
    void finishOver() {
        if (runsThisOver == 0) {
            maidens++;
        }
        runsThisOver = 0;
        completedOvers++;
        ballsInCurrentOver = 0;
    }

    // Call this on each legal delivery
    void recordLegalBall(int runs) {
        ballsInCurrentOver++;
        runsConceded   += runs;
        runsThisOver   += runs;

        if (ballsInCurrentOver == 6) {
            finishOver();
        }
    }

    void recordWide(int penalty) {
        wides         += 1;
        runsConceded  += penalty;
        runsThisOver  += penalty;
        // Wides do NOT count as a legal delivery in cricket
    }

    void recordNoBall(int penalty) {
        noBalls       += 1;
        runsConceded  += penalty;
        runsThisOver  += penalty;
        // No-balls do NOT count as a legal delivery in cricket
    }
};


// ================================================================
//  FIELDING STATS
// ================================================================
struct FieldingCard {
    int catches   = 0;
    int runOuts   = 0;
    int stumpings = 0;
};


// ================================================================
//  PLAYER
// ================================================================
struct Player {
    string  id;
    string  name;
    Role    role;
    string  teamId;
    int     jerseyNumber;

    BattingCard  bat;
    BowlingCard  bowl;
    FieldingCard field;

    Player() : role(Role::BATSMAN), jerseyNumber(0) {}

    Player(string id, string name, Role role, string teamId, int jersey)
        : id(id), name(name), role(role), teamId(teamId), jerseyNumber(jersey) {}
};


// ================================================================
//  INNINGS STATE — tracks live match state
// ================================================================
struct InningsState {
    int  totalRuns     = 0;
    int  totalWickets  = 0;
    int  legalBalls    = 0;     // legal deliveries only (no wides/no-balls)
    int  extras        = 0;
    int  wides         = 0;
    int  noBalls       = 0;
    int  byes          = 0;
    int  legByes       = 0;

    // Which over number are we in? (0-indexed)
    int currentOverNumber() const {
        return legalBalls / 6;
    }

    // Which ball in the current over? (1-indexed for display)
    int currentBallInOver() const {
        return legalBalls % 6;
    }

    // Run rate = runs per over
    double runRate() const {
        double oversPlayed = legalBalls / 6.0;
        if (oversPlayed <= 0.0) {
            return 0.0;
        }
        return totalRuns / oversPlayed;
    }

    // Required run rate for the chasing team
    double requiredRunRate(int target, int totalOvers) const {
        int  runsNeeded   = target - totalRuns;
        int  ballsLeft    = (totalOvers * 6) - legalBalls;
        if (ballsLeft <= 0 || runsNeeded <= 0) {
            return 0.0;
        }
        double oversLeft = ballsLeft / 6.0;
        return runsNeeded / oversLeft;
    }

    // e.g. "147/3"
    string scoreString() const {
        return to_string(totalRuns) + "/" + to_string(totalWickets);
    }

    // e.g. "(12.3 ov)"
    string overString() const {
        return "(" + to_string(currentOverNumber()) + "."
                   + to_string(currentBallInOver()) + " ov)";
    }

    bool isAllOut() const {
        return totalWickets >= 10;
    }

    bool hasCompletedOvers(int totalOvers) const {
        return legalBalls >= totalOvers * 6;
    }
};


// ================================================================
//  TEAM
// ================================================================
struct Team {
    string id;
    string name;
    string shortName;

    // Players in batting order
    vector<string>                   battingOrder;
    unordered_map<string, Player>    players;

    // Which players have already batted (are out or still in)
    // The "next batsman index" points into battingOrder
    int nextBatsmanIndex = 0;

    Team() = default;

    Team(string id, string name, string shortName)
        : id(id), name(name), shortName(shortName) {}

    void addPlayer(const Player& p) {
        players[p.id] = p;
        battingOrder.push_back(p.id);
    }

    // Get a mutable pointer to a player (returns nullptr if not found)
    Player* getPlayer(const string& playerId) {
        auto it = players.find(playerId);
        if (it == players.end()) {
            return nullptr;
        }
        return &(it->second);
    }

    // Get the next batsman to come in (returns empty string if all out)
    string getNextBatsman() {
        if (nextBatsmanIndex >= (int)battingOrder.size()) {
            return "";
        }
        string id = battingOrder[nextBatsmanIndex];
        nextBatsmanIndex++;
        return id;
    }

    // How many wickets have fallen (= how many players have been dismissed)?
    int wicketsFallen() const {
        int count = 0;
        for (auto& entry : players) {
            if (entry.second.bat.isOut) {
                count++;
            }
        }
        return count;
    }
};

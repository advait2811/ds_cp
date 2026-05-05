#pragma once
#include "models.h"
#include <random>

class BallGenerator {
private:
    mt19937 rng;  
    double randomFloat() {
        uniform_real_distribution<double> dist(0.0, 1.0);
        return dist(rng);
    }
    int randomInt(int low, int high) {
        uniform_int_distribution<int> dist(low, high);
        return dist(rng);
    }

public:
    BallGenerator() {
        random_device rd;
        rng = mt19937(rd());
    }
    void seed(unsigned int s) {
        rng = mt19937(s);
    }
    struct DeliveryResult {
        string type;          // "DOT", "RUN", "FOUR", "SIX", "WICKET", "WIDE", "NOBALL", "BYE", "LEGBYE"
        int    runsScored;   
        int    extras;       
        string wicketType; 
        string commentary;  
    };

    DeliveryResult generateBall(int overNumber,
                                int batsmanRuns,
                                int bowlerWickets,
                                bool isChasing,
                                double targetRunRate,
                                bool isFastBowler)
    {
        DeliveryResult result;
        result.runsScored = 0;
        result.extras     = 0;

        double roll = randomFloat();
        double wideChance   = 0.06;
        double noBallChance = 0.03;

        if (roll < wideChance) {
            result.type       = "WIDE";
            result.extras     = 1;
            result.commentary = "Wide ball — strays down the leg side.";
            return result;
        }
        roll = roll - wideChance;

        if (roll < noBallChance) {
            result.type       = "NOBALL";
            result.extras     = 1;
            int freeHitRun = randomInt(0, 4);
            result.runsScored = freeHitRun;
            result.commentary = "No ball! Free hit. Batsman hits " + to_string(freeHitRun) + ".";
            return result;
        }
        roll = roll - noBallChance;
        double dotChance     = 0.30;
        double singleChance  = 0.30;
        double twoChance     = 0.12;
        double threeChance   = 0.03;
        double fourChance    = 0.13;
        double sixChance     = 0.06;
        double wicketChance  = 0.07;
        double byeChance     = 0.01;

        if (overNumber <= 5) {
            dotChance    = 0.25;
            singleChance = 0.28;
            twoChance    = 0.10;
            fourChance   = 0.18;
            sixChance    = 0.08;
            wicketChance = 0.09;
            byeChance    = 0.02;

        } else if (overNumber >= 15) {
            dotChance    = 0.20;
            singleChance = 0.20;
            twoChance    = 0.08;
            fourChance   = 0.18;
            sixChance    = 0.18;
            wicketChance = 0.12;
            byeChance    = 0.01;

        } else {
            dotChance    = 0.35;
            singleChance = 0.30;
            twoChance    = 0.13;
            fourChance   = 0.10;
            sixChance    = 0.04;
            wicketChance = 0.06;
            byeChance    = 0.02;
        }
        if (batsmanRuns >= 50) {
            sixChance    += 0.04;
            fourChance   += 0.03;
            dotChance    -= 0.04;
            singleChance -= 0.03;
        } else if (batsmanRuns <= 5) {
            dotChance    += 0.05;
            singleChance += 0.02;
            fourChance   -= 0.03;
            sixChance    -= 0.02;
        }
        if (bowlerWickets >= 2) {
            wicketChance += 0.04;
            dotChance    += 0.03;
            sixChance    -= 0.02;
        }
        if (isFastBowler) {
            wicketChance += 0.02;
            dotChance    += 0.02;
            sixChance    -= 0.01;
        } else {
            wicketChance += 0.01;
            twoChance    += 0.01;
        }
        dotChance    = max(dotChance,    0.05);
        singleChance = max(singleChance, 0.05);
        twoChance    = max(twoChance,    0.02);
        fourChance   = max(fourChance,   0.02);
        sixChance    = max(sixChance,    0.01);
        wicketChance = max(wicketChance, 0.02);
        double total = dotChance + singleChance + twoChance + threeChance
                     + fourChance + sixChance + wicketChance + byeChance;

        dotChance    /= total;
        singleChance /= total;
        twoChance    /= total;
        threeChance  /= total;
        fourChance   /= total;
        sixChance    /= total;
        wicketChance /= total;
        byeChance    /= total;
        double cumulative = 0.0;

        cumulative += dotChance;
        if (roll < cumulative) {
            result.type        = "DOT";
            result.runsScored  = 0;
            result.commentary  = pickRandom({"Dot ball. Defended solidly.",
                                             "Good length — no shot offered.",
                                             "Beaten outside off stump!",
                                             "Pushed to mid-on, no run.",
                                             "Excellent line and length, dot."});
            return result;
        }

        cumulative += singleChance;
        if (roll < cumulative) {
            result.type       = "RUN";
            result.runsScored = 1;
            result.commentary = pickRandom({"Nudged to mid-wicket, one run.",
                                            "Worked to fine leg, quick single.",
                                            "Pushed through covers, one.",
                                            "Flicked to square leg, one run.",
                                            "Tapped to point, a single."});
            return result;
        }

        cumulative += twoChance;
        if (roll < cumulative) {
            result.type       = "RUN";
            result.runsScored = 2;
            result.commentary = pickRandom({"Driven to deep extra cover, two runs.",
                                            "Punched through the gap, two.",
                                            "Cut to deep point, they run two.",
                                            "Flicked to deep mid-wicket, two runs."});
            return result;
        }

        cumulative += threeChance;
        if (roll < cumulative) {
            result.type       = "RUN";
            result.runsScored = 3;
            result.commentary = "Good running — three runs!";
            return result;
        }

        cumulative += fourChance;
        if (roll < cumulative) {
            result.type       = "FOUR";
            result.runsScored = 4;
            result.commentary = pickRandom({"FOUR! Driven beautifully through covers!",
                                            "FOUR! Cut hard past point!",
                                            "FOUR! Flicked off the pads to fine leg!",
                                            "FOUR! Pulled powerfully over mid-on!",
                                            "FOUR! Whipped through mid-wicket!"});
            return result;
        }

        cumulative += sixChance;
        if (roll < cumulative) {
            result.type       = "SIX";
            result.runsScored = 6;
            result.commentary = pickRandom({"SIX! Launched over long-on — into the stands!",
                                            "SIX! Cleared the rope at square leg!",
                                            "SIX! Lofted down the ground — maximum!",
                                            "SIX! Slog sweep over deep mid-wicket!",
                                            "SIX! Inside-out over extra cover — huge hit!"});
            return result;
        }

        cumulative += wicketChance;
        if (roll < cumulative) {
            result.type       = "WICKET";
            result.runsScored = 0;
            result.wicketType = pickWicketType(isFastBowler);
            result.commentary = buildWicketCommentary(result.wicketType);
            return result;
        }
        result.type    = "BYE";
        result.extras  = randomInt(1, 2);
        result.commentary = "Bye! Missed by the keeper — " + to_string(result.extras) + " run(s).";
        return result;
    }

private:
    string pickRandom(const vector<string>& options) {
        int index = randomInt(0, (int)options.size() - 1);
        return options[index];
    }
    string pickWicketType(bool isFastBowler) {
        double roll = randomFloat();

        if (isFastBowler) {
            if (roll < 0.25) return "BOWLED";
            if (roll < 0.55) return "CAUGHT";
            if (roll < 0.70) return "LBW";
            if (roll < 0.85) return "CAUGHT"; 
            return "RUN_OUT";
        } else {
            if (roll < 0.20) return "BOWLED";
            if (roll < 0.50) return "CAUGHT";
            if (roll < 0.65) return "LBW";
            if (roll < 0.80) return "STUMPED";
            return "CAUGHT";
        }
    }

    string buildWicketCommentary(const string& wicketType) {
        if (wicketType == "BOWLED") {
            return pickRandom({"WICKET! Bowled — the stumps are shattered!",
                                "OUT! Bowled through the gate!",
                                "GONE! The off stump is knocked back!"});
        }
        if (wicketType == "CAUGHT") {
            return pickRandom({"WICKET! Caught at mid-off — mistimed drive!",
                                "OUT! Caught behind! Feathered edge to the keeper.",
                                "CAUGHT! Top edge, taken by fine leg.",
                                "OUT! Caught at deep square — holed out!"});
        }
        if (wicketType == "LBW") {
            return pickRandom({"OUT! LBW — hit on the back pad, plumb in front!",
                                "GIVEN! LBW — missing leg stump.",
                                "OUT! LBW — skidded on, struck on the knee roll."});
        }
        if (wicketType == "STUMPED") {
            return pickRandom({"STUMPED! Danced down the track and missed — keeper does the rest.",
                                "OUT! Stumped — went for the big hit, lost his ground."});
        }
        if (wicketType == "RUN_OUT") {
            return pickRandom({"RUN OUT! Direct hit at the bowler's end — terrible mix-up!",
                                "RUN OUT! Caught short of the crease — brilliant fielding!"});
        }
        if (wicketType == "HIT_WICKET") {
            return "OUT! Hit Wicket — dislodged the bails with his bat!";
        }
        return "OUT!";
    }
};
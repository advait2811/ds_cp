#include "match_engine.h"
Team buildIndia() {
    Team india("IND", "India", "IND");
    using R = Role;

    india.addPlayer(Player("ind_rohit",    "Rohit Sharma",     R::BATSMAN,      "IND",  1));
    india.addPlayer(Player("ind_gill",     "Shubman Gill",     R::BATSMAN,      "IND", 77));
    india.addPlayer(Player("ind_kohli",    "Virat Kohli",      R::BATSMAN,      "IND", 18));
    india.addPlayer(Player("ind_surya",    "Suryakumar Yadav", R::BATSMAN,      "IND", 63));
    india.addPlayer(Player("ind_hardik",   "Hardik Pandya",    R::ALLROUNDER,   "IND", 33));
    india.addPlayer(Player("ind_jadeja",   "Ravindra Jadeja",  R::ALLROUNDER,   "IND",  8));
    india.addPlayer(Player("ind_karthik",  "Dinesh Karthik",   R::WICKETKEEPER, "IND", 24));
    india.addPlayer(Player("ind_ashwin",   "R Ashwin",         R::BOWLER,       "IND", 99));
    india.addPlayer(Player("ind_bumrah",   "Jasprit Bumrah",   R::BOWLER,       "IND", 93));
    india.addPlayer(Player("ind_shami",    "Mohammed Shami",   R::BOWLER,       "IND", 11));
    india.addPlayer(Player("ind_arshdeep","Arshdeep Singh",    R::BOWLER,       "IND", 72));

    return india;
}

Team buildAustralia() {
    Team aus("AUS", "Australia", "AUS");
    using R = Role;

    aus.addPlayer(Player("aus_warner",  "David Warner",    R::BATSMAN,      "AUS", 31));
    aus.addPlayer(Player("aus_finch",   "Aaron Finch",     R::BATSMAN,      "AUS", 19));
    aus.addPlayer(Player("aus_smith",   "Steve Smith",     R::BATSMAN,      "AUS",  7));
    aus.addPlayer(Player("aus_maxwell", "Glenn Maxwell",   R::ALLROUNDER,   "AUS", 32));
    aus.addPlayer(Player("aus_stoinis", "Marcus Stoinis",  R::ALLROUNDER,   "AUS", 24));
    aus.addPlayer(Player("aus_inglis",  "Josh Inglis",     R::WICKETKEEPER, "AUS", 41));
    aus.addPlayer(Player("aus_timdavid","Tim David",       R::BATSMAN,      "AUS", 14));
    aus.addPlayer(Player("aus_starc",   "Mitchell Starc",  R::BOWLER,       "AUS", 56));
    aus.addPlayer(Player("aus_hazel",   "Josh Hazlewood",  R::BOWLER,       "AUS", 38));
    aus.addPlayer(Player("aus_cummins", "Pat Cummins",     R::BOWLER,       "AUS", 30));
    aus.addPlayer(Player("aus_zampa",   "Adam Zampa",      R::BOWLER,       "AUS",  9));

    return aus;
}
void showMenu() {
    cout << "\n";
    cout << "  |----------------------------------------------|\n";
    cout << "  |        ANALYSIS MENU                         |\n";
    cout << "  |----------------------------------------------|\n";
    cout << "  |  1.  Show live scorecard                     |\n";
    cout << "  |  2.  Simulate next over(s)                   |\n";
    cout << "  |  3.  [Seg Tree]   Runs in over range         |\n";
    cout << "  |  4.  [Fenwick]    Run rate at over X         |\n";
    cout << "  |  5.  [Trie]       Search player by prefix    |\n";
    cout << "  |  6.  [KMP]        Search commentary          |\n";
    cout << "  |  7.  [Dijkstra]   Partnership chain          |\n";
    cout << "  |  8.  [BFS]        Find all partners          |\n";
    cout << "  |  9.  [DFS]        Batting clusters           |\n";
    cout << "  |  10. Show partnership network                |\n";
    cout << "  |  11. [AVL Tree]   Batting leaderboard        |\n";
    cout << "  |  12. [Hash Table] Player stat lookup         |\n";
    cout << "  |  13. [Interval Tree] Who batted at over X?   |\n";
    cout << "  |  0.  Exit                                    |\n";
    cout << "  |----------------------------------------------|\n";
    cout << "  Choice: ";
}

void runMenu(MatchEngine& engine) {
    int choice = -1;

    while (true) {
        showMenu();
        cin >> choice;

        if (choice == 0) {
            cout << "\n  Exiting analysis. Goodbye!\n\n";
            break;
        }

        if (choice == 1) {
            engine.printDashboard();

        } else if (choice == 2) {
            if (engine.isMatchOver()) {
                cout << "\n  Match is already over!\n";
                engine.printResult();
                continue;
            }

            cout << "  How many overs to simulate? ";
            int n;
            cin >> n;
            if (n < 1)  { n = 1;  }
            if (n > 40) { n = 40; }   // allow up to 40 so a full match can run in one go

            engine.simulateNextOvers(n);

            if (engine.isMatchOver()) {
                engine.printFinalScorecard();
                engine.printResult();
            }

        } else if (choice == 3) {
            cout << "  Enter over range [from to]: ";
            int from, to;
            cin >> from >> to;
            engine.analyzeOversRange(from, to);

        } else if (choice == 4) {
            cout << "  Run rate after over number: ";
            int ov;
            cin >> ov;
            engine.analyzeRunRate(ov);

        } else if (choice == 5) {
            cout << "  Enter name prefix (e.g. 'ro', 'vi', 'ja'): ";
            string prefix;
            cin >> prefix;
            engine.autocomplete(prefix);

        } else if (choice == 6) {
            cout << "  Enter search keyword: ";
            string kw;
            cin >> kw;
            engine.searchCommentary(kw);

        } else if (choice == 7) {
            cout << "  Enter player A ID (e.g. ind_rohit): ";
            string a;
            cin >> a;
            cout << "  Enter player B ID (e.g. ind_kohli): ";
            string b;
            cin >> b;
            engine.findStrongestPartnershipChain(a, b);

        } else if (choice == 8) {
            cout << "  Enter player ID (e.g. ind_rohit): ";
            string pid;
            cin >> pid;
            engine.findAllPartners(pid);

        } else if (choice == 9) {
            engine.findBattingClusters();

        } else if (choice == 10) {
            engine.printPartnershipNetwork();

        } else if (choice == 11) {
            engine.printLeaderboard();

        } else if (choice == 12) {
            cout << "  Enter player ID (e.g. ind_rohit): ";
            string pid;
            cin >> pid;
            engine.lookupPlayerStats(pid);

        } else if (choice == 13) {
            cout << "  Enter over number to query: ";
            int ov;
            cin >> ov;
            engine.queryBattingAtOver(ov);

        } else {
            cout << "  Invalid choice. Please try again.\n";
        }
    }
}


// ================================================================
//  MAIN
// ================================================================

int main() {
    cout << "\n";
    cout << "  |-------------------------------------------------------|\n";
    cout << "  |     LIVE CRICKET STATISTICS UPDATER                   |\n";
    cout << "  |     Advanced Data Structures — Course Project        |\n";
    cout << "  |     India vs Australia — T20 International           |\n";
    cout << "  |                                                       |\n";
    cout << "  |  Algorithms used:                                     |\n";
    cout << "  |    1.  Min-Heap      - Ball event ordering            |\n";
    cout << "  |    2.  Segment Tree  - Over range run queries         |\n";
    cout << "  |    3.  Fenwick Tree  - Prefix run rate calculations   |\n";
    cout << "  |    4.  Trie          - Player name autocomplete       |\n";
    cout << "  |    5.  KMP           - Commentary keyword search      |\n";
    cout << "  |    6.  Dijkstra      - Strongest partnership chain    |\n";
    cout << "  |    7.  BFS + DFS     - Partnership network analysis   |\n";
    cout << "  |    8.  AVL Tree      - Self-balancing leaderboard     |\n";
    cout << "  |    9.  Hash Table    - O(1) player stat cache         |\n";
    cout << "  |    10. Interval Tree - Batting spell queries          |\n";
    cout << "  |-------------------------------------------------------|\n\n";

    Team india     = buildIndia();
    Team australia = buildAustralia();

    MatchEngine engine(india, australia, 20);

    cout << "  Match starts: India vs Australia (T20, 20 overs)\n";
    cout << "  India bat first.\n";
    cout << "  Use the menu to simulate overs and analyse mid-match.\n";

    runMenu(engine);

    return 0;
}
#include <iostream>
#include <string>
#include <vector>
#include <cstdlib>
#include <ctime>
#include <conio.h> // For _getch()
#include <limits>  // For numeric_limits

#ifdef _WIN32
    #define CLEAR_CMD "cls"
#else
    #define CLEAR_CMD "clear"
#endif

using namespace std;

// ==========================================
// CONFIG & UTILS
// ==========================================

void printLine() { cout << "------------------------------------------------------------\n"; }
void clearScreen() { system(CLEAR_CMD); }
void pressAnyKey() { cout << "\n(Press ANY KEY to continue...)"; _getch(); }

enum Difficulty { EASY, NORMAL, HARD, EXTREME, NIGHTMARE };

struct GameSettings {
    Difficulty diff;
    int mapW, mapH;
    int mobDensity;
    bool flashlightBroken;
    bool canTransform;
};

// ==========================================
// GAME STATE
// ==========================================

vector<string> gameMap;
int MAP_W, MAP_H;

// ==========================================
// CLASSES
// ==========================================

enum Form { MAESTRO, SKELETON, SUPER_GHOUL };

class Player {
public:
    string name;
    int hp, maxHp, atk;
    int groove, maxGroove;
    int level, xp, xpToNext;
    Form currentForm;
    int ultCooldown;

    // Inventory & Tools
    bool keyRed, keyGreen, keyGold;
    bool flashlightOn;
    int battery;

    // Difficulty Flags
    bool isFlashlightBroken;
    bool isMagicBlocked;

    // Position
    int x, y;

    Player(string n, GameSettings settings) {
        name = n;
        maxHp = 100; hp = 100;
        maxGroove = 60; groove = 60;
        atk = 15;
        level = 1; xp = 0; xpToNext = 100;
        currentForm = MAESTRO;
        ultCooldown = 0;

        keyRed = false; keyGreen = false; keyGold = false;
        battery = 100;

        // Apply Difficulty Settings
        if (settings.flashlightBroken) {
            flashlightOn = false;
            isFlashlightBroken = true;
        } else {
            flashlightOn = true;
            isFlashlightBroken = false;
        }

        isMagicBlocked = !settings.canTransform;

        x = 1; y = 1; // Default start
    }

    void transform(Form newForm) {
        if (isMagicBlocked) {
            cout << "\n> ...Nothing happened!\n";
            cout << "> The mansion suppresses your magic! You cannot transform!\n";
            pressAnyKey();
            return;
        }

        currentForm = newForm;
        cout << "\n> MORPH TIME! ";
        switch(currentForm) {
            case MAESTRO: cout << "Form: MAESTRO (Balanced)\n"; atk = 15 + (level * 3); break;
            case SKELETON: cout << "Form: SKELETON (High Evasion)\n"; atk = 12 + (level * 3); break;
            case SUPER_GHOUL: cout << "Form: SUPER GHOUL (Power)\n"; atk = 30 + (level * 5); break;
        }
        pressAnyKey();
    }

    void gainXp(int amount) {
        xp += amount;
        if (xp >= xpToNext) {
            level++;
            xp = 0;
            xpToNext = static_cast<int>(xpToNext * 1.5);
            hp = maxHp; groove = maxGroove;
            cout << "\n*** LEVEL UP! (Lvl " << level << ") ***\n";
            pressAnyKey();
        }
    }
};

class Enemy {
public:
    string name;
    int hp, maxHp, atk;
    bool isStunned;
    string type;

    Enemy(int level, string t, Difficulty diff) {
        type = t;
        isStunned = false;

        // Nightmare makes enemies harder
        int difficultyMult = (diff == NIGHTMARE || diff == EXTREME) ? 2 : 1;

        if (type == "Boss") {
            name = "The Mayor";
            maxHp = 300 * difficultyMult;
            atk = 20 * difficultyMult;
        } else {
            string names[] = {"Zombie Butler", "Ceiling Ghoul", "Werewolf", "Possessed Doll", "Spider"};
            name = names[rand() % 5];
            maxHp = (40 + (level * 10)) * difficultyMult;
            atk = (8 + (level * 2)) * difficultyMult;
        }
        hp = maxHp;
    }
};

// ==========================================
// MAP GENERATION
// ==========================================

void generateMap(GameSettings settings) {
    MAP_W = settings.mapW;
    MAP_H = settings.mapH;
    gameMap.clear();

    // 1. Fill with Floor
    for(int i=0; i<MAP_H; i++) {
        string row = "";
        for(int j=0; j<MAP_W; j++) {
            row += ".";
        }
        gameMap.push_back(row);
    }

    // 2. Build Border Walls & Random Obstacles
    for(int y=0; y<MAP_H; y++) {
        for(int x=0; x<MAP_W; x++) {
            // Borders
            if (y == 0 || y == MAP_H-1 || x == 0 || x == MAP_W-1) {
                gameMap[y][x] = '#';
            }
            // Random internal walls (20% chance)
            else if (rand() % 100 < 20) {
                gameMap[y][x] = '#';
            }
        }
    }

    // 3. Create Boss Room (Bottom Right)
    int bx = MAP_W - 3;
    int by = MAP_H - 3;
    gameMap[by][bx] = 'B'; // Boss
    gameMap[by-1][bx] = '#';
    gameMap[by+1][bx] = '#';
    gameMap[by][bx+1] = '#';
    gameMap[by][bx-1] = 'D'; // The Gold Door to enter

    // 4. Scatter Keys & Items
    auto placeItem = [&](char item) {
        int rX, rY;
        do {
            rX = (rand() % (MAP_W - 2)) + 1;
            rY = (rand() % (MAP_H - 2)) + 1;
        } while(gameMap[rY][rX] != '.');
        gameMap[rY][rX] = item;
    };

    placeItem('K'); // Gold Key
    placeItem('r'); // Red Key
    placeItem('g'); // Green Key

    for(int i=0; i<5; i++) placeItem('+');

    int enemyCount = (MAP_W * MAP_H) / settings.mobDensity;
    for(int i=0; i<enemyCount; i++) placeItem('E');

    // Ensure Player Start is clear
    gameMap[1][1] = '.';
}

// ==========================================
// BATTLE
// ==========================================

void battle(Player &p, Enemy e, GameSettings settings) {
    clearScreen();
    printLine();
    cout << "!!! ENCOUNTER: " << p.name << " vs " << e.name << " !!!\n";
    printLine();
    pressAnyKey();

    while(p.hp > 0 && e.hp > 0) {
        clearScreen();
        // HUD
        cout << "ENEMY: " << e.name << " [HP: " << e.hp << "]\n";
        if(e.isStunned) cout << "STATUS: STUNNED\n";
        printLine();
        cout << "PLAYER: " << p.name << " (Lvl " << p.level << ")\n";
        cout << "HP: " << p.hp << "/" << p.maxHp << " | Groove: " << p.groove << "/" << p.maxGroove << "\n";
        cout << "Form: ";
        if(p.currentForm == MAESTRO) cout << "Maestro";
        else if(p.currentForm == SKELETON) cout << "Skeleton";
        else cout << "Super Ghoul";
        printLine();

        if (p.ultCooldown > 0) p.ultCooldown--;

        cout << "[1] Attack\n[2] Transform\n[3] Magic\n[4] Heal\n>> ";
        char choice = _getch();

        int dmg = 0;
        bool dodged = false;

        // --- PLAYER TURN ---
        if(choice == '1') {
            dmg = p.atk;
            cout << "\n> You attacked! " << dmg << " dmg.\n";
            p.groove = min(p.maxGroove, p.groove + 5);
        }
        else if(choice == '2') {
            cout << "\n[1] Maestro [2] Skeleton [3] Super Ghoul: ";
            char f = _getch();
            if(f=='1') p.transform(MAESTRO);
            else if(f=='2') p.transform(SKELETON);
            else if(f=='3') p.transform(SUPER_GHOUL);
        }
        else if(choice == '3') {
            if(p.currentForm == MAESTRO && p.groove >= 15) {
                cout << "\n> MAGIC: 'IS THIS SCARY?' (Stun)\n";
                e.isStunned = true; dmg = p.atk + 10; p.groove -= 15;
            }
            else if(p.currentForm == SKELETON && p.groove >= 10) {
                cout << "\n> MAGIC: 'CEILING DANCE' (Dodge)\n";
                dodged = true; p.groove -= 10;
            }
            else if(p.currentForm == SUPER_GHOUL && p.groove >= 30) {
                if(p.ultCooldown == 0) {
                    cout << "\n> ULT: 'DEMON ROAR'\n";
                    dmg = p.atk * 3; p.ultCooldown = 3; p.groove -= 30;
                } else cout << "\n> Cooldown active!\n";
            }
            else cout << "\n> Not enough Groove!\n";
        }
        else if(choice == '4') {
            if(p.groove >= 20) {
                p.hp = min(p.maxHp, p.hp + 40); p.groove -= 20;
                cout << "\n> Moonwalk Heal.\n";
            } else cout << "\n> Need 20 Groove!\n";
        }

        if(dmg > 0) e.hp -= dmg;
        if(e.hp <= 0) {
            cout << "\n>>> VICTORY! <<<\n";
            p.gainXp(e.type == "Boss" ? 1000 : 100);
            pressAnyKey();
            return;
        }

        // --- ENEMY TURN ---
        if(e.isStunned) {
            cout << "\n> Enemy is stunned!\n";
            e.isStunned = false;
        } else if(dodged) {
            cout << "\n> You dodged the attack!\n";
        } else {
            int eDmg = e.atk + (rand() % 5);
            if(p.currentForm == SKELETON) eDmg += 5;
            if(p.currentForm == SUPER_GHOUL) eDmg -= 5;

            cout << "\n> Enemy hits for " << eDmg << " dmg.\n";
            p.hp -= eDmg;
        }
        pressAnyKey();
    }
}

// ==========================================
// MAIN LOOP
// ==========================================

int main() {
    srand(time(0));
    clearScreen();

    // 1. Setup User
    string username;
    cout << "Enter Player Name: ";
    cin >> username;

    // 2. Setup Difficulty
    cout << "\nSelect Difficulty:\n";
    cout << "1. Easy (Small map, flashlight lasts longer)\n";
    cout << "2. Normal\n";
    cout << "3. Hard\n";
    cout << "4. Extreme (Huge map, strong enemies)\n";
    cout << "5. NIGHTMARE (Broken Flashlight, No Magic, Constant Fear)\n";
    cout << "Choice: ";
    int dChoice;
    cin >> dChoice;

    // Clear input buffer so Enter key doesn't skip turn
    cin.ignore(numeric_limits<streamsize>::max(), '\n');

    GameSettings settings;
    settings.canTransform = true;
    settings.flashlightBroken = false;

    switch(dChoice) {
        case 1: settings.diff = EASY; settings.mapW=15; settings.mapH=10; settings.mobDensity=20; break;
        case 2: settings.diff = NORMAL; settings.mapW=20; settings.mapH=15; settings.mobDensity=15; break;
        case 3: settings.diff = HARD; settings.mapW=100; settings.mapH=90; settings.mobDensity=10; break;
        case 4: settings.diff = EXTREME; settings.mapW=40; settings.mapH=20; settings.mobDensity=8; break;
        case 5:
            settings.diff = NIGHTMARE;
            settings.mapW=40; settings.mapH=20;
            settings.mobDensity=5;
            settings.flashlightBroken = true;
            settings.canTransform = false;
            break;
        default: settings.diff = NORMAL; settings.mapW=20; settings.mapH=15; settings.mobDensity=15; break;
    }

    // 3. Generate World
    generateMap(settings);
    Player p(username, settings);

    if (settings.diff == NIGHTMARE) {
        cout << "\n\n!!! NIGHTMARE MODE !!!\n";
        cout << username << ": \"Oh no... I dropped my flashlight! It's broken!\"\n";
        cout << "The mansion feels darker than usual. Your magic feels weak.\n";
        cout << "Transformation is DISABLED.\n";
        pressAnyKey();
    }

    string msg = "Find the Gold Key (K) to open the Gold Door (D).";

    // 4. Game Loop
    while(p.hp > 0) {
        clearScreen();

        // DRAW HUD
        cout << "DIFFICULTY: " << (settings.diff==NIGHTMARE ? "NIGHTMARE" : "Standard") << "\n";
        printLine();
        cout << "HP: " << p.hp << " | Key: " << (p.keyGold ? "YES" : "NO") << " | Bat: " << p.battery << "%\n";
        cout << "Light: " << (p.flashlightOn ? "ON" : "OFF") << "\n";
        cout << "Message: " << msg << "\n";
        printLine();

        // DRAW MAP
        int visionRadius = p.flashlightOn ? 100 : 2;

        for(int y=0; y<MAP_H; y++) {
            for(int x=0; x<MAP_W; x++) {
                if(x==p.x && y==p.y) cout << "M";
                else {
                    if (abs(p.x - x) <= visionRadius && abs(p.y - y) <= (visionRadius/2)) {
                        cout << gameMap[y][x];
                    } else {
                        cout << " ";
                    }
                }
            }
            cout << "\n";
        }

        // INPUT HANDLING
        int input = _getch();

        int nx = p.x; int ny = p.y;

        // Handle Arrow Keys
        if (input == 0 || input == 224) {
            input = _getch(); // Read second code
            switch (input) {
                case 72: ny--; break; // Up
                case 80: ny++; break; // Down
                case 75: nx--; break; // Left
                case 77: nx++; break; // Right
            }
        }
        else {
            if(input == 'w' || input == 'W') ny--;
            if(input == 's' || input == 'S') ny++;
            if(input == 'a' || input == 'A') nx--;
            if(input == 'd' || input == 'D') nx++;

            if (input == 'f' || input == 'F') {
                if (p.isFlashlightBroken) {
                    msg = "Switch broken.";
                } else if (p.battery > 0) {
                    p.flashlightOn = !p.flashlightOn;
                    msg = p.flashlightOn ? "Light ON" : "Light OFF";
                } else {
                    msg = "Battery Dead.";
                }
                continue;
            }
        }

        // BATTERY DRAIN
        if(p.flashlightOn && !p.isFlashlightBroken) {
            p.battery--;
            if(p.battery <= 0) {
                p.battery = 0;
                p.flashlightOn = false;
                msg = "Battery Died.";
            }
        }

        // COLLISION & BOUNDARY CHECKS
        if (nx >= 0 && nx < MAP_W && ny >= 0 && ny < MAP_H) {
            char tile = gameMap[ny][nx];

            if(tile == '#') {
                msg = "Wall.";
            }
            else if(tile == 'D' && !p.keyGold) {
                msg = "Locked Gold Door. Need 'K'.";
            }
            else {
                // Move Player
                p.x = nx; p.y = ny;

                if(tile == 'K') { p.keyGold = true; gameMap[ny][nx] = '.'; msg = "Got GOLD KEY!"; }
                else if(tile == 'r' || tile == 'g') { gameMap[ny][nx] = '.'; msg = "Found a spare key."; }
                else if(tile == '+') { p.battery = 100; gameMap[ny][nx] = '.'; msg = "Battery Recharged."; }
                else if(tile == 'D' && p.keyGold) { gameMap[ny][nx] = '.'; msg = "Door Unlocked."; }

                // COMBAT
                else if(tile == 'E') {
                    Enemy e(p.level, "Mob", settings.diff);
                    battle(p, e, settings);
                    gameMap[ny][nx] = '.';
                    msg = "Enemy Defeated.";
                }
                else if(tile == 'B') {
                    Enemy boss(p.level, "Boss", settings.diff);
                    battle(p, boss, settings);
                    if(p.hp > 0) {
                        clearScreen();
                        cout << "\n*** CONGRATULATIONS " << p.name << " ***\n";
                        cout << "You survived the " << (settings.diff==NIGHTMARE ? "NIGHTMARE" : "Mansion") << "!\n";
                        pressAnyKey();
                        return 0;
                    }
                }
                // RANDOM ATTACKS
                else if (!p.flashlightOn) {
                    int chance = (settings.diff == NIGHTMARE) ? 20 : 5;
                    if (rand() % 100 < chance) {
                        msg = "Ambushed in the dark!";
                        pressAnyKey();
                        Enemy e(p.level, "Mob", settings.diff);
                        battle(p, e, settings);
                    }
                }
            }
        }
    }

    cout << "\nGAME OVER.\n";
    return 0;
}

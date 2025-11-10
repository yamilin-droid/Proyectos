#include <iostream>
#include <vector>
#include <stack>
#include <utility>
#include <random>
#include <ctime>
#include <fstream>
#include <string>
#include <algorithm>

#ifdef _WIN32
#include <windows.h>
#endif

using namespace std;

const int MAZE_ROWS = 15;
const int MAZE_COLS = 15;

const char PATH_CHAR = ' ';
const char WALL_CHAR = '|';
const char KEY_CHAR = 'L';
const char TRAP_CHAR = 'T';
const char DOOR_CHAR = 'C';
const char EXIT_CHAR = 'S';
const char PORTAL_CHAR = 'O';
const char FOG_CHAR = '.';
const int VISION_RADIUS = 3;

const string SAVE_FILE  = "savegame.txt";
const string SCORE_FILE = "scores.txt";

struct Player {
    int r, c;
    int lives;
    int keys;
    int score;
    int energy;
    string name;
};

struct Maze {
    int rows, cols;
    vector<string> grid;
    vector<vector<bool>> seen;
    Maze(int r = MAZE_ROWS, int c = MAZE_COLS) : rows(r), cols(c) {
        grid.assign(rows, string(cols, WALL_CHAR));
        seen.assign(rows, vector<bool>(cols, false));
    }
};

static bool g_ansi_enabled = false;

void enableAnsiIfWindows() {
#ifdef _WIN32
    if (g_ansi_enabled) return;
    HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
    if (hOut == INVALID_HANDLE_VALUE) return;
    DWORD dwMode = 0;
    if (!GetConsoleMode(hOut, &dwMode)) return;
    dwMode |= ENABLE_VIRTUAL_TERMINAL_PROCESSING;
    if (!SetConsoleMode(hOut, dwMode)) return;
    g_ansi_enabled = true;
#endif
}

void moveCursor(int row, int col) {
    cout << "\x1b[" << row << ";" << col << "H";
}

void clearLine() {
    cout << "\x1b[0K";
}

static inline void ensureSeenSized(Maze &m) {
    if ((int)m.seen.size() != m.rows || (m.rows>0 && (int)m.seen[0].size() != m.cols)) {
        m.seen.assign(m.rows, vector<bool>(m.cols, false));
    }
}

static inline void revealAround(Maze &m, int r, int c, int radius) {
    ensureSeenSized(m);
    for (int dr = -radius; dr <= radius; ++dr) {
        for (int dc = -radius; dc <= radius; ++dc) {
            int rr = r + dr, cc = c + dc;
            if (rr >= 0 && rr < m.rows && cc >= 0 && cc < m.cols) {
                if (abs(dr) + abs(dc) <= radius) m.seen[rr][cc] = true;
            }
        }
    }
}

Maze generateMaze(int rows = MAZE_ROWS, int cols = MAZE_COLS) {
    Maze m(rows, cols);
    if (rows % 2 == 0) rows--;
    if (cols % 2 == 0) cols--;
    m.rows = rows; m.cols = cols;
    m.grid.assign(rows, string(cols, WALL_CHAR));
    m.seen.assign(rows, vector<bool>(cols, false));

    mt19937 rng((unsigned)time(nullptr) + rows*31 + cols*17);
    auto randInt = [&](int a, int b){ return uniform_int_distribution<int>(a,b)(rng); };

    int sr = 1, sc = 1;
    m.grid[sr][sc] = PATH_CHAR;
    vector<pair<int,int>> stackCells;
    stackCells.emplace_back(sr, sc);

    while (!stackCells.empty()) {
        auto [r, c] = stackCells.back();
        vector<pair<int,int>> vecinos;
        if (r - 2 > 0 && m.grid[r-2][c] == WALL_CHAR) vecinos.emplace_back(r-2, c);
        if (r + 2 < rows-1 && m.grid[r+2][c] == WALL_CHAR) vecinos.emplace_back(r+2, c);
        if (c - 2 > 0 && m.grid[r][c-2] == WALL_CHAR) vecinos.emplace_back(r, c-2);
        if (c + 2 < cols-1 && m.grid[r][c+2] == WALL_CHAR) vecinos.emplace_back(r, c+2);

        if (!vecinos.empty()) {
            auto next = vecinos[randInt(0, (int)vecinos.size()-1)];
            int nr = next.first, nc = next.second;
            int mr = (r + nr) / 2, mc = (c + nc) / 2;
            m.grid[mr][mc] = PATH_CHAR;
            m.grid[nr][nc] = PATH_CHAR;
            stackCells.emplace_back(nr, nc);
        } else {
            stackCells.pop_back();
        }
    }

    m.grid[rows-2][cols-2] = EXIT_CHAR;
    return m;
}

void placeElements(Maze &m, int keyCount, int trapCount, int doorCount, int portalPairs) {
    mt19937 rng((unsigned)time(nullptr) + 1234);
    auto randInt = [&](int a, int b){ return uniform_int_distribution<int>(a,b)(rng); };

    vector<pair<int,int>> empties;
    for (int i = 1; i < m.rows-1; ++i) for (int j = 1; j < m.cols-1; ++j)
        if (m.grid[i][j] == PATH_CHAR) empties.emplace_back(i,j);

    auto placeRandom = [&](char symbol, int count) {
        for (int k=0; k<count && !empties.empty(); ++k) {
            int idx = randInt(0, (int)empties.size()-1);
            auto pos = empties[idx];
            m.grid[pos.first][pos.second] = symbol;
            empties.erase(empties.begin() + idx);
        }
    };

    placeRandom(KEY_CHAR, keyCount);
    placeRandom(TRAP_CHAR, trapCount);
    placeRandom(DOOR_CHAR, doorCount);
    int portalsToPlace = portalPairs * 2;
    placeRandom(PORTAL_CHAR, portalsToPlace);
}

vector<pair<int,int>> findPositions(const Maze &m, char sym) {
    vector<pair<int,int>> res;
    for (int i=0;i<m.rows;i++) for (int j=0;j<m.cols;j++)
        if (m.grid[i][j] == sym) res.emplace_back(i,j);
    return res;
}

int gridTopRow() { return 1; }
int colIndexRow(const Maze &m) { return gridTopRow() + m.rows; }
int statusRow(const Maze &m)   { return colIndexRow(m) + 1; }
int messageRow(const Maze &m)  { return statusRow(m) + 1; }

int cellCol(int j) { return 1 + j*2; }

void drawFullMazeStatic(const Maze &m, const Player &p) {
    enableAnsiIfWindows();
    cout << "\x1b[2J";
    for (int i=0;i<m.rows;i++) {
        moveCursor(gridTopRow() + i, 1);
        for (int j=0;j<m.cols;j++) {
            if (i==p.r && j==p.c) {
                cout << "3>";
            } else {
                if (!m.seen.empty() && !m.seen[i][j]) cout << FOG_CHAR << ' ';
                else cout << m.grid[i][j] << ' ';
            }
        }
    }
    moveCursor(colIndexRow(m), 1);
    for (int c=0;c<m.cols;c++) {
        if (c < 10) cout << c << ' ';
        else cout << c;
    }
    moveCursor(statusRow(m), 1);
    cout << "Vidas: " << p.lives << "  Energia: [";
    for (int i=0;i<4;i++) cout << (i < p.energy ? '*' : '-');
    cout << "]  Llaves: " << p.keys << "  Puntos: " << p.score;
    moveCursor(messageRow(m), 1);
    clearLine();
    cout.flush();
}

void drawCellAt(const Maze &m, int r, int c) {
    moveCursor(gridTopRow() + r, cellCol(c));
    if (!m.seen.empty() && !m.seen[r][c]) cout << FOG_CHAR << ' ';
    else {
        char ch = m.grid[r][c];
        cout << ch << ' ';
    }
}

void drawPlayerAt(int r, int c) {
    moveCursor(gridTopRow() + r, cellCol(c));
    cout << "3>";
}

void updateStatus(const Maze &m, const Player &p) {
    moveCursor(statusRow(m), 1);
    clearLine();
    cout << "Vidas: " << p.lives << "  Energia: [";
    for (int i=0;i<4;i++) cout << (i < p.energy ? '*' : '-');
    cout << "]  Llaves: " << p.keys << "  Puntos: " << p.score;
    cout.flush();
}

void showMessage(const Maze &m, const string &msg) {
    moveCursor(messageRow(m), 1);
    clearLine();
    cout << msg;
    cout.flush();
}

static inline void drawVisibleWindow(const Maze &m, const Player &p, int radius) {
    int r0 = max(0, p.r - radius);
    int r1 = min(m.rows - 1, p.r + radius);
    int c0 = max(0, p.c - radius);
    int c1 = min(m.cols - 1, p.c + radius);
    for (int i = r0; i <= r1; ++i) {
        for (int j = c0; j <= c1; ++j) {
            if (i == p.r && j == p.c) continue;
            drawCellAt(m, i, j);
        }
    }
}

bool tryMove(Player &player, Maze &m, const string &dir) {
    int dr=0, dc=0;
    if (dir=="w" || dir=="up") { dr=-1; dc=0; }
    else if (dir=="s" || dir=="down") { dr=1; dc=0; }
    else if (dir=="a" || dir=="left") { dr=0; dc=-1; }
    else if (dir=="d" || dir=="right") { dr=0; dc=1; }
    else return false;

    int nr = player.r + dr;
    int nc = player.c + dc;
    if (nr < 0 || nr >= m.rows || nc < 0 || nc >= m.cols) {
        showMessage(m, "No puedes salir del laberinto.");
        return false;
    }
    char cell = m.grid[nr][nc];
    if (cell == WALL_CHAR) {
        player.energy--;
        if (player.energy <= 0) {
            player.lives--;
            player.energy = 4;
            updateStatus(m, player);
            if (player.lives <= 0) {
                showMessage(m, "Has perdido todas tus vidas. Reiniciando nivel...");
                return false;
            } else {
                showMessage(m, "Se agotó la energia. Perdiste una vida.");
            }
        } else {
            updateStatus(m, player);
            showMessage(m, string("Golpeaste una pared. Energia restante: ") + to_string(player.energy) + "/4");
        }
        return false;
    }

    drawCellAt(m, player.r, player.c);

    if (cell == DOOR_CHAR) {
        if (player.keys > 0) {
            player.keys--;
            player.score += 5;
            m.grid[nr][nc] = PATH_CHAR;
            showMessage(m, "Usaste una llave para abrir la puerta.");
        } else {
            drawPlayerAt(player.r, player.c);
            showMessage(m, "Necesitas una llave para abrir la puerta.");
            return false;
        }
    }

    player.r = nr; player.c = nc;
    revealAround(m, player.r, player.c, VISION_RADIUS);
    drawVisibleWindow(m, player, VISION_RADIUS);
    drawPlayerAt(player.r, player.c);

    if (cell == KEY_CHAR) {
        player.keys++;
        player.score += 10;
        m.grid[nr][nc] = PATH_CHAR;
        updateStatus(m, player);
        showMessage(m, "Recogiste una llave!");
    } else if (cell == TRAP_CHAR) {
        player.lives--;
        m.grid[nr][nc] = PATH_CHAR;
        updateStatus(m, player);
        showMessage(m, "Caiste en una trampa! Perdiste una vida.");
        if (player.lives <= 0) {
            showMessage(m, "Has perdido todas tus vidas. Reiniciando nivel...");
            return false;
        }
    } else if (cell == EXIT_CHAR) {
        player.score += 50;
        updateStatus(m, player);
        showMessage(m, "¡Salida encontrada!");
        return true;
    } else if (cell == PORTAL_CHAR) {
        vector<pair<int,int>> portals = findPositions(m, PORTAL_CHAR);
        if (portals.size() >= 2) {
            pair<int,int> cur = {nr, nc};
            vector<pair<int,int>> others;
            for (auto &pp: portals) if (pp != cur) others.push_back(pp);
            if (!others.empty()) {
                mt19937 rng((unsigned)time(nullptr));
                int idx = uniform_int_distribution<int>(0, (int)others.size()-1)(rng);
                drawCellAt(m, player.r, player.c);
                player.r = others[idx].first;
                player.c = others[idx].second;
                revealAround(m, player.r, player.c, VISION_RADIUS);
                drawVisibleWindow(m, player, VISION_RADIUS);
                drawPlayerAt(player.r, player.c);
                showMessage(m, "¡Portal! Teletransportado a otra ubicación.");
            }
        }
    }

    updateStatus(m, player);
    return false;
}

void saveGame(int levelIndex, const Maze &m, const Player &p) {
    ofstream ofs(SAVE_FILE);
    if (!ofs) { cout << "Error al abrir archivo de guardado.\n"; return; }
    ofs << levelIndex << " " << m.rows << " " << m.cols << "\n";
    ofs << p.r << " " << p.c << " " << p.lives << " " << p.keys << " " << p.score << " " << p.energy << " " << p.name << "\n";
    for (int i=0;i<m.rows;i++) ofs << m.grid[i] << "\n";
    ofs.close();
    cout << "Partida guardada en " << SAVE_FILE << "\n";
}

int loadGame(Maze &m, Player &p) {
    ifstream ifs(SAVE_FILE);
    if (!ifs) { cout << "No existe archivo de guardado.\n"; return -1; }
    int levelIndex, rows, cols;
    ifs >> levelIndex >> rows >> cols;
    ifs >> p.r >> p.c >> p.lives >> p.keys >> p.score >> p.energy;
    string restOfLine;
    getline(ifs, restOfLine);
    getline(ifs, p.name);
    m.rows = rows; m.cols = cols;
    m.grid.assign(rows, string());
    ifs.close();
    ifstream ifs2(SAVE_FILE);
    ifs2 >> levelIndex >> rows >> cols;
    ifs2 >> p.r >> p.c >> p.lives >> p.keys >> p.score >> p.energy;
    getline(ifs2, restOfLine);
    getline(ifs2, p.name);
    for (int i=0;i<rows;i++) {
        string gline;
        getline(ifs2, gline);
        if ((int)gline.size() < cols) getline(ifs2, gline);
        if ((int)gline.size() >= cols) m.grid[i] = gline.substr(0, cols);
        else m.grid[i] = string(cols, PATH_CHAR);
    }
    ifs2.close();
    ensureSeenSized(m);
    cout << "Partida cargada desde " << SAVE_FILE << "\n";
    return levelIndex;
}

string rankFromScore(int score) {
    if (score >= 200) return "Maestro";
    if (score >= 120) return "Experto";
    if (score >= 70) return "Intermedio";
    if (score >= 30) return "Principiante";
    return "Novato";
}

void saveScore(const string &name, int score) {
    ofstream ofs(SCORE_FILE, ios::app);
    if (!ofs) return;
    ofs << name << " " << score << "\n";
    ofs.close();
}

vector<pair<string,int>> readScores() {
    vector<pair<string,int>> res;
    ifstream ifs(SCORE_FILE);
    if (!ifs) return res;
    string name; int sc;
    while (ifs >> name >> sc) res.emplace_back(name, sc);
    ifs.close();
    return res;
}

void bubbleSortScores(vector<pair<string,int>> &scores) {
    int n = scores.size();
    for (int i=0;i<n;i++) for (int j=0;j<n-i-1;j++)
        if (scores[j].second < scores[j+1].second) swap(scores[j], scores[j+1]);
}

void selectionSortScores(vector<pair<string,int>> &scores) {
    int n = scores.size();
    for (int i=0;i<n;i++) {
        int maxIdx = i;
        for (int j=i+1;j<n;j++) if (scores[j].second > scores[maxIdx].second) maxIdx = j;
        swap(scores[i], scores[maxIdx]);
    }
}

void showLeaderboard() {
    auto scores = readScores();
    if (scores.empty()) { cout << "No hay puntuaciones guardadas.\n"; return; }

    cout << "Elige método de ordenamiento: 1) Burbuja  2) Selección : ";
    int opt = 1; cin >> opt;
    if (opt == 2) selectionSortScores(scores);
    else bubbleSortScores(scores);
    cout << "Leaderboard (desc):\n";
    for (auto &p : scores) cout << p.first << " - " << p.second << "\n";
    cout << "\nRangos:\n";
    for (auto &p : scores) cout << p.first << " -> " << rankFromScore(p.second) << "\n";
}

int main() {
    enableAnsiIfWindows();
    cout << "==== Laberinto de Escape (Consola C++) ====\n";
    cout << "Ingresa tu nombre: ";
    Player player;
    getline(cin, player.name);
    if (player.name.empty()) player.name = "Jugador";

    int numLevels = 3;
    vector<Maze> levels;
    for (int i=0;i<numLevels;i++) {
        Maze mz = generateMaze(MAZE_ROWS, MAZE_COLS);
        placeElements(mz, 3 + i, 6 + i*2, 3 + i, 2 + i);
        levels.push_back(mz);
    }

    int currentLevel = 0;
    bool exitGame = false;

    player.lives = 3;
    player.keys = 0;
    player.score = 0;
    player.energy = 4;

    bool foundStart = false;
    for (int r=0; r<levels[0].rows && !foundStart; ++r)
        for (int c=0; c<levels[0].cols; ++c)
            if (levels[0].grid[r][c] == PATH_CHAR) { player.r = r; player.c = c; foundStart = true; break; }

    while (!exitGame && currentLevel < (int)levels.size()) {
        Maze &maze = levels[currentLevel];
        ensureSeenSized(maze);
        revealAround(maze, player.r, player.c, VISION_RADIUS);
        drawFullMazeStatic(maze, player);
        showMessage(maze, string("Nivel ") + to_string(currentLevel+1) + " - usa WASD o up/down/left/right. save/load/quit.");

        bool levelComplete = false;
        while (!levelComplete) {
            moveCursor(messageRow(maze)+1,1);
            clearLine();
            cout << "> ";
            cout.flush();
            string cmd;
            getline(cin, cmd);
            if (cmd.empty()) continue;
            for (auto &ch: cmd) ch = tolower(ch);

            if (cmd == "quit") {
                cout << "¿Deseas guardar partida antes de salir? (s/n): ";
                string yn; getline(cin, yn);
                if (!yn.empty() && (yn[0]=='s' || yn[0]=='S' || yn[0]=='y')) saveGame(currentLevel, maze, player);
                exitGame = true;
                break;
            } else if (cmd == "save") {
                saveGame(currentLevel, maze, player);
                continue;
            } else if (cmd == "load") {
                int lvl = loadGame(maze, player);
                if (lvl >= 0) currentLevel = lvl;
                ensureSeenSized(maze);
                revealAround(maze, player.r, player.c, VISION_RADIUS);
                drawFullMazeStatic(maze, player);
                continue;
            } else if (cmd == "scores") {
                showLeaderboard();
                drawFullMazeStatic(maze, player);
                continue;
            } else if (cmd == "up" || cmd == "down" || cmd == "left" || cmd == "right" ||
                       cmd == "w" || cmd == "a" || cmd == "s" || cmd == "d") {
                bool finished = tryMove(player, maze, cmd);
                if (finished) {
                    showMessage(maze, string("¡Completaste el nivel ") + to_string(currentLevel+1) + "!");
                    currentLevel++;
                    levelComplete = true;
                    if (currentLevel < (int)levels.size()) {
                        levels[currentLevel] = generateMaze(MAZE_ROWS, MAZE_COLS);
                        placeElements(levels[currentLevel], 3 + currentLevel, 6 + currentLevel*2, 3 + currentLevel, 2 + currentLevel);
                        Maze &next = levels[currentLevel];
                        bool setPos = false;
                        for (int r=0;r<next.rows && !setPos;r++) for (int c=0;c<next.cols;c++)
                            if (next.grid[r][c] == PATH_CHAR) { player.r = r; player.c = c; setPos = true; break; }
                        player.lives = min(player.lives + 1, 5);
                        player.energy = 4;
                        player.score += 20;
                    } else {
                        showMessage(maze, "¡Has escapado de todos los niveles!");
                        saveScore(player.name, player.score);
                        exitGame = true;
                        break;
                    }
                }
                if (player.lives <= 0) {
                    showMessage(maze, "Reiniciando nivel por perder vidas...");
                    levels[currentLevel] = generateMaze(MAZE_ROWS, MAZE_COLS);
                    placeElements(levels[currentLevel], 3+currentLevel, 6+currentLevel*2, 3+currentLevel, 2+currentLevel);
                    Maze &cur = levels[currentLevel];
                    for (int r=0;r<cur.rows;r++) for (int c=0;c<cur.cols;c++)
                        if (cur.grid[r][c] == PATH_CHAR) { player.r=r; player.c=c; r=cur.rows; break; }
                    player.lives = 3;
                    player.keys = 0;
                    player.energy = 4;
                    player.score = max(0, player.score-10);
                    break;
                }
            } else {
                showMessage(maze, "Comando no reconocido. Usa WASD o up/down/left/right, save, load, scores, quit.");
            }
        }
    }

    moveCursor(statusRow(levels.back())+3, 1);
    cout << "\nJuego finalizado. Puntos totales: " << player.score << "\n";
    saveScore(player.name, player.score);
    cout << "Tu rango: " << rankFromScore(player.score) << "\n";
    cout << "Gracias por jugar.\n";
    return 0;
}

#include <string>
#include <vector>
#include <ctime>
#include <cstdlib>
#include <sstream>

static std::vector<std::vector<char>> board;
static int rows = 11;
static int cols = 21;
static int player_r = 1, player_c = 1;
static int lives = 3;
static int keys = 0;
static std::string boardStr; // string que devolvemos a JS
static std::vector<std::vector<bool>> seen;

const char PATH_CHAR = ' ';
const char WALL_CHAR = '|';
const char KEY_CHAR = 'L';
const char TRAP_CHAR = 'T';
const char DOOR_CHAR = 'C';
const char EXIT_CHAR = 'S';
const char PORTAL_CHAR = 'O';
const char FOG_CHAR = '.';
const int VISION_RADIUS = 3;

// genera un laberinto muy simple con paredes exteriores y algunas paredes internas aleatorias
static void generate_simple_maze() {
    board.assign(rows, std::vector<char>(cols, '1')); // '1' = pared por defecto
    // crear camino libre básico (un laberinto muy simple)
    for (int r = 1; r < rows-1; ++r) {
        for (int c = 1; c < cols-1; ++c) {
            board[r][c] = ' '; // camino libre
        }
    }
    // coloca algunas paredes internas
    for (int r = 2; r < rows-2; r += 2) {
        for (int c = 2; c < cols-2; c += 2) {
            board[r][c] = '1';
            // añadir muros vecinos aleatorios para darle sabor
            int dr[4] = {0,1,0,-1};
            int dc[4] = {1,0,-1,0};
            int k = rand() % 4;
            int nr = r + dr[k], nc = c + dc[k];
            if (nr>0 && nr<rows-1 && nc>0 && nc<cols-1) board[nr][nc] = '1';
        }
    }
    // colocar puerta cerrada (C), salida (S), llaves (L) y trampas (T)
    board[rows-2][cols-2] = 'S'; // salida
    board[1][cols-2] = 'C';     // puerta cerrada
    board[1][cols-3] = 'L';     // llave cercana
    // algunas trampas aleatorias
    for (int i=0;i<6;i++){
        int r = 1 + rand() % (rows-2);
        int c = 1 + rand() % (cols-2);
        if (board[r][c] == ' ') board[r][c] = 'T';
    }
    // colocar posición jugador
    player_r = 1; player_c = 1;
    board[player_r][player_c] = 'P';
    seen.assign(rows, std::vector<bool>(cols, false));
    revealAround(player_r, player_c, VISION_RADIUS);
}

// reconstruye la representación en texto del tablero + info de inventario
static void build_board_string() {
    std::ostringstream oss;
    for (int r=0;r<rows;++r) {
        for (int c=0;c<cols;++c) {
            if (seen[r][c]) {
                oss << board[r][c];
            } else {
                oss << FOG_CHAR;
            }
        }
        oss << '\n';
    }
    oss << "\nVidas: " << lives << "  Llaves: " << keys << "\n";
    boardStr = oss.str();
}

static void place_player_on_board() {
    // limpia antiguas 'P'
    for (int r=0;r<rows;++r) for (int c=0;c<cols;++c) if (board[r][c] == 'P') board[r][c] = ' ';
    board[player_r][player_c] = 'P';
}

static void attempt_move(int newr, int newc) {
    if (newr < 0 || newr >= rows || newc < 0 || newc >= cols) return;
    char dest = board[newr][newc];
    if (dest == '1') {
        // pared -> no mover
        return;
    } else if (dest == 'T') {
        // trampa -> perder vida y moverse
        lives--;
        player_r = newr; player_c = newc;
        board[newr][newc] = ' '; // quitar trampa
    } else if (dest == 'L') {
        keys++;
        player_r = newr; player_c = newc;
        board[newr][newc] = ' ';
    } else if (dest == 'C') {
        // puerta cerrada: requiere llave
        if (keys > 0) {
            keys--;
            player_r = newr; player_c = newc;
            board[newr][newc] = ' '; // abrir
        } else {
            // no mover si no tiene llave
            return;
        }
    } else if (dest == 'S') {
        // ganar: para simplicidad, mover al jugador y marcar victoria con 'W'
        player_r = newr; player_c = newc;
        board[newr][newc] = 'W';
    } else {
        // camino libre ' ' u otros
        player_r = newr; player_c = newc;
    }
    place_player_on_board();
    revealAround(player_r, player_c, VISION_RADIUS);
    build_board_string();
}

static void revealAround(int r, int c, int radius) {
    for (int dr = -radius; dr <= radius; ++dr) {
        for (int dc = -radius; dc <= radius; ++dc) {
            int rr = r + dr, cc = c + dc;
            if (rr >= 0 && rr < rows && cc >= 0 && cc < cols) {
                if (abs(dr) + abs(dc) <= radius) seen[rr][cc] = true;
            }
        }
    }
}

extern "C" {

// inicializa el juego (llamado desde JS)
void init_game() {
    srand((unsigned)time(nullptr));
    lives = 3;
    keys = 0;
    generate_simple_maze();
    build_board_string();
}

// devuelve un puntero a la cadena con el tablero (UTF-8)
// en JS usar Module.UTF8ToString(ptr) para leer.
const char* get_board() {
    return boardStr.c_str();
}

void move_up()    { attempt_move(player_r-1, player_c); }
void move_down()  { attempt_move(player_r+1, player_c); }
void move_left()  { attempt_move(player_r, player_c-1); }
void move_right() { attempt_move(player_r, player_c+1); }

} // extern "C"

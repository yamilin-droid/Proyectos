/*
 Lab Escape - core C++ (Emscripten friendly)
 - Compile with emscripten (instructions in README.md)
 - Exposes functions to JS via EMSCRIPTEN_KEEPALIVE
*/

#include <emscripten.h>
#include <emscripten/val.h>
#include <string>
#include <vector>
#include <stack>
#include <random>
#include <ctime>
#include <sstream>
#include <algorithm>
#include <cstring>
#include <cassert>

using namespace std;

static std::mt19937 rng((unsigned)time(nullptr));

struct Player {
    int x=1, y=1;
    int lives=3;
    int keys=0;
    int level=0;
    int score=0;
};

struct Level {
    int w, h;
    vector<string> grid; // rows of length w
    // portals: map char 'O' positions to pair (targetLevel, tx,ty) - for simplicity we store vector of pairs
    vector<pair<int,int>> portals_src;
    vector<pair<int,int>> portals_dst;
};

vector<Level> levels;
Player player;

static string lastBoardStr;
static string lastStatusStr;
static string lastSaveStr;
static string lastHighScoresStr;

// --- Utility ---
inline bool inside(int x,int y,int w,int h){ return x>=0 && y>=0 && x<w && y<h; }

char tileAt(const Level& L,int x,int y){ return L.grid[y][x]; }
void setTile(Level& L,int x,int y,char c){ L.grid[y][x]=c; }

string boardToString(const Level& L, const Player& p){
    ostringstream ss;
    for(int y=0;y<L.h;y++){
        for(int x=0;x<L.w;x++){
            if (x==p.x && y==p.y) ss << 'P';
            else ss << L.grid[y][x];
        }
        if (y < L.h-1) ss << '\n';
    }
    return ss.str();
}

string statusToJSON(const Player& p){
    ostringstream ss;
    ss << "{\"lives\":"<<p.lives<<",\"keys\":"<<p.keys<<",\"level\":"<<p.level<<",\"score\":"<<p.score<<" }";
    return ss.str();
}

// --- Maze generator (randomized DFS), odd dimensions recommended ---
Level generateMaze(int w, int h){
    // Ensure odd dims >=3
    if (w%2==0) w++;
    if (h%2==0) h++;
    if (w<3) w=3;
    if (h<3) h=3;
    Level L; L.w=w; L.h=h;
    L.grid.assign(h, string(w,'1')); // walls
    // carve passages
    auto carve = [&](int sx,int sy){
        stack<pair<int,int>> st;
        st.push({sx,sy});
        setTile(L,sx,sy,' ');
        while(!st.empty()){
            auto [cx,cy]=st.top(); st.pop();
            vector<pair<int,int>> dirs = {{2,0},{-2,0},{0,2},{0,-2}};
            shuffle(dirs.begin(), dirs.end(), rng);
            for(auto d: dirs){
                int nx = cx + d.first;
                int ny = cy + d.second;
                if (inside(nx,ny,w,h) && tileAt(L,nx,ny)=='1'){
                    // carve
                    setTile(L, cx + d.first/2, cy + d.second/2, ' ');
                    setTile(L, nx, ny, ' ');
                    st.push({nx,ny});
                }
            }
        }
    };
    carve(1,1);
    // place exit S at far corner
    setTile(L,w-2,h-2,'S');
    // Place some random keys, traps, closed doors and portals
    int cells = (w*h)/20;
    uniform_int_distribution<int> dx(1,w-2), dy(1,h-2);
    for(int i=0;i<cells;i++){
        int x=dx(rng), y=dy(rng);
        if (tileAt(L,x,y)!=' ') continue;
        int r = rng()%5;
        if (r==0) setTile(L,x,y,'L'); // key
        else if (r==1) setTile(L,x,y,'T'); // trap
        else if (r==2) setTile(L,x,y,'C'); // closed door
        else if (r==3) setTile(L,x,y,'O'); // portal source/target
        // else leave empty
    }
    // collect portal locations; we'll pair them later in caller
    for(int y=0;y<h;y++) for(int x=0;x<w;x++) if (tileAt(L,x,y)=='O') L.portals_src.push_back({x,y});
    return L;
}

// Pair portals between levels - simple one-to-one mapping if possible
void pairPortalsAcrossLevels(){
    for(size_t i=0;i+1<levels.size();++i){
        auto &A = levels[i];
        auto &B = levels[i+1];
        // take min count
        size_t n = min(A.portals_src.size(), B.portals_src.size());
        for(size_t k=0;k<n;k++){
            A.portals_dst.push_back(B.portals_src[k]); // portal in A leads to position in B
            B.portals_dst.push_back(A.portals_src[k]); // back link (symmetric)
        }
    }
}

// --- Movement and interactions ---
void tryCollectKey(Player& p, Level& L){
    char t = tileAt(L,p.x,p.y);
    if (t=='L'){ p.keys++; setTile(L,p.x,p.y,' '); p.score+=50; }
    else if (t=='T'){ p.lives--; setTile(L,p.x,p.y,' '); p.score = max(0, p.score-20); }
    else if (t=='S'){ p.score += 200; /* level finished: move to next if exists */ if (p.level+1 < (int)levels.size()){ p.level++; p.x=1; p.y=1; } }
    else if (t=='C'){ // closed door, require key
        if (p.keys>0){ p.keys--; setTile(L,p.x,p.y,' '); p.score+=30; }
        else { /* can't move into door; caller should prevent movement */ }
    } else if (t=='O'){
        // find portal index
        for(size_t i=0;i<L.portals_src.size();++i){
            if (L.portals_src[i].first==p.x && L.portals_src[i].second==p.y){
                if (i < L.portals_dst.size()){
                    auto dst = L.portals_dst[i];
                    // find which level this portal belongs to (we built two-way links earlier)
                    // naive: teleport to next level if exists, otherwise to previous
                    int targetLevel = p.level;
                    // simple heuristic: search in all levels for dst coordinate
                    for(size_t li=0; li<levels.size(); ++li){
                        if (li== (size_t)p.level) continue;
                        if (inside(dst.first,dst.second,levels[li].w, levels[li].h) &&
                            tileAt(levels[li], dst.first, dst.second) == 'O'){
                                targetLevel = (int)li;
                                break;
                        }
                    }
                    p.level = targetLevel;
                    p.x = dst.first;
                    p.y = dst.second;
                    p.score += 100;
                }
            }
        }
    }
}

bool canMoveTo(const Level& L, int nx, int ny, const Player& p){
    if (!inside(nx,ny,L.w,L.h)) return false;
    char t = tileAt(L,nx,ny);
    if (t=='1') return false; // wall
    if (t=='C' && p.keys<=0) return false; // door and no key
    return true;
}

void moveBy(int dx, int dy){
    if (player.lives<=0) return;
    Level& L = levels[player.level];
    int nx = player.x + dx, ny = player.y + dy;
    if (!canMoveTo(L,nx,ny,player)) return;
    player.x = nx; player.y = ny;
    tryCollectKey(player, L);
    // update board strings
    lastBoardStr = boardToString(levels[player.level], player);
    lastStatusStr = statusToJSON(player);
}

// --- Save/Load (JSON-ish strings) ---
string escapeJson(const string& s){
    string out; for(char c:s){ if (c=='"') out += "\\\""; else out += c; } return out;
}

string getSaveString(){
    ostringstream ss;
    ss << "{ \"player\": {\"x\":"<<player.x<<",\"y\":"<<player.y<<",\"lives\":"<<player.lives<<",\"keys\":"<<player.keys<<",\"level\":"<<player.level<<",\"score\":"<<player.score<<" }, \"levels\":[";
    for(size_t li=0; li<levels.size(); ++li){
        if (li) ss << ",";
        ss << "{ \"w\":"<<levels[li].w<<",\"h\":"<<levels[li].h<<",\"grid\":\""<< escapeJson( [&]{ ostringstream t; for(auto &r: levels[li].grid){ t<<r; if (&r != &levels[li].grid.back()) t<<"\\n"; } return t.str(); }() ) <<"\" }";
    }
    ss << "] }";
    return ss.str();
}

void loadFromString(const char* json){
    // Very simple (fragile) parser for the specific format produced by getSaveString.
    // For production use use a JSON library; here we implement a minimal parser.
    string s = json ? json : "";
    // reset
    levels.clear();
    player = Player();
    // parse player fields
    auto findInt = [&](const string& key, int def=0)->int{
        size_t p = s.find("\""+key+"\":");
        if (p==string::npos) return def;
        p += key.size()+3;
        int val = def;
        val = stoi(s.substr(p));
        return val;
    };
    player.x = findInt("x",1);
    player.y = findInt("y",1);
    player.lives = findInt("lives",3);
    player.keys = findInt("keys",0);
    player.level = findInt("level",0);
    player.score = findInt("score",0);
    // parse levels: find occurrences of "grid":"...".
    size_t pos = 0;
    while(true){
        size_t gpos = s.find("\"grid\":\"", pos);
        if (gpos==string::npos) break;
        gpos += 8;
        size_t end = gpos;
        string gridflat;
        while (end < s.size()){
            if (s[end]=='\\' && end+1 < s.size() && s[end+1]=='n'){
                gridflat.push_back('\n');
                end+=2;
            } else if (s[end]=='\"') break;
            else { gridflat.push_back(s[end]); end++; }
        }
        Level L;
        // Also try to find width/height nearby (optional)
        size_t wpos = s.rfind("\"w\":", gpos);
        size_t hpos = s.rfind("\"h\":", gpos);
        if (wpos!=string::npos && hpos!=string::npos && wpos < gpos && hpos < gpos){
            L.w = stoi(s.substr(wpos+4));
            L.h = stoi(s.substr(hpos+4));
        } else {
            // fallback: split lines
            vector<string> rows;
            string row;
            for(char c: gridflat){
                if (c=='\n'){ rows.push_back(row); row.clear(); }
                else row.push_back(c);
            }
            if (!row.empty()) rows.push_back(row);
            L.h = (int)rows.size();
            L.w = (L.h>0)?(int)rows[0].size():0;
        }
        // reconstruct grid by splitting lines
        L.grid.clear();
        string row;
        for(char c: gridflat){
            if (c=='\n'){ L.grid.push_back(row); row.clear(); }
            else row.push_back(c);
        }
        if (!row.empty()) L.grid.push_back(row);
        if (L.grid.empty()){
            // fallback to fill empty
            L.w = max(3,L.w); L.h = max(3,L.h);
            L.grid.assign(L.h, string(L.w,' '));
        }
        // collect portal positions
        for(int y=0;y<L.h;y++) for(int x=0;x<L.w;x++) if (L.grid[y][x]=='O') L.portals_src.push_back({x,y});
        levels.push_back(L);
        pos = end+1;
    }
    // repopulate portal pairings (simple)
    pairPortalsAcrossLevels();
    lastBoardStr = boardToString(levels[player.level], player);
    lastStatusStr = statusToJSON(player);
}

// --- High Scores and quicksort implementation (descending) ---
vector<pair<string,int>> highScores;

void quicksortScores(int l, int r){
    if (l>=r) return;
    auto pivot = highScores[(l+r)/2].second;
    int i=l, j=r;
    while(i<=j){
        while(highScores[i].second > pivot) i++;
        while(highScores[j].second < pivot) j--;
        if (i<=j){ swap(highScores[i], highScores[j]); i++; j--; }
    }
    if (l<j) quicksortScores(l,j);
    if (i<r) quicksortScores(i,r);
}

void addHighScore(const string& name, int score){
    highScores.push_back({name, score});
    if (highScores.size()>1) quicksortScores(0, (int)highScores.size()-1);
    // keep top 10
    if (highScores.size()>10) highScores.resize(10);
}

string highScoresToJSON(){
    ostringstream ss;
    ss << "[";
    for(size_t i=0;i<highScores.size();++i){
        if (i) ss << ",";
        ss << "{\"name\":\""<<escapeJson(highScores[i].first)<<"\",\"score\": "<<highScores[i].second<<"}";
    }
    ss << "]";
    return ss.str();
}

// --- Initialization ---
void initGame(int numLevels=2, int w=21, int h=15){
    levels.clear();
    for(int i=0;i<numLevels;i++){
        levels.push_back(generateMaze(w,h));
    }
    pairPortalsAcrossLevels();
    player = Player();
    player.x = 1; player.y = 1; player.level = 0; player.lives = 3; player.keys = 0; player.score = 0;
    lastBoardStr = boardToString(levels[player.level], player);
    lastStatusStr = statusToJSON(player);
    highScores.clear();
}

// --- Exported C interface for JS ---
extern "C" {

EMSCRIPTEN_KEEPALIVE
const char* getBoard(){
    lastBoardStr = boardToString(levels[player.level], player);
    return lastBoardStr.c_str();
}

EMSCRIPTEN_KEEPALIVE
const char* getStatus(){
    lastStatusStr = statusToJSON(player);
    return lastStatusStr.c_str();
}

EMSCRIPTEN_KEEPALIVE
void moveUp(){ moveBy(0,-1); }

EMSCRIPTEN_KEEPALIVE
void moveDown(){ moveBy(0,1); }

EMSCRIPTEN_KEEPALIVE
void moveLeft(){ moveBy(-1,0); }

EMSCRIPTEN_KEEPALIVE
void moveRight(){ moveBy(1,0); }

EMSCRIPTEN_KEEPALIVE
const char* getSaveString(){
    lastSaveStr = ::getSaveString();
    return lastSaveStr.c_str();
}

EMSCRIPTEN_KEEPALIVE
void loadFromString(const char* json){
    ::loadFromString(json);
}

EMSCRIPTEN_KEEPALIVE
void addScore(const char* name, int score){
    addHighScore(string(name?name:"anon"), score);
    lastHighScoresStr = highScoresToJSON();
}

EMSCRIPTEN_KEEPALIVE
const char* getHighScores(){
    lastHighScoresStr = highScoresToJSON();
    return lastHighScoresStr.c_str();
}

EMSCRIPTEN_KEEPALIVE
void newGame(){
    initGame(2, 21, 15);
}

} // extern "C"

// When building to WASM you'll expose those functions to JS via ccall/cwrap.
// End of file
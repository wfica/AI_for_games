#include <bits/stdc++.h>

using namespace std;

/**
 * Make the hero reach the exit of the maze alive.
 **/

constexpr int N = 12;
constexpr int M = 16;
constexpr int MAX_HERO_HEALTH = 20;
constexpr array<int, 4> X = {-1, 0, 1, 0};
constexpr array<int, 4> Y = {0, 1, 0, -1};

bool operator==(const pair<int, int>& a, const pair<int, int>& b) {
  return a.first == b.first && a.second == b.second;
}

bool operator!=(const pair<int, int>& a, const pair<int, int>& b) {
  return !(a == b);
}

enum MonsterT { None, Box, Skeleton, Gargoyle, Orc, Vampire };

constexpr int num_M = 6;
constexpr array<int, num_M> monster_view_range = {-1, 0, 1, 2, 2, 3};
constexpr array<int, num_M> monster_attack_range = {-1, 0, 1, 1, 2, 3};
constexpr array<int, num_M> monster_damage = {-1, 0, 1, 2, 2, 3};

using MonsterCmpT = pair<MonsterT, pair<int, int>>;

struct MonsterCmp {
  bool operator()(const MonsterCmpT& a, const MonsterCmpT& b) const {
    const auto [ma, pa] = a;
    const auto [mb, pb] = b;
    if (monster_attack_range[ma] > monster_attack_range[mb] ||
        (monster_attack_range[ma] == monster_attack_range[mb] &&
         monster_damage[ma] > monster_damage[mb])) {
      return true;
    }
    if (pa.first < pb.first ||
        (pa.first == pb.first && pa.second < pb.second)) {
      return true;
    }
    return false;
  }
};

struct Monster {
  Monster() : type{None}, health{0} {}
  MonsterT type;
  int health;
};

inline bool DangerousMonster(const MonsterT m) { return m != None && m != Box; }
inline int ViewRange(const MonsterT m) { return monster_view_range[m]; }
inline int AttackRange(const MonsterT m) { return monster_attack_range[m]; }
inline int Damage(const MonsterT m) { return monster_damage[m]; }
inline MonsterT ToMonsterT(int x) { return static_cast<MonsterT>(x % num_M); }

enum ItemT { Nothing, Exit, Obstacle, Treasure, Potion, Hammer, Scythe, Bow };
constexpr int num_I = 8;
constexpr array<int, num_I> item_value = {0, 10000, -1, 100, 10, 1, 2, 3};
constexpr array<int, num_I> item_priority = {0,   100, -100, 600,
                                             200, 300, 400,  500};
inline int Value(const ItemT item) { return item_value[item]; }

inline ItemT ToItemT(int x) { return static_cast<ItemT>(x % num_I + 1); }

enum Weapon { WSword, WHammer, WScythe, WBow };

const vector<vector<vector<pair<int, int>>>> weapon_range = {
    {{{-1, 0}}, {{0, 1}}, {{1, 0}}, {{0, -1}}},  // sword
    {{{-1, -1}, {-1, 0}, {-1, 1}},
     {{-1, 0}, {-1, 1}, {0, 1}},
     {{-1, 1}, {0, 1}, {1, 1}},
     {{0, 1}, {1, 1}, {1, 0}},
     {{1, 1}, {1, 0}, {1, -1}},
     {{1, 0}, {1, -1}, {0, -1}},
     {{1, -1}, {0, -1}, {-1, -1}},
     {{0, -1}, {-1, -1}, {-1, 0}}},  // hammer
    {{{-1, 0}, {-2, 0}},
     {{-1, 1}, {-2, 2}},
     {{0, 1}, {0, 2}},
     {{1, 1}, {2, 2}},
     {{1, 0}, {2, 0}},
     {{1, -1}, {2, -2}},
     {{0, -1}, {0, -2}},
     {{-1, -1}, {-2, -2}}},  // scythe
    {{{-3, -3}}, {{-3, -2}}, {{-3, -1}}, {{-3, 0}},  {{-3, 1}},  {{-3, 2}},
     {{-3, 3}},  {{-2, -3}}, {{-2, -2}}, {{-2, -1}}, {{-2, 0}},  {{-2, 1}},
     {{-2, 2}},  {{-2, 3}},  {{-1, -3}}, {{-1, -2}}, {{-1, -1}}, {{-1, 0}},
     {{-1, 1}},  {{-1, 2}},  {{-1, 3}},  {{0, -3}},  {{0, -2}},  {{0, -1}},
     {{0, 0}},   {{0, 1}},   {{0, 2}},   {{0, 3}},   {{1, -3}},  {{1, -2}},
     {{1, -1}},  {{1, 0}},   {{1, 1}},   {{1, 2}},   {{1, 3}},   {{2, -3}},
     {{2, -2}},  {{2, -1}},  {{2, 0}},   {{2, 1}},   {{2, 2}},   {{2, 3}},
     {{3, -3}},  {{3, -2}},  {{3, -1}},  {{3, 0}},   {{3, 1}},   {{3, 2}},
     {{3, 3}}}  // bow
};
constexpr array<int, 4> weapon_damage = {10, 6, 7, 8};

struct Tile {
  Tile() : item{Nothing}, discovered{false} {}
  Monster monster;
  ItemT item;
  bool discovered;
};

struct Hero {
  Hero() {}
  Hero(int x_, int y_, int health_, int score_, int charges_hammer_,
       int charges_scythe_, int charges_bow_)
      : x{x_},
        y{y_},
        health{health_},
        score{score_},
        charges{{INT_MAX, charges_hammer_, charges_scythe_, charges_bow_}} {}
  int x, y, health;
  int score;  // current score
  array<int, 4> charges;
  //   int charges_sword;
  //   int charges_hammer;  // how many times the hammer can be used
  //   int charges_scythe;  // how many times the scythe can be used
  //   int charges_bow;     // how many times the bow can be used
};

struct Step {
  Step(int x_, int y_) : x{x_}, y{y_} {}
  int x, y;
};

struct Attack {
  Attack(int w_, int x_, int y_) : weapon{w_}, x{x_}, y{y_} {}
  int weapon, x, y;
};

struct Move {
  Move(int x, int y, string msg = "") : mv{Step(x, y)}, info{msg} {}
  Move(int w, int x, int y, string msg = "") : mv{Attack(w, x, y)}, info{msg} {}
  variant<Step, Attack> mv;
  string info;
  void Print() {
    if (holds_alternative<Step>(mv)) {
      Step s = get<Step>(mv);
      cout << "MOVE " << s.y << " " << s.x << " " << info << endl;
    } else {
      Attack a = get<Attack>(mv);
      cout << "ATTACK " << a.weapon << " " << a.y << " " << a.x << " " << info
           << endl;
    }
  }
};

struct Board {
  Board() : exit_x{-1}, exit_y{-1}, aim_x{-1}, aim_y{-1}, turn{0} {
    for (int i = 0; i < N; ++i)
      for (int j = 0; j < M; ++j) {
        vst[i][j] = false;
        in_queue[i][j] = false;
      }
  }
  Tile tiles[N][M];
  Hero hero;
  int exit_x, exit_y;
  int turn;

  // monster data.
  set<pair<MonsterT, pair<int, int>>, MonsterCmp> dangerous_monsters;

  // NextMove data.
  bool vst[N][M];
  bool in_queue[N][M];
  int aim_x, aim_y;
  // priority (larger comes out first), <x, y>
  vector<pair<int, int>> points_to_visit;

  // bfs data.
  int dist[N][M];
  pair<int, int> parent[N][M];

  void Bfs() {
    for (int i = 0; i < N; ++i)
      for (int j = 0; j < M; ++j) dist[i][j] = INT_MAX;
    dist[hero.x][hero.y] = 0;
    parent[hero.x][hero.y] = {-1, -1};
    queue<pair<int, int>> Q;
    Q.push({hero.x, hero.y});
    while (!Q.empty()) {
      const auto [x, y] = Q.front();
      // cerr << "bfs at " << x<< " " << y;
      Q.pop();
      for (int i = 0; i < 4; ++i) {
        int nx = x + X[i];
        int ny = y + Y[i];
        // cerr << nx << " " << ny << endl;
        // cerr << turn << " "<< (turn > 150) << " " << (tiles[nx][ny].item != Exit) << endl;
        // cerr << OnBoard(nx, ny) << " " <<  tiles[nx][ny].discovered << " " <<
        //     (dist[nx][ny] > dist[x][y] + 1) << " " <<  (tiles[nx][ny].item !=
        //     Obstacle) << " " << !DangerousMonster(tiles[nx][ny].monster.type)
        //     << endl;
        if (OnBoard(nx, ny) && tiles[nx][ny].discovered &&
            (dist[nx][ny] > dist[x][y] + 1) && tiles[nx][ny].item != Obstacle &&
            ((turn > 150) || (tiles[nx][ny].item != Exit)) &&
            !DangerousMonster(tiles[nx][ny].monster.type)) {
          dist[nx][ny] = dist[x][y] + 1;
          parent[nx][ny] = {x, y};
          Q.push({nx, ny});
        }
      }
    }

    if (aim_x != -1)
      cerr << "dist between hero: " << hero.x << " " << hero.y
           << " and aim: " << aim_x << " " << aim_y
           << " is: " << dist[aim_x][aim_y] << endl;

    /*
        cerr << "BFS\n";
        for (int i = 0; i < N; ++i) {
          for (int j = 0; j < M; ++j) {
            if (dist[i][j] == INT_MAX) {
              cerr << "MAX ";
            } else {
              cerr << dist[i][j] << " ";
            }
          }
          cerr << "\n";
        }
        */
  }

  pair<int, int> BacktrackFirstMoveToTheAim() {
    int curr_x = aim_x;
    int curr_y = aim_y;
    while (parent[curr_x][curr_y] != pair<int, int>({hero.x, hero.y})) {
      const auto [x, y] = parent[curr_x][curr_y];
      curr_x = x;
      curr_y = y;
    }
    return {curr_x, curr_y};
  }

  bool NeighbouringUnknown(int x, int y) {
    for (int i = 0; i < 4; ++i) {
      int nx = x + X[i];
      int ny = y + Y[i];
      if (OnBoard(nx, ny) && !tiles[nx][ny].discovered) {
        return true;
      }
    }
    return false;
  }

  bool OneStepNeighbours(int x, int y, int n_x, int n_y) {
    return (abs(x - n_x) + abs(y - n_y) == 1);
  }

  inline bool OnBoard(int x, int y) {
    return x >= 0 && y >= 0 && x < N && y < M;
  }

  bool ValidStep(int x, int y) {
    if (!OnBoard(x, y) || DangerousMonster(tiles[x][y].monster.type) ||
        tiles[x][y].item == Obstacle) {
      // cerr << "Not a valid move" << OnBoard(x, y) << " "
      //      << tiles[x][y].monster.type << " " << tiles[x][y].item << endl;
      return false;
    }
    return true;
  }

  bool InRange(int monster_x_delta, int monster_y_delta, Weapon w) {
    for (const auto& poss : weapon_range[w]) {
      for (const auto [x_delta, y_delta] : poss) {
        if (x_delta == monster_x_delta && y_delta == monster_y_delta) {
          return true;
        }
      }
    }
    return false;
  }

  optional<Move> DealWithMonsters() {
    if (dangerous_monsters.empty()) {
      return {};
    }
    while (!dangerous_monsters.empty()) {
      const auto& iter = dangerous_monsters.begin();
      const auto [monster_type, monster_pos] = *iter;
      const auto [mx_delta, my_delta] = monster_pos;
      dangerous_monsters.erase(iter);
      const array<Weapon, 4> weapon_order = {WScythe, WBow, WHammer, WSword};
      for (const Weapon w : weapon_order) {
        if (hero.charges[w] < 1) continue;
        if (InRange(mx_delta, my_delta, w)) {
          return Move(w, mx_delta + hero.x, my_delta + hero.y, "in range");
        }
      }
    }
    cerr << "Wanted to attack but out of range." << endl;
    return {};
  }

  void AddNewGoalsSimple() {
    for (int i = 0; i < 4; ++i) {
      int x = hero.x + X[i];
      int y = hero.y + Y[i];
      cerr << "considering adding i-th move " << i << " to " << x << " " << y
           << endl;
      if (!ValidStep(x, y)) continue;
      if (vst[x][y] || in_queue[x][y]) continue;
      points_to_visit.push_back({x, y});
      in_queue[x][y] = true;
    }
  }

  void AddNewGoals() {
    vector<pair<int, int>> interesting, boring;
    for (int i = -3; i <= 3; ++i) {
      for (int j = -3; j <= 3; ++j) {
        int x = hero.x + i;
        int y = hero.y + j;
        // cerr << "considering adding a move to " << x << " " << y << endl;
        if (!ValidStep(x, y)) continue;
        if (vst[x][y] || in_queue[x][y]) continue;
        if (dist[x][y] == INT_MAX && tiles[x][y].item != Exit) continue;

        points_to_visit.push_back({x, y});
        in_queue[x][y] = true;
        // if (tiles[x][y].item != Nothing) {
        //   interesting.push_back({x, y});
        // } else {
        //   boring.push_back({x, y});
        // }
      }
    }
    // // stack: push the boring one first;
    // for (const auto [x, y] : boring) {
    //   points_to_visit.push({x, y});
    //   in_queue[x][y] = true;
    // }
    // // stack: then the interesting ones.
    // sort(interesting.begin(), interesting.end(),
    //      [&](const pair<int, int>& a, const pair<int, int>& b) -> bool {
    //        ItemT item_a = tiles[a.first][a.second].item;
    //        ItemT item_b = tiles[b.first][b.second].item;
    //        if (item_a == Treasure) return false;
    //        if (item_b == Treasure) return true;
    //        return item_a < item_b;
    //      });
    // cerr << "interesting moves: ";
    // for (const auto [x, y] : interesting) {
    //   cerr << x << "," << y << "  ";
    //   points_to_visit.push({x, y});
    //   in_queue[x][y] = true;
    // }
    // cerr << endl;
  }

  void FindNewAim() {
    int curr_idx = -1;
    int curr_priority = INT_MIN;
    for (int i = 0; i < static_cast<int>(points_to_visit.size()); ++i) {
      const auto [x, y] = points_to_visit[i];
      if(turn > 150) cerr << "pos new aim: " << x << " " << y;
      int prio = item_priority[tiles[x][y].item];
      if (NeighbouringUnknown(x, y)) {
        prio += 1;
      }
      if (hero.health <= 10 && tiles[x][y].item == Potion) {
        prio *= 10;
      }
      if (tiles[x][y].item == Treasure &&
          OneStepNeighbours(x, y, hero.x, hero.y)) {
        prio *= 10;
      }
      if (tiles[x][y].item == Treasure) {
        prio -= abs(hero.x - x);
        prio -= abs(hero.y - y);
      }
      if(turn > 150) cerr << " with probability: " << prio << endl;
      if (prio > curr_priority) {
        curr_priority = prio;
        curr_idx = i;
      }
    }
    aim_x = points_to_visit[curr_idx].first;
    aim_y = points_to_visit[curr_idx].second;
    points_to_visit.erase(points_to_visit.begin() + curr_idx);
  }

  Move NextMove() {
    turn++;
    vst[hero.x][hero.y] = true;
    cerr << "hero at " << hero.x << " " << hero.y << " aiming at " << aim_x
         << " " << aim_y << " in turn " << turn << endl;

    if (hero.x == aim_x && hero.y == aim_y) {
      cerr << "at the current aim" << endl;
      aim_x = -1;
      aim_y = -1;
    }

    const optional<Move> may_mv = DealWithMonsters();
    if (may_mv.has_value()) {
      return may_mv.value();
    }

    // no monsters in range.
    Bfs();

    // AddNewGoalsSimple();
    AddNewGoals();

    if (aim_x == -1 && !points_to_visit.empty()) {
      // no aim in mind and some goals on the list.
      FindNewAim();
    }

    if (aim_x != -1) {
      // has a set aim.
      const auto [x, y] = BacktrackFirstMoveToTheAim();
      return Move(x, y,
                  " to the aim " + to_string(aim_x) + " " + to_string(aim_y));
    }

    if (exit_x == -1) {
      // known exit.
      cerr << "ERROR: Going to exit, exit unknown." << endl;
    }

    return Move(exit_x, exit_y, "to the exit!");
  }

  void ClearBoardInHeroRange() {
    dangerous_monsters.clear();
    for (int i = -3; i < 4; ++i) {
      for (int j = -3; j < 4; ++j) {
        if (i == 0 && j == 0) continue;
        int x = hero.x + i;
        int y = hero.y + j;
        if (!OnBoard(x, y)) continue;
        tiles[x][y] = Tile();
        tiles[x][y].discovered = true;
      }
    }
  }
};

int main() {
  Board board;

  while (1) {
    int x;               // x position of the hero
    int y;               // y position of the hero
    int health;          // current health points
    int score;           // current score
    int charges_hammer;  // how many times the hammer can be used
    int charges_scythe;  // how many times the scythe can be used
    int charges_bow;     // how many times the bow can be used
    cin >> x >> y >> health >> score >> charges_hammer >> charges_scythe >>
        charges_bow;
    cin.ignore();

    board.hero =
        Hero(y, x, health, score, charges_hammer, charges_scythe, charges_bow);

    // Clear board within hero range.
    board.ClearBoardInHeroRange();

    int visible_entities;  // the number of visible entities
    cin >> visible_entities;
    cin.ignore();
    for (int i = 0; i < visible_entities; i++) {
      int ex;      // x position of the entity
      int ey;      // y position of the entity
      int etype;   // the type of the entity
      int evalue;  // value associated with the entity
      cin >> ex >> ey >> etype >> evalue;
      cin.ignore();

      Tile& tile = board.tiles[ey][ex];

      tile.discovered = true;
      if (etype < 7) {
        // item.
        tile.item = ToItemT(etype);
        if (tile.item == Exit) {
          board.exit_x = ey;
          board.exit_y = ex;
        }
        if (Value(tile.item) != evalue) {
          cerr << "ERROR: Item values do not match! " << ex << " " << ey << " "
               << etype << " " << evalue << " " << Value(tile.item) << endl;
        }
      } else {
        // monster.
        tile.monster.type = ToMonsterT(etype);
        tile.monster.health = evalue;
        if (DangerousMonster(tile.monster.type)) {
          board.dangerous_monsters.insert(
              {tile.monster.type, {ey - y, ex - x}});
        }
      }
    }

    board.NextMove().Print();
  }
}
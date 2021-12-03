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

enum MonsterT { None, Box, Skeleton, Gargoyle, Orc, Vampire };

constexpr int num_M = 6;
constexpr array<int, num_M> monster_view_range = {-1, 0, 1, 2, 2, 3};
constexpr array<int, num_M> monster_attack_range = {-1, 0, 1, 1, 2, 3};
constexpr array<int, num_M> monster_damage = {-1, 0, 1, 2, 2, 3};

struct Monster {
  Monster() : type{None}, health{0} {}
  MonsterT type;
  int health;
};

inline int ViewRange(const MonsterT m) { return monster_view_range[m]; }
inline int AttackRange(const MonsterT m) { return monster_attack_range[m]; }
inline int Damage(const MonsterT m) { return monster_damage[m]; }
inline MonsterT ToMonsterT(int x) { return static_cast<MonsterT>(x % num_M); }

enum ItemT { Nothing, Exit, Obstacle, Treasure, Potion, Hammer, Scythe, Bow };
constexpr int num_I = 8;
constexpr array<int, num_I> item_value = {0, 10000, -1, 100, 10, 1, 2, 3};
inline int Value(const ItemT item) { return item_value[item]; }

inline ItemT ToItemT(int x) { return static_cast<ItemT>(x % num_I + 1); }

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
        charges_hammer{charges_hammer_},
        charges_scythe{charges_scythe_},
        charges_bow{charges_bow_} {}
  int x, y, health;
  int score;           // current score
  int charges_hammer;  // how many times the hammer can be used
  int charges_scythe;  // how many times the scythe can be used
  int charges_bow;     // how many times the bow can be used
};

struct Step {
  int x, y;
};

struct Attack {
  int weapon, x, y;
};

struct Move {
  variant<Step, Attack> mv;
  void Print() {
    if (holds_alternative<Step>(mv)) {
      Step s = get<Step>(mv);
      cout << "MOVE " << s.y << " " << s.x << endl;
    } else {
      Attack a = get<Attack>(mv);
      cout << "ATTACK " << a.weapon << " " << a.y << " " << a.x << endl;
    }
  }
};

struct Board {
  Board() : exit_x{-1}, exit_y{-1} {
    for (int i = 0; i < N; ++i)
      for (int j = 0; j < M; ++j) vst[i][j] = 0;
  }
  Tile tiles[N][M];
  Hero hero;
  int exit_x, exit_y;
  int vst[N][M];

  inline bool OnBoard(int x, int y) {
    return x >= 0 && y >= 0 && x < N && y < M;
  }

  inline bool ValidStep(int x, int y) {
    if (!OnBoard(x, y) || tiles[x][y].monster.type != None ||
        tiles[x][y].item == Obstacle) {
      return false;
    }
    return true;
  }

  void dfs(int cnt) {
    ReadInput();

    if (exit_x != -1) {
      // known exit.
      cout << "MOVE " << exit_y << " " << exit_x << endl;
    } else {
      // undiscovered exit.
      for (int i = 0; i < 4; ++i) {
        int x = hero.x + X[i];
        int y = hero.y + Y[i];
        if (!ValidStep(x, y)) continue;
        if (vst[x][y]) continue;
        cout << "MOVE " << y << " " << x << endl;
        dfs(cnt);
      }
    }
  }

  void Play() {
    for (int r = 1; r < 151; ++r) {
      for (int i = 0; i < N; ++i)
        for (int j = 0; j < M; ++j) vst[i][j] = 0;
      dfs(r);
    }
  }

  void ReadInput() {
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

    hero =
        Hero(y, x, health, score, charges_hammer, charges_scythe, charges_bow);

    // Clear board within hero range.
    ClearBoardInHeroRange();

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

      tiles[ey][ex].discovered = true;
      if (etype < 7) {
        // item.
        tiles[ey][ex].item = ToItemT(etype);
        if (tiles[ey][ex].item == Exit) {
          exit_x = ey;
          exit_y = ex;
        }
        if (Value(tiles[ey][ex].item) != evalue) {
          cerr << "ERROR: Item values do not match! " << ex << " " << ey << " "
               << etype << " " << evalue << " " << Value(tiles[ey][ex].item)
               << endl;
        }
      } else {
        // monster.
        tiles[ey][ex].monster.type = ToMonsterT(etype);
        tiles[ey][ex].monster.health = evalue;
      }
    }
  }

  void ClearBoardInHeroRange() {
    for (int i = -3; i < 4; ++i) {
      for (int j = -3; j < 4; ++j) {
        if (i == 0 && j == 0) continue;
        int x = hero.x + i;
        int y = hero.y + j;
        tiles[x][y] = Tile();
      }
    }
  }
};

int main() {
  Board board;
  board.Play();
}
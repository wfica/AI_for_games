// Implementation based on
// http://www.gameaipro.com/GameAIPro2/GameAIPro2_Chapter14_JPS_Plus_An_Extreme_A_Star_Speed_Optimization_for_Static_Uniform_Cost_Grids.pdf

#include <bits/stdc++.h>

#define loop(x, n) for (int x = 1; x <= (n); ++x)

using namespace std;

template <typename T, typename C>
ostream &operator<<(ostream &os, const pair<T, C> &x) {
  os << x.first << " " << x.second;
  return os;
}
template <typename T>
int sgn(T val) {
  return (T(0) < val) - (val < T(0));
}

constexpr int OPEN_TILE = 1;
constexpr int CLOSED_TILE = 0;
constexpr int CARDINAL_X[] = {-1, 0, 1, 0};
constexpr int CARDINAL_Y[] = {0, 1, 0, -1};
constexpr int ALL_X[] = {-1, -1, 0, 1, 1, 1, 0, -1};
constexpr int ALL_Y[] = {0, 1, 1, 1, 0, -1, -1, -1};

// enum CanonicalDirection { N, E, S, W };

/**
 * Tiles are encoded as follows:
 * - if first bit is OFF then then the tile is closed, else it is open.
 * - if the tile is open it might be a primary jump point, in such a case:
 *   - bits number 1, 2, 3, 4 represent the direction of travel (from a forced
 *neighbour to that point) for that node to be a primary jump point.
 **/
using Tile = int;

enum AllDirection { N, NE, E, SE, S, SW, W, NW };
using Distances = array<int, 8>;

inline bool IsOpenTile(const Tile tile) { return (tile & OPEN_TILE); }

struct BoardIterator {
 public:
  BoardIterator(int n, int m, int x, int y, AllDirection dir, int wrap_x,
                int wrap_y, int fin_x, int fin_y)
      : n_{n},
        m_{m},
        curr_x{x},
        curr_y{y},
        direction{dir},
        wrap_x_{wrap_x},
        wrap_y_{wrap_y},
        end_x{fin_x},
        end_y{fin_y},
        new_roll_{true} {
    Move(end_x, end_y);
  }

  pair<int, int> Next() {
    int old_x = curr_x, old_y = curr_y;
    new_roll_ = Move(curr_x, curr_y);
    return {old_x, old_y};
  }

  inline bool IsEnd() const { return end_x == curr_x && end_y == curr_y; }
  inline bool IsNewRoll() const { return new_roll_; }
  inline AllDirection GetDirection() const { return direction; }

 private:
  bool Move(int &x, int &y) {
    int new_x = x + ALL_X[direction];
    int new_y = y + ALL_Y[direction];
    if (new_x > 0 && new_x <= n_ && new_y > 0 && new_y <= m_) {
      x = new_x;
      y = new_y;
      return false;
    }
    x += wrap_x_;
    y += wrap_y_;
    return true;
  }

 public:
  int n_, m_;
  int curr_x, curr_y;
  AllDirection direction;
  int wrap_x_, wrap_y_;
  int end_x, end_y;
  bool new_roll_;
};

struct Board {
  Board() {}
  void ReadBoard() {
    cin >> m_ >> n_;
    cin.ignore();
    // Pad the board with a wall on all 4 sides.
    board_ = vector<vector<Tile>>(n_ + 2, vector<Tile>(m_ + 2, CLOSED_TILE));
    dist_ = vector<vector<Distances>>(
        n_ + 2, vector<Distances>(m_ + 2, Distances({0, 0, 0, 0, 0, 0, 0, 0})));
    for (int i = 1; i <= n_; i++) {
      string row;  // A single row of the map consisting of passable terrain
                   // ('.') and walls ('#')
      cin >> row;
      cin.ignore();
      for (int j = 1; j <= m_; j++) {
        board_[i][j] = (row[j - 1] == '.' ? OPEN_TILE : CLOSED_TILE);
      }
    }
  }

  void CheckForcedNeighbour(int i, int j, int prev_x, int prev_y, int direction,
                            int perpendicular_direction) {
    int neigh_x = i + CARDINAL_X[perpendicular_direction];
    int neigh_y = j + CARDINAL_Y[perpendicular_direction];

    int prev_neigh_x = prev_x + CARDINAL_X[perpendicular_direction];
    int prev_neigh_y = prev_y + CARDINAL_Y[perpendicular_direction];

    if (IsOpen(neigh_x, neigh_y) && !IsOpen(prev_neigh_x, prev_neigh_y)) {
      board_[i][j] |= (1 << (1 + direction));
    }
  }

  void FindPrimaryPoints() {
    for (int i = 1; i <= n_; ++i) {
      for (int j = 1; j <= m_; ++j) {
        if (IsOpen(i, j)) {
          for (int k = 0; k < 4; ++k) {
            int prev_x = i + CARDINAL_X[k];
            int prev_y = j + CARDINAL_Y[k];
            if (IsOpen(prev_x, prev_y)) {
              CheckForcedNeighbour(i, j, prev_x, prev_y, k, (k + 1) % 4);
              CheckForcedNeighbour(i, j, prev_x, prev_y, k, (k + 3) % 4);
            }
          }
        }
      }
    }
  }

  BoardIterator GetIterator(const AllDirection dir) const {
    switch (dir) {
      case N:
      case NE:
        return BoardIterator(n_, m_, n_, 1, N, n_ - 1, 1, 1, m_);
      case E:
        return BoardIterator(n_, m_, 1, 1, E, 1, -1 * m_ + 1, n_, m_);
      case S:
      case SE:
        return BoardIterator(n_, m_, 1, 1, S, -1 * n_ + 1, 1, n_, m_);
      case W:
      case SW:
        return BoardIterator(n_, m_, 1, m_, W, 1, m_ - 1, n_, 1);
      case NW:
        return BoardIterator(n_, m_, n_, m_, W, -1, m_ - 1, 1, 1);
      default:
        throw logic_error("Only 8 directions allowed.");
    }
  }

  void SweepCardinalDirections() {
    const array<AllDirection, 4> canonical_dirs = {N, E, S, W};
    for (const AllDirection &dir : canonical_dirs) {
      const AllDirection opposite_dir =
          static_cast<AllDirection>((dir + 4) % 8);

      BoardIterator itr = GetIterator(dir);

      // cerr << "New iterator going " << dir << endl;
      int dist = -1;
      bool jump_point_last_seen = false;
      while (!itr.IsEnd()) {
        if (itr.IsNewRoll()) {
          dist = -1;
          jump_point_last_seen = false;
        }
        const auto [x, y] = itr.Next();
        // cerr << "iterator position: " << x << " " << y << "\n";
        if (!IsOpen(x, y)) {
          dist = -1;
          jump_point_last_seen = false;
          dist_[x][y][opposite_dir] = 0;

        } else {  // IsOpen(Tile)
          dist++;
          if (jump_point_last_seen) {
            dist_[x][y][opposite_dir] = dist;
          } else {  // Wall last seen
            dist_[x][y][opposite_dir] = -1 * dist;
          }
          if (JumpPoint(x, y, dir)) {
            dist = 0;
            jump_point_last_seen = true;
          }
        }
      }
    }
  }

  void SweepNonCardinalDirections() {
    const array<AllDirection, 4> non_canonical_dirs = {NE, SE, SW, NW};
    for (const AllDirection &dir : non_canonical_dirs) {
      const AllDirection opposite_dir =
          static_cast<AllDirection>((dir + 4) % 8);

      BoardIterator itr = GetIterator(dir);

      // cerr << "New iterator going " << dir << endl;
      while (!itr.IsEnd()) {
        const auto [x, y] = itr.Next();
        // cerr << "iterator position: " << x << " " << y << "\n";
        if (IsOpen(x, y)) {
          int diag_neigh_x = x + ALL_X[opposite_dir];
          int diag_neigh_y = y + ALL_Y[opposite_dir];

          int side_1_neigh_x = x + ALL_X[(opposite_dir + 1) % 8];
          int side_1_neigh_y = y + ALL_Y[(opposite_dir + 1) % 8];

          int side_2_neigh_x = x + ALL_X[(opposite_dir + 7) % 8];
          int side_2_neigh_y = y + ALL_Y[(opposite_dir + 7) % 8];

          if (!IsOpen(diag_neigh_x, diag_neigh_y) ||
              !IsOpen(side_1_neigh_x, side_1_neigh_y) ||
              !IsOpen(side_2_neigh_x, side_2_neigh_y)) {
            // Wall one away.
            dist_[x][y][opposite_dir] = 0;
          } else if (dist_[diag_neigh_x][diag_neigh_y][(opposite_dir + 1) % 8] >
                         0 ||
                     dist_[diag_neigh_x][diag_neigh_y][(opposite_dir + 7) % 8] >
                         0) {
            // Straight jump point one away.
            dist_[x][y][opposite_dir] = 1;
          } else {
            // Increment from last
            int jump_distance = dist_[diag_neigh_x][diag_neigh_y][opposite_dir];
            dist_[x][y][opposite_dir] =
                jump_distance > 0 ? jump_distance + 1 : jump_distance - 1;
          }
        }
      }
    }
  }

  void Preprocess() {
    FindPrimaryPoints();
    SweepCardinalDirections();
    SweepNonCardinalDirections();
  }

  void PrintDistances() {
    for (int i = 1; i <= n_; ++i) {
      for (int j = 1; j <= m_; ++j) {
        if (IsOpen(i, j)) {
          cout << j-1 << " " << i-1;
          for (int dir = 0; dir < 8; ++dir) {
            cout << " " << dist_[i][j][dir];
          }
          cout << "\n";
        }
      }
    }
  }

  void Print() {
    cerr << "\n";
    for (int i = 1; i <= n_; ++i) {
      for (int j = 1; j <= m_; ++j) {
        cerr << board_[i][j] << " ";
        if (board_[i][j] < 10) cerr << " ";
      }
      cerr << "\n";
    }
  }
  inline bool IsOpen(int x, int y) { return IsOpenTile(board_[x][y]); }
  inline bool JumpPoint(int x, int y, AllDirection dir) {
    return ((dir & 1) == 0) && (board_[x][y] & (1 << (1 + (dir / 2))));
  }

  int n_;  // Height of the map
  int m_;  // Width of the map
  vector<vector<Tile>> board_;
  vector<vector<Distances>> dist_;
};

void DebugPrintDist(const Board &board, const AllDirection dir) {
  cerr << "DebugPrintDist in direction " << dir << "\n";
  for (int i = 1; i <= board.n_; ++i) {
    for (int j = 1; j <= board.m_; ++j) {
      cerr << board.dist_[i][j][dir] << " ";
      if (board.dist_[i][j][dir] < 10) cerr << " ";
    }
    cerr << "\n";
  }
}

/**
 * Compute the proper wall / jump point distances, according to the
 *preprocessing phase of the JPS+ algorithm.
 **/
int main() {
  Board board;
  board.ReadBoard();
  board.Preprocess();

  // DebugPrintDist(board, N);
  // DebugPrintDist(board, E);
  // DebugPrintDist(board, S);
  // DebugPrintDist(board, W);

  // DebugPrintDist(board, NE);
  // DebugPrintDist(board, SE);
  // DebugPrintDist(board, SW);
  // DebugPrintDist(board, NW);

  board.PrintDistances();
  // Write an action using cout. DON'T FORGET THE "<< endl"
  // To debug: cerr << "Debug messages..." << endl;

  // For each empty tile of the map, a line containing "column row N NE E SE S
  // SW W NW".
  // cout << "0 0 -1 -1 -1 -1 -1 -1 -1 -1" << endl;
  // cout << "1 2 -1 3 -2 4 0 0 2 -2" << endl;
  // cout << "..." << endl;
}
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

constexpr int OPEN_TILE = 1;
constexpr int CLOSED_TILE = 0;
constexpr int CARDINAL_X[] = {-1, 0, 1, 0};
constexpr int CARDINAL_Y[] = {0, 1, 0, -1};
constexpr int ALL_X[] = {-1, -1, 0, 1, 1, 1, 0, -1};
constexpr int ALL_Y[] = {0, 1, 1, 1, 0, -1, -1, -1};

// enum CanonicalDirection { N, E, S, W };
enum AllDirection { N, NE, E, SE, S, SW, W, NW };

/**
 * Tiles are encoded as follows:
 * - if first bit is OFF then then the tile is closed, else it is open.
 * - if the tile is open it might be a primary jump point, in such a case:
 *   - bits number 1, 2, 3, 4 represent the direction of travel (from a forced
 *neighbour to that point) for that node to be a primary jump point.
 **/
using Tile = int;

inline bool IsOpenTile(const Tile tile) { return (tile & OPEN_TILE); }

struct Board {
  Board() {}
  void ReadBoard() {
    cin >> m_ >> n_;
    cin.ignore();
    // Pad the board with a wall on all 4 sides.
    board_ = vector<vector<Tile>>(n_ + 2, vector<Tile>(m_ + 2, CLOSED_TILE));
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

  void Preprocess() {
    // First, find all the primary points.
    for (int i = 1; i <= n_; ++i) {
      for (int j = 1; j <= m_; ++j) {
        if (IsOpen(i, j)) {
          for (int k = 0; k < 4; ++k) {
            int prev_x = i + CARDINAL_X[k];
            int prev_y = j + CARDINAL_Y[k];
            if (IsOpen(prev_x, prev_y)) {
              CheckForcedNeighbour(i, j, prev_x, prev_y, k,
                                   (k + 1) % 4);
              CheckForcedNeighbour(i, j, prev_x, prev_y, k ,
                                   (k + 3) % 4);
            }
          }
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

  int n_;  // Height of the map
  int m_;  // Width of the map
  vector<vector<Tile>> board_;
};

/**
 * Compute the proper wall / jump point distances, according to the
 *preprocessing phase of the JPS+ algorithm.
 **/
int main() {
  Board board;
  board.ReadBoard();
  board.Preprocess();
  board.Print();
  // Write an action using cout. DON'T FORGET THE "<< endl"
  // To debug: cerr << "Debug messages..." << endl;

  // For each empty tile of the map, a line containing "column row N NE E SE S
  // SW W NW".
  // cout << "0 0 -1 -1 -1 -1 -1 -1 -1 -1" << endl;
  // cout << "1 2 -1 3 -2 4 0 0 2 -2" << endl;
  // cout << "..." << endl;
}
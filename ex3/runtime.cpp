// Implementation based on
// http://www.gameaipro.com/GameAIPro2/GameAIPro2_Chapter14_JPS_Plus_An_Extreme_A_Star_Speed_Optimization_for_Static_Uniform_Cost_Grids.pdf

#include <bits/stdc++.h>
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
constexpr int MAX_SIZE = 20;

// enum CanonicalDirection { N, E, S, W };

enum AllDirection { N, NE, E, SE, S, SW, W, NW };
// First 8 ints represent distances, the last one tells if a tile is open or
// closed.
using Distances = array<int, 9>;

struct Board {
  Board()
      : dist_{vector<vector<Distances>>(
            MAX_SIZE,
            vector<Distances>(
                MAX_SIZE, Distances({0, 0, 0, 0, 0, 0, 0, 0, CLOSED_TILE})))} {}

  void ReadBoard() {
    cin >> m_ >> n_;
    // cin.ignore();
    cin >> start_y >> start_x >> goal_y >> goal_x;
    // cin.ignore();
    int open;  // number of open tiles on the map
    cin >> open;
    //   cin.ignore();
    while (open--) {
      int column;  // coordinate of the empty tile described
      int row;     // coordinate of the empty tile described
      int n;   // distance to the closest jump point (positive number) or wall
               // (otherwise) going north
      int ne;  // distance to the closest jump point (positive number) or wall
               // (otherwise) going northeast
      int e;   // distance to the closest jump point (positive number) or wall
               // (otherwise) going east
      int se;  // distance to the closest jump point (positive number) or wall
               // (otherwise) going southeast
      int s;   // distance to the closest jump point (positive number) or wall
               // (otherwise) going south
      int sw;  // distance to the closest jump point (positive number) or wall
               // (otherwise) going southwest
      int w;   // distance to the closest jump point (positive number) or wall
               // (otherwise) going west
      int nw;  // distance to the closest jump point (positive number) or wall
               // (otherwise) going northwest
      cin >> column >> row >> n >> ne >> e >> se >> s >> sw >> w >> nw;
      dist_[row][column] = {n, ne, e, se, s, sw, w, nw, OPEN_TILE};
      // cin.ignore();
    }
  }

  void PrintDistances() {
    for (int i = 1; i <= n_; ++i) {
      for (int j = 1; j <= m_; ++j) {
        if (IsOpen(i, j)) {
          cout << j - 1 << " " << i - 1;
          for (int dir = 0; dir < 8; ++dir) {
            cout << " " << dist_[i][j][dir];
          }
          cout << "\n";
        }
      }
    }
  }

  inline bool IsOpen(int x, int y) { return dist_[x][y][8] == OPEN_TILE; }
  inline bool IsClosed(int x, int y) { return dist_[x][y][8] == CLOSED_TILE; }

  int n_;       // Height of the map
  int m_;       // Width of the map
  int start_y;  // coordinate of the starting tile
  int start_x;  // coordinate of the starting tile
  int goal_y;   // coordinate of the goal tile
  int goal_x;   // coordinate of the goal tile
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
 * Compute the nodes visited by the JPS+ algorithm when performing runtime phase
 *search.
 **/

AllDirection GetDirection(int x, int y, int prev_x, int prev_y) {
  x -= prev_x;
  y -= prev_y;
  for (int i = 0; i < 8; ++i) {
    if (x == ALL_X[i] && y == ALL_Y[i]) {
      return static_cast<AllDirection>(i);
    }
  }
  throw logic_error("Could not find any matching direction.");
}

inline bool Cardinal(AllDirection dir) { return dir % 2 == 0; }

vector<AllDirection> ValidNextSteps(int x, int y, int prev_x, int prev_y) {
  if (prev_x == -1) return {N, NE, E, SE, S, SW, W, NW};
  AllDirection dir = GetDirection(x, y, prev_x, prev_y);

  vector<AllDirection> res;
  vector<int> offsets({7, 0, 1});
  if (Cardinal(dir)) {
    offsets = {6, 7, 0, 1, 2};
  }

  for (int offset : offsets) {
    res.push_back(static_cast<AllDirection>((dir + offset) % 8));
  }
  return res;
}

int main() {
  Board board;
  board.ReadBoard();
  // Write an action using cout. DON'T FORGET THE "<< endl"
  // To debug: cerr << "Debug messages..." << endl;

  // In order of nodes visited by the JPS+ algorithm, a line containing
  // "nodeColumn nodeRow parentColumn parentRow givenCost".
  cout << board.start_y << " " << board.start_x << " -1 -1 0.00" << endl;

  // -dist, x, y
  priority_queue<tuple<float, int, int>> Q;
  Q.push({0., board.start_x, board.start_y});

  pair<int, int> parent[MAX_SIZE][MAX_SIZE];

  for (int i = 0; i < MAX_SIZE; ++i) {
    for (int j = 0; j < MAX_SIZE; ++j) {
      parent[i][j] = {-1, -1};
    }
  }

  float distance[MAX_SIZE][MAX_SIZE];
  for (int i = 0; i < MAX_SIZE; ++i) {
    for (int j = 0; j < MAX_SIZE; ++j) {
      distance[i][j] = MAX_SIZE * MAX_SIZE * MAX_SIZE;
    }
  }
  distance[board.start_x][board.start_y] = 0.;

  // game loop
  while (!Q.empty()) {
    // Write an action using cout. DON'T FORGET THE "<< endl"
    // To debug: cerr << "Debug messages..." << endl;
    // In order of nodes visited by the JPS+ algorithm, a line containing
    // "nodeColumn nodeRow parentColumn parentRow givenCost".

    float cost = -1 * get<0>(Q.top());
    int x = get<1>(Q.top());
    int y = get<2>(Q.top());
    Q.pop();
    if (x == board.goal_x && y == board.goal_y) {
      return 0;
    }
    int prev_x = parent[x][y].first;
    int prev_y = parent[x][y].second;

    for(const AllDirection dir : ValidNextSteps(x, y, prev_x, prev_y)){
    }

    cout << "3 4 0 2 3.14" << endl;
  }
}
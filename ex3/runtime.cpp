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
  int dir_x = sgn(x - prev_x);
  int dir_y = sgn(y - prev_y);
  for (int i = 0; i < 8; ++i) {
    if (dir_x == ALL_X[i] && dir_y == ALL_Y[i]) {
      return static_cast<AllDirection>(i);
    }
  }
  string error = to_string(prev_x) + "," + to_string(prev_y) + " --> " +
                 to_string(x) + "," + to_string(y);
  throw logic_error("Could not find any matching direction. " + error);
}

inline bool Cardinal(AllDirection dir) { return dir % 2 == 0; }
inline bool Diagonal(AllDirection dir) { return dir % 2 == 1; }

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

bool ExactDirection(int x, int y, AllDirection dir, int goal_x, int goal_y) {
  return (x == goal_x && sgn(ALL_Y[dir]) == sgn(goal_y - y)) ||
         (y == goal_y && sgn(ALL_X[dir]) == sgn(goal_x - x));
}

bool GeneralDirection(int x, int y, AllDirection dir, int goal_x, int goal_y) {
  return sgn(ALL_X[dir]) == sgn(goal_x - x) &&
         sgn(ALL_Y[dir]) == sgn(goal_y - y);
}

inline int ManhatanDist(int x1, int y1, int x2, int y2) {
  return abs(x1 - x2) + abs(y1 - y2);
}

// http://theory.stanford.edu/~amitp/GameProgramming/Heuristics.html
inline double HeuristicGeneral(int x1, int y1, int x2, int y2, double d,
                               double d2) {
  double dx = static_cast<double>(abs(x1 - x2));
  double dy = static_cast<double>(abs(y1 - y2));
  return d * (dx + dy) + (d2 - 2 * d) * min(dx, dy);
}

inline double HeuristicCost(int x1, int y1, int x2, int y2) {
  return HeuristicGeneral(x1, y1, x2, y2, 1.0, M_SQRT2);
}

int main() {
  Board board;
  board.ReadBoard();
  // Write an action using cout. DON'T FORGET THE "<< endl"
  // To debug: cerr << "Debug messages..." << endl;

  // In order of nodes visited by the JPS+ algorithm, a line containing
  // "nodeColumn nodeRow parentColumn parentRow givenCost".
  // cout << board.start_y << " " << board.start_x << " -1 -1 0.00" << endl;

  // -heuristic_dist, x, y
  priority_queue<tuple<double, int, int>> Q;
  Q.push({0., board.start_x, board.start_y});

  pair<int, int> parent[MAX_SIZE][MAX_SIZE];
  bool closed[MAX_SIZE][MAX_SIZE];

  for (int i = 0; i < MAX_SIZE; ++i) {
    for (int j = 0; j < MAX_SIZE; ++j) {
      parent[i][j] = {-1, -1};
      closed[i][j] = false;
    }
  }

  double distance[MAX_SIZE][MAX_SIZE];
  for (int i = 0; i < MAX_SIZE; ++i) {
    for (int j = 0; j < MAX_SIZE; ++j) {
      distance[i][j] = MAX_SIZE * MAX_SIZE * MAX_SIZE;
    }
  }
  distance[board.start_x][board.start_y] = 0.;

  // game loop
  bool found = false;
  while (!Q.empty()) {
    // Write an action using cout. DON'T FORGET THE "<< endl"
    // To debug: cerr << "Debug messages..." << endl;
    // In order of nodes visited by the JPS+ algorithm, a line containing
    // "nodeColumn nodeRow parentColumn parentRow givenCost".

    // double heuristic_cost = -1 * get<0>(Q.top());
    int x = get<1>(Q.top());
    int y = get<2>(Q.top());
    Q.pop();
    if(closed[x][y]){
      continue;
    }
    closed[x][y] = true;

    cout << y << " " << x << " " << parent[x][y].second << " "
         << parent[x][y].first << " " << distance[x][y] << endl;
    if (x == board.goal_x && y == board.goal_y) {
      found = true;
      break;
    }
    int prev_x = parent[x][y].first;
    int prev_y = parent[x][y].second;

    for (const AllDirection dir : ValidNextSteps(x, y, prev_x, prev_y)) {
      // cerr << "poss dir: " << dir << endl;
      int next_x = -1, next_y = -1;
      double cost;
      // cerr << Cardinal(dir) << endl
      //      << ExactDirection(x, y, dir, board.goal_x, board.goal_y) << endl
      //      << ManhatanDist(x, y, board.goal_x, board.goal_y) << endl
      //      << abs(board.dist_[x][y][dir]) << endl;
      if (Cardinal(dir) &&
          ExactDirection(x, y, dir, board.goal_x, board.goal_y) &&
          ManhatanDist(x, y, board.goal_x, board.goal_y) <=
              abs(board.dist_[x][y][dir])) {
        // Goal is closer than wall distance or closer than or equal to jump
        // point distance.
        next_x = board.goal_x;
        next_y = board.goal_y;
        cost = distance[x][y] + ManhatanDist(x, y, board.goal_x, board.goal_y);
      } else if (Diagonal(dir) &&
                 GeneralDirection(x, y, dir, board.goal_x, board.goal_y) &&
                 min(abs(x - board.goal_x), abs(y - board.goal_y)) <=
                     abs(board.dist_[x][y][dir])) {
        // Goal is closer or equal in either row or column than wall or jump
        // point distance.
        int jump_len = min(abs(x - board.goal_x), abs(y - board.goal_y));
        next_x = x + ALL_X[dir] * jump_len;
        next_y = y + ALL_Y[dir] * jump_len;
        cost = distance[x][y] + M_SQRT2 * jump_len;
      } else if (board.dist_[x][y][dir] > 0) {
        // Jump point in this direction.
        int jump_len = board.dist_[x][y][dir];
        next_x = x + ALL_X[dir] * jump_len;
        next_y = y + ALL_Y[dir] * jump_len;
        cost = distance[x][y] + (Diagonal(dir) ? M_SQRT2 : 1.0) * jump_len;
      }

      // Traditional A* from this point.
      if (next_x != -1) {
        // cerr << "pos next move: " << next_x << ", " << next_y << endl;
        if (!closed[next_x][next_y] && cost < distance[next_x][next_y]) {
          parent[next_x][next_y] = {x, y};
          distance[next_x][next_y] = cost;
          double heuristic_cost =
              cost + HeuristicCost(next_x, next_y, board.goal_x, board.goal_y);
          Q.push({-heuristic_cost, next_x, next_y});
        }
      }
    }
  }

  if (!found) {
    cout << "NO PATH" << endl;
  }
}
#include <bits/stdc++.h>

#include <chrono>
using namespace std;

constexpr bool DEBUG_V2 = false;
constexpr bool DEBUG_V1 = true;
constexpr double MARS_GRAVITY = 3.711;  // in m/s^2.
constexpr int MAX_X = 7000;
constexpr int MAX_Y = 3000;
constexpr int MAX_LANDING_HS = 20;
constexpr int MAX_LANDING_VS = 40;

using Point = pair<double, double>;  // x, y.
using Move = pair<int, int>;         // angle, thrust.
enum Status { FLYING, CRASHED, LANDED };

template <typename T>
ostream& operator<<(ostream& os, const pair<T, T>& x) {
  os << x.first << ", " << x.second;
  return os;
}

inline Point operator-(const Point& p, const Point& q) {
  return {p.first - q.first, p.second - q.second};
}

// cross product
inline double operator*(const Point& p, const Point& q) {
  return {p.first * q.second - p.second * q.first};
}

inline double Direction(const Point& origin, const Point& a, const Point& b) {
  return (a - origin) * (b - origin);
}

inline bool SegmentsIntersect(const Point& p1, const Point& p2, const Point& p3,
                              const Point& p4) {
  double d1 = Direction(p3, p4, p1);
  double d2 = Direction(p3, p4, p2);
  double d3 = Direction(p1, p2, p3);
  double d4 = Direction(p1, p2, p4);
  return (((d1 > 0 && d2 < 0) || (d1 < 0 && d2 > 0)) &&
          ((d3 > 0 && d4 < 0) || (d3 < 0 && d4 > 0)));
}

struct State {
  void ReadSurface() {
    int n;
    cin >> n;
    surface_.resize(n);
    cin >> surface_[0].first >> surface_[0].second;
    for (int i = 1; i < n; ++i) {
      double x, y;
      cin >> x >> y;
      surface_[i] = {x, y};
      if (y == surface_[i - 1].second) {
        flat_ground_idx_ = i;
      }
    }
  }

  State() : initialized_{false} { ReadSurface(); }

  void Init(int x, int y, double hs, double vs, int fuel, int rotation,
            int power) {
    pos_ = {x, y};
    hs_ = hs;
    vs_ = vs;
    fuel_ = fuel;
    rotation_ = rotation;
    thrust_ = power;
    prev_pos_ = pos_;
    prev_rotation_ = rotation_;
    prev_hs_ = hs_;
    prev_vs_ = vs_;
    status_ = FLYING;
    initialized_ = true;
  }

  void Simulation() {
    while (!IsTerminal()) {
      Move mv = RandomMove();
      MakeMove(mv);
      if (DEBUG_V2) cerr << "current pos: " << pos_ << endl;
    }
  }

  Move RandomMove() {
    int min_angle = max(rotation_ - 15, -90);
    int max_angle = min(rotation_ + 15, 90);
    int angle = rand() % (max_angle - min_angle + 1) + min_angle;

    int min_thrust = max(0, thrust_ - 1);
    int max_thrust = min(4, thrust_ + 1);

    int thrust = rand() % (max_thrust - min_thrust + 1) + min_thrust;
    return {angle, thrust};
  }

  // For a landing to be successful, the ship must:
  // - land on flat ground
  // - land in a vertical position (tilt angle = 0°)
  // - vertical speed must be limited ( ≤ 40m/s in absolute value)
  // - horizontal speed must be limited ( ≤ 20m/s in absolute value)
  inline bool SuccessfulLanding(const Point& left, const Point& right) {
    return (left.second == right.second && rotation_ == 0 &&
            abs(vs_) < MAX_LANDING_VS && abs(hs_) < MAX_LANDING_HS &&
            prev_rotation_ == 0 && abs(prev_vs_) < MAX_LANDING_VS &&
            abs(prev_hs_) < MAX_LANDING_HS);
  }

  void UpdateStatus() {
    const auto [x, y] = pos_;
    if (x < 0 || x > MAX_X || y > MAX_Y) {
      status_ = CRASHED;
      return;
    }

    for (int i = 1; i < surface_.size(); ++i) {
      Point left = surface_[i - 1];
      Point right = surface_[i];
      if (SegmentsIntersect(prev_pos_, pos_, left, right)) {
        status_ = SuccessfulLanding(left, right) ? LANDED : CRASHED;
      }
    }
  }

  void MakeMove(const Move& mv) {
    prev_rotation_ = rotation_;
    prev_pos_ = pos_;
    prev_vs_ = vs_;
    prev_hs_ = hs_;

    rotation_ = mv.first;
    thrust_ = mv.second;

    if (fuel_ <= 0) {
      thrust_ = 0;
    } else {
      fuel_ -= thrust_;
    }
    MakeStep();
  }

  void MakeStep() {
    double alpha = rotation_ * M_PI / 180.0;
    double ha = -1.0 * sin(alpha) * thrust_;
    double va = cos(alpha) * thrust_ - MARS_GRAVITY;
    pos_.first += hs_ + ha / 2.0;
    pos_.second += vs_ + va / 2.0;

    hs_ += ha;
    vs_ += va;

    UpdateStatus();
  }

  bool IsTerminal() { return status_ != FLYING; }

  Point pos_;
  Point prev_pos_;
  double vs_;  // the vertical speed (in m/s), can be negative.
  double prev_vs_;
  double hs_;  // the horizontal speed (in m/s), can be negative.
  double prev_hs_;
  int thrust_;    // the thrust power (0 to 4).
  int fuel_;      // the quantity of remaining fuel in liters.
  int rotation_;  // the rotation angle in degrees (-90 to 90).
  int prev_rotation_;
  Status status_;

  bool initialized_;

  vector<Point> surface_;
  int flat_ground_idx_;  // flat ground between point i and i+1.
};

int RandomSimulationsUntilTimeout(const State& state, int miliseconds) {
  int cnt = 0;
  chrono::steady_clock::time_point begin = std::chrono::steady_clock::now();
  int diff = 0;
  while (diff < miliseconds) {
    cnt++;
    State state_cpy = state;
    state_cpy.Simulation();
    chrono::steady_clock::time_point end = std::chrono::steady_clock::now();
    diff =
        chrono::duration_cast<std::chrono::milliseconds>(end - begin).count();
  }
  if (DEBUG_V2) cerr << "Time difference = " << diff << " [ms] " << endl;
  return cnt;
}

int main() {
  State engine;
  // game loop
  while (1) {
    int x;
    int y;
    int hs;  // the horizontal speed (in m/s), can be negative.
    int vs;  // the vertical speed (in m/s), can be negative.
    int f;   // the quantity of remaining fuel in liters.
    int r;   // the rotation angle in degrees (-90 to 90).
    int p;   // the thrust power (0 to 4).
    cin >> x >> y >> hs >> vs >> f >> r >> p;
    cin.ignore();
    if (!engine.initialized_) {
      engine.Init(x, y, hs, vs, f, r, p);
    }

    if(DEBUG_V1) cerr << "Num of random simulations: " <<  RandomSimulationsUntilTimeout(engine, 95);

    // Write an action using cout. DON'T FORGET THE "<< endl"
    // To debug: cerr << "Debug messages..." << endl;

    // R P. R is the desired rotation angle. P is the desired thrust power.
    cout << "-20 3" << endl;
  }
}
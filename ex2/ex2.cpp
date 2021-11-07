#include <bits/stdc++.h>

#include <chrono>

using namespace std;

constexpr bool DEBUG_V2 = false;
constexpr bool DEBUG_V1 = false;
constexpr bool DEBUG_V0 = false;
constexpr double MARS_GRAVITY = 3.711; // in m/s^2.
constexpr int MAX_X = 7000;
constexpr int MAX_Y = 3000;
constexpr int MAX_LANDING_HS = 20;
constexpr int MAX_LANDING_VS = 40;
constexpr int MAX_ROTATION = 90;
constexpr int MIN_ROTATION = -90;
constexpr int MAX_THRUST = 4;
constexpr int MIN_THRUST = 0;
constexpr size_t CHROMOSOME_LENGTH = 220;
constexpr size_t POPULATION_SIZE = 100;
constexpr size_t ELITISM = 12;
constexpr double GENE_MUTATION_CHANCE = 0.06;

enum Status { FLYING, CRASHED, LANDED };
using Point = pair<double, double>;                  // x, y.
using Move = pair<int, int>;                         // angle, thrust.
using ChromosomeScore = tuple<double, Status, bool>; // score, Status, terminal
                                                     // after applying moves.

template <typename T, typename C>
ostream &operator<<(ostream &os, const pair<T, C> &x) {
  os << x.first << " " << x.second;
  return os;
}

double DoubleRand() { return (double)rand() / RAND_MAX; }

bool IsWinningScore(const ChromosomeScore &score) {
  return get<1>(score) == LANDED && get<2>(score) == true;
}

Point operator-(const Point &p, const Point &q) {
  return {p.first - q.first, p.second - q.second};
}

Point operator+(const Point &p, const Point &q) {
  return {p.first + q.first, p.second + q.second};
}

Point operator/(const Point &p, const double d) {
  return {p.first / d, p.second / d};
}

double len2(const Point &v) { return v.first * v.first + v.second * v.second; }

double dist2(const Point &p, const Point &q) { return len2(p - q); }
// cross product
double operator*(const Point &p, const Point &q) {
  return p.first * q.second - p.second * q.first;
}

double Direction(const Point &origin, const Point &a, const Point &b) {
  return (a - origin) * (b - origin);
}

bool SegmentsIntersect(const Point &p1, const Point &p2, const Point &p3,
                       const Point &p4) {
  double d1 = Direction(p3, p4, p1);
  double d2 = Direction(p3, p4, p2);
  double d3 = Direction(p1, p2, p3);
  double d4 = Direction(p1, p2, p4);
  return (((d1 > 0 && d2 < 0) || (d1 < 0 && d2 > 0)) &&
          ((d3 > 0 && d4 < 0) || (d3 < 0 && d4 > 0)));
}

// X ~ Gamma(a, 1) and Y ~ Gamma(b, 1) then Y / (X+Y) ~ Beta(a, b)
struct BetaGenarator {
  BetaGenarator(double a, double b) : alpha_{a}, beta_{b} {
    gamma_distribution_a_1_ = gamma_distribution<double>(a, 1.0);
    gamma_distribution_b_1_ = gamma_distribution<double>(b, 1.0);
  }

  double Rand() {
    double x = gamma_distribution_a_1_(generator_);
    double y = gamma_distribution_b_1_(generator_);
    return x / (x + y);
  }

  default_random_engine generator_;
  gamma_distribution<double> gamma_distribution_a_1_;
  gamma_distribution<double> gamma_distribution_b_1_;
  double alpha_, beta_;
};

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
        flat_ground_idx_ = i - 1;
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

  void Validate(int x, int y, double hs, double vs, int fuel, int rotation,
                int power) {
    const double eps = 1.0;
    if (abs(pos_.first - x) > eps)
      cerr << "Validate x: " << pos_.first << " input: " << x;
    if (abs(pos_.second - y) > eps)
      cerr << "Validate y: " << pos_.second << " input: " << y;
    if (abs(hs_ - hs) > eps)
      cerr << "Validate hs: " << hs_ << " input: " << hs;
    if (abs(vs_ - vs) > eps)
      cerr << "Validate vs: " << vs_ << " input: " << vs;
    if (abs(fuel_ - fuel) > eps)
      cerr << "Validate fuel: " << fuel_ << " input: " << fuel;
    if (abs(rotation_ - rotation) > eps)
      cerr << "Validate rotation: " << rotation_ << " input: " << rotation;
    if (abs(thrust_ - power) > eps)
      cerr << "Validate power: " << thrust_ << " input: " << power;
  }

  void Simulation() {
    while (!IsTerminal()) {
      Move mv = RandomMove();
      MakeMove(mv);
      if (DEBUG_V2)
        cerr << "current pos: " << pos_ << endl;
    }
  }

  Move RandomMove() const {
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
  bool SuccessfulLanding(const Point &left, const Point &right) const {
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

    for (size_t i = 1; i < surface_.size(); ++i) {
      const Point &left = surface_[i - 1];
      const Point &right = surface_[i];
      bool negated_necessary_condition =
          (pos_.second > left.second && prev_pos_.second > right.second) ||
          (pos_.first > right.first && prev_pos_.first > right.first) ||
          (pos_.first < left.first && prev_pos_.first < left.first);

      if (!negated_necessary_condition &&
          SegmentsIntersect(prev_pos_, pos_, left, right)) {
        status_ = SuccessfulLanding(left, right) ? LANDED : CRASHED;
        break;
      }
    }
  }

  void MakeMove(const Move &mv) {
    if (DEBUG_V2 || DEBUG_V1) {
      if (abs(rotation_ - mv.first) > 15)
        cerr << "Rotation change must be no more than 15 degrees. Previous: "
             << rotation_ << " requested: " << mv.first;
      if (abs(thrust_ - mv.second) > 1)
        cerr << "Thrust change must be no more than +/-1 . Previous: "
             << thrust_ << " requested: " << mv.second;
      if (status_ != FLYING)
        cerr << "Move not possible in status " << status_;
    }

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

  bool IsTerminal() const { return status_ != FLYING; }

  // For a landing to be successful, the ship must:
  // - land on flat ground
  // - land in a vertical position (tilt angle = 0°)
  // - vertical speed must be limited ( ≤ 40m/s in absolute value)
  // - horizontal speed must be limited ( ≤ 20m/s in absolute value)
  double Score() const {
    if (status_ == LANDED) {
      return 0;
    }

    Point flat_left = surface_[flat_ground_idx_];
    Point flat_right = surface_[flat_ground_idx_ + 1];
    double flat_x_1 = flat_left.first;
    double flat_x_2 = flat_right.first;
    double flat_y = flat_left.second + 1.;

    if (DEBUG_V2 && flat_y != flat_right.second)
      cerr << "ERROR: flat ground wrong idx." << endl;

    double distance_penalty = max(0., (pos_.first - (flat_x_1 + 150)) *
                                          (pos_.second - (flat_x_2 - 150)));
    distance_penalty = sqrt(distance_penalty);

    const double vv_mult = 5;
    const double vh_mult = 1;
    const double anMult = 1;

    double vh_penalty = vh_mult * max(0., 1. + abs(hs_) - MAX_LANDING_HS + 2);
    double vv_penalty = vv_mult * max(0., 1. + abs(vs_) - MAX_LANDING_VS + 4);
    double anglePenalty = anMult * rotation_ * rotation_;
    vh_penalty *= vh_penalty;
    vv_penalty *= vv_penalty;

    double vh_prev_penalty =
        vh_mult * max(0., 1. + abs(prev_hs_) - MAX_LANDING_HS + 2);
    double vv_prev_penalty =
        vv_mult * max(0., 1. + abs(prev_vs_) - MAX_LANDING_VS + 4);
    double angle_prev_penalty = anMult * prev_rotation_ * prev_rotation_;
    vh_prev_penalty *= vh_prev_penalty;
    vv_prev_penalty *= vv_prev_penalty;

    vh_penalty = (1 + vh_penalty + vh_prev_penalty) / 2;
    vv_penalty = (1 + vv_penalty + vv_prev_penalty) / 2;
    anglePenalty = (1 + anglePenalty + angle_prev_penalty) / 2;

    if (status_ == FLYING) {
      anglePenalty = 0.;
      distance_penalty /= 100.;
    }

    double penalty;
    if (!(flat_x_1 < pos_.first && pos_.first < flat_x_2))
      penalty = 5;
    else if (abs(flat_y - pos_.second) >= MAX_LANDING_VS)
      penalty = 4;
    else if (abs(vs_) >= MAX_LANDING_VS)
      penalty = 3;
    else if (abs(hs_) >= MAX_LANDING_HS)
      penalty = 2;
    else if (rotation_ != 0 || prev_rotation_ != 0)
      penalty = 1;
    else
      penalty = 0;
    penalty *= 20000.;

    double score =
        distance_penalty + vh_penalty + vv_penalty + anglePenalty + penalty;

    return score;
  }

  Point pos_;
  Point prev_pos_;
  double vs_; // the vertical speed (in m/s), can be negative.
  double prev_vs_;
  double hs_; // the horizontal speed (in m/s), can be negative.
  double prev_hs_;
  int thrust_;   // the thrust power (0 to 4).
  int fuel_;     // the quantity of remaining fuel in liters.
  int rotation_; // the rotation angle in degrees (-90 to 90).
  int prev_rotation_;
  Status status_;

  bool initialized_;

  vector<Point> surface_;
  int flat_ground_idx_; // flat ground between point i and i+1.
};

int RandomSimulationsUntilTimeout(const State &state, int miliseconds) {
  int cnt = 0;
  chrono::steady_clock::time_point begin = chrono::steady_clock::now();
  int diff = 0;
  while (diff < miliseconds) {
    cnt++;
    State state_cpy = state;
    state_cpy.Simulation();
    chrono::steady_clock::time_point end = chrono::steady_clock::now();
    diff = chrono::duration_cast<chrono::milliseconds>(end - begin).count();
  }
  if (DEBUG_V2)
    cerr << "Time difference = " << diff << " [ms] " << endl;
  return cnt;
}

struct Gene {
  // Initialize randomly.
  Gene() : angle_delta_{(rand() % 31) - 15}, thrust_detla_{1} {}

  Move ToMove(const State &state) const {
    int rotation =
        clamp(state.rotation_ + (int)angle_delta_, MIN_ROTATION, MAX_ROTATION);
    int thrust =
        clamp(state.thrust_ + (int)thrust_detla_, MIN_THRUST, MAX_THRUST);

    if (DEBUG_V2) {
      if (abs(state.thrust_ - thrust) > 1)
        cerr << "ERROR: state.thrust_ = " << state.thrust_
             << ", thrust = " << thrust;
      if (abs(state.rotation_ - rotation) > 15)
        cerr << "ERROR: state.rotation_ = " << state.rotation_
             << ", rotation = " << rotation;
    }
    return {rotation, thrust};
  }

  void RandomMutation() {
    angle_delta_ = (rand() % 31) - 15;
    thrust_detla_ = (rand() % 3) - 1;
  }

  // Doubles to allow flexible manipulations (mutations/ crossovers). Will be
  // casetd to ints once applied as a move.
  double angle_delta_;  // new_angle = angle_ +/-15.
  double thrust_detla_; // new_thrust = thrust_ +/-1.
};

struct Chromosome {
  Chromosome() : dna_{deque<Gene>(CHROMOSOME_LENGTH)} {}

  ChromosomeScore Evaluate(State state) const {
    if (DEBUG_V2)
      cerr << "Evaluate chromosome score for state" << state.pos_ << endl;
    for (size_t i = 0; (i < dna_.size()) && (!state.IsTerminal()); ++i) {
      Move mv = dna_[i].ToMove(state);
      state.MakeMove(mv);
      if (DEBUG_V2)
        cerr << "DNA move: " << mv << "moved to " << state.pos_ << endl;
    }
    bool terminal_reached_within_dna = state.IsTerminal();
    while (!state.IsTerminal()) {
      Move mv = state.RandomMove();
      state.MakeMove(mv);
      if (DEBUG_V2)
        cerr << "ranom move: " << state.pos_ << endl;
    }
    if (DEBUG_V2)
      cerr << "State finaly in: " << state.pos_ << endl
           << "With score: " << state.Score() << endl;

    return {state.Score(), state.status_, terminal_reached_within_dna};
  }

  Gene Rollout() {
    Gene first_gene = dna_.front();
    dna_.pop_front();
    dna_.push_back(Gene());
    return first_gene;
  }

  void Mutation() {
    for (Gene &gene : dna_) {
      if (DoubleRand() < GENE_MUTATION_CHANCE) {
        gene.RandomMutation();
      }
    }
  }
  deque<Gene> dna_;
};

bool cmp(const pair<double, Chromosome> &a, const pair<double, Chromosome> &b) {
  return a.first < b.first;
}

struct Population {
  Population()
      : winning_idx_{nullopt},
        individuals_{vector<Chromosome>(POPULATION_SIZE)}, beta_{BetaGenarator(
                                                               1., 3.)} {}

  void NextGeneration(const State &state) {
    // Already know how to win.
    if (winning_idx_.has_value()) {
      if (DEBUG_V2)
        cerr << "Already know how to win. No new genration." << endl;
      return;
    }
    Sort(state);
    // Already know how to win.
    if (winning_idx_.has_value()) {
      if (DEBUG_V2)
        cerr << "No new genration." << endl;
      return;
    }

    // Keep first ELITISM num of chromosoms.
    vector<Chromosome> children;
    while (children.size() < individuals_.size() - ELITISM) {
      auto [parent_1, parent_2] = SelectParents();
      auto [child_1, child_2] = Crossover(parent_1, parent_2);
      children.push_back(child_1);
      children.push_back(child_2);
    }

    for (size_t i = ELITISM; i < individuals_.size(); ++i) {
      individuals_[i] = children.back();
      children.pop_back();
    }
  }

  pair<Chromosome, Chromosome> SelectParents() {
    const Chromosome &parent_1 =
        individuals_[beta_.Rand() * individuals_.size()];
    const Chromosome &parent_2 =
        individuals_[beta_.Rand() * individuals_.size()];
    return {parent_1, parent_2};
  }

  pair<Chromosome, Chromosome> Crossover(const Chromosome &parent_1,
                                         const Chromosome &parent_2) {
    Chromosome child_1, child_2;
    double rho = DoubleRand();
    for (size_t i = 0; i < CHROMOSOME_LENGTH; ++i) {
      child_1.dna_[i].angle_delta_ =
          parent_1.dna_[i].angle_delta_ * rho +
          parent_2.dna_[i].angle_delta_ * (1.0 - rho);
      child_1.dna_[i].thrust_detla_ =
          parent_1.dna_[i].thrust_detla_ * rho +
          parent_2.dna_[i].thrust_detla_ * (1.0 - rho);

      child_2.dna_[i].angle_delta_ =
          parent_1.dna_[i].angle_delta_ * (1.0 - rho) +
          parent_2.dna_[i].angle_delta_ * rho;
      child_1.dna_[i].thrust_detla_ =
          parent_1.dna_[i].thrust_detla_ * (1.0 - rho) +
          parent_2.dna_[i].thrust_detla_ * rho;
    }
    child_1.Mutation();
    child_2.Mutation();
    return {child_1, child_2};
  }

  Gene RolloutAndReturnBestGene(const State &state) {
    if (winning_idx_.has_value()) {
      if (DEBUG_V0)
        cerr << "No rollout." << endl;
      const int idx = winning_idx_.value();
      return individuals_[idx].Rollout();
    }
    Sort(state);
    Gene best = individuals_[0].dna_.front();
    for (Chromosome &chromosome : individuals_) {
      chromosome.Rollout();
    }
    return best;
  }

  void Sort(const State &state) {
    vector<pair<double, Chromosome>> zip;
    for (size_t i = 0; i < individuals_.size(); ++i) {
      const Chromosome &chromosome = individuals_[i];
      ChromosomeScore score = chromosome.Evaluate(state);
      if (IsWinningScore(score)) {
        winning_idx_ = i;
        return;
      }
      zip.push_back({get<0>(score), chromosome});
    }
    sort(zip.begin(), zip.end(), cmp);
    for (size_t i = 0; i < individuals_.size(); ++i) {
      individuals_[i] = zip[i].second;
    }
    if (DEBUG_V0)
      cerr << "Sorted. Best score is " << zip[0].first << endl;
  }

  optional<int> winning_idx_;
  vector<Chromosome> individuals_;
  BetaGenarator beta_;
};

int main() {
  // BetaGenarator beta_(1.0, 5.0);
  // for (int i = 0; i < 500; ++i) cout << beta_.Rand() << endl;

  // return 0;

  // State engine;
  // Population populationX;
  // int timeout = 950;

  // for (Gene gene : populationX.individuals_[0].dna_) {
  //   cout << gene.ToMove() << endl;
  // }

  State engine;
  Population population;
  int timeout = 900;

  // game loop
  while (1) {
    int x;
    int y;
    int hs; // the horizontal speed (in m/s), can be negative.
    int vs; // the vertical speed (in m/s), can be negative.
    int f;  // the quantity of remaining fuel in liters.
    int r;  // the rotation angle in degrees (-90 to 90).
    int p;  // the thrust power (0 to 4).
    cin >> x >> y >> hs >> vs >> f >> r >> p;
    cin.ignore();
    if (!engine.initialized_) {
      engine.Init(x, y, hs, vs, f, r, p);
    } else if (DEBUG_V1 || DEBUG_V2)
      engine.Validate(x, y, hs, vs, f, r, p);

    chrono::steady_clock::time_point begin = chrono::steady_clock::now();
    int diff = 0, cnt = 0;
    while (!population.winning_idx_.has_value() && diff < timeout) {
      cnt++;
      population.NextGeneration(engine);
      chrono::steady_clock::time_point end = chrono::steady_clock::now();
      diff = chrono::duration_cast<chrono::milliseconds>(end - begin).count();
    }
    timeout = 85;
    if (DEBUG_V0)
      cerr << "Time difference is = " << diff << " [ms] " << endl
           << "Num of generations: " << cnt << endl;

    Gene best_gene = population.RolloutAndReturnBestGene(engine);
    Move mv = best_gene.ToMove(engine);
    engine.MakeMove(mv);

    // cerr << "Num of random simulations: "
    //      << RandomSimulationsUntilTimeout(engine, 95);
    // Write an action using cout. DON'T FORGET THE "<< endl"
    // To debug: cerr << "Debug messages..." << endl;

    // R P. R is the desired rotation angle. P is the desired thrust power.
    // Move mv = engine.RandomMove();
    // engine.MakeMove(mv);
    cout << mv << endl;
    // cout << "-10 1" << endl;
    // engine.MakeMove({-10, 1});
  }
}
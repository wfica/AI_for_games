#include <bits/stdc++.h>
using namespace std;

/**
 * Auto-generated code below aims at helping you parse
 * the standard input according to the problem statement.
 **/

struct pair_hash {
  template <class T1, class T2>
  std::size_t operator()(const std::pair<T1, T2> &p) const {
    auto h1 = hash<T1>{}(p.first);
    auto h2 = hash<T2>{}(p.second);
    return h1 ^ h2;
  }
};

ostream &operator<<(ostream &os, const pair<int, int> &x) {
  os << x.first << ", " << x.second;
  return os;
}

constexpr bool VERBOSE = false;
constexpr int FLAT_MC_REPS = 100;
using Stone = pair<int, int>;
using WinCnt = pair<int, int>;
using PossibleMoves = unordered_map<Stone, WinCnt, pair_hash>;
enum StoneType { EMPTY, X, O };

StoneType OpponentStoneType(const StoneType &my_type) {
  return my_type == X ? O : X;
}

float WinRatio(WinCnt stats) {
  if (stats.second == 0)
    return 0.;
  else
    return 1. * stats.first / stats.second;
}

class IBoard {
 public:
  virtual void PlaceStone(const Stone &place, const StoneType xo) = 0;
  // Copies the board. Performs my_mv and then random moves. Returns the winner
  // of the random game.
  virtual StoneType RandomPolicy(const Stone &my_mv) = 0;
  virtual StoneType Winner() = 0;
  virtual const unordered_set<Stone, pair_hash> &GetPossibleMoves() = 0;
  virtual const StoneType &GetCurrentPlayer() = 0;
};

class SimpleBoard : public IBoard {
 public:
  SimpleBoard(const StoneType &curr_player) : current_player_{curr_player} {
    for (int i = 0; i < 3; ++i)
      for (int j = 0; j < 3; ++j) {
        tab[i][j] = EMPTY;
        possible_moves_.insert({i, j});
      }
  }

  SimpleBoard(const map<Stone, StoneType> &stones, const StoneType curr_player)
      : current_player_{curr_player} {
    for (int i = 0; i < 3; ++i)
      for (int j = 0; j < 3; ++j) {
        tab[i][j] = EMPTY;
      }

    for (auto &[stone, type] : stones) {
      const auto [x, y] = stone;
      tab[x][y] = type;
    }
    for (int i = 0; i < 3; ++i)
      for (int j = 0; j < 3; ++j)
        if (tab[i][j] == EMPTY) possible_moves_.insert({i, j});
  }
  SimpleBoard(const SimpleBoard &other) {
    for (int i = 0; i < 3; ++i)
      for (int j = 0; j < 3; ++j) tab[i][j] = other.tab[i][j];
    current_player_ = other.current_player_;
    possible_moves_ = other.possible_moves_;
  }
  void PlaceStone(const Stone &mv, const StoneType xo) override {
    const auto [x, y] = mv;
    if (x == -1 && y == -1) return;

    tab[x][y] = xo;
    possible_moves_.erase(mv);
    current_player_ = OpponentStoneType(current_player_);
  }
  StoneType RandomPolicy(const Stone &mv) override {
    if (VERBOSE) cerr << "RandomPolicy" << endl;

    SimpleBoard cpy = *this;

    cpy.PlaceStone(mv, current_player_);

    while (cpy.Winner() == EMPTY && !cpy.possible_moves_.empty()) {
      auto it = cpy.possible_moves_.begin();
      advance(it, rand() % cpy.possible_moves_.size());
      Stone move = *it;
      if (VERBOSE) cerr << cpy.current_player_ << " chose " << move << endl;

      cpy.PlaceStone(move, cpy.current_player_);
    }
    if (VERBOSE) cerr << "RandomPolicy winner" << cpy.Winner() << endl;

    return cpy.Winner();
  }

  const unordered_set<Stone, pair_hash> &GetPossibleMoves() override {
    return possible_moves_;
  }

  const StoneType &GetCurrentPlayer() override { return current_player_; }

  StoneType Winner() override {
    for (int i = 0; i < 3; ++i) {
      int cnt_me = 0;
      int cnt_other = 0;
      for (int j = 0; j < 3; ++j) {
        if (tab[i][j] == X) {
          cnt_me++;
        } else if (tab[i][j] == O) {
          cnt_other++;
        }
      }
      if (cnt_other == 3) {
        return O;
      }
      if (cnt_me == 3) {
        return X;
      }
    }

    for (int i = 0; i < 3; ++i) {
      int cnt_me = 0;
      int cnt_other = 0;
      for (int j = 0; j < 3; ++j) {
        if (tab[j][i] == X) {
          cnt_me++;
        } else if (tab[i][j] == O) {
          cnt_other++;
        }
      }
      if (cnt_other == 3) {
        return O;
      }
      if (cnt_me == 3) {
        return X;
      }
    }

    if (tab[0][0] == tab[1][1] && tab[1][1] == tab[2][2]) {
      return tab[1][1];
    }
    if (tab[0][2] == tab[1][1] && tab[1][1] == tab[2][0]) {
      return tab[1][1];
    }
    return EMPTY;
  }
  StoneType tab[3][3];
  unordered_set<Stone, pair_hash> possible_moves_;
  StoneType current_player_;
};

template <typename TBoard>
class IAgent {
 public:
  IAgent(TBoard &board, StoneType my_stone_type)
      : board_{board}, my_stone_type_{my_stone_type} {
    static_assert(is_base_of<IBoard, TBoard>::value == true);
  }
  virtual Stone MyMove() = 0;
  void OpponentMove(const Stone &position) {
    board_.PlaceStone(position, OpponentStoneType(my_stone_type_));
  }

 public:
  TBoard &board_;
  StoneType my_stone_type_;
};

template <typename TBoard>
class FlatMCAgent : public IAgent<IBoard> {
 public:
  FlatMCAgent(TBoard &board, StoneType my_stone_type)
      : IAgent(board, my_stone_type) {}

  int WinnerToRate(const StoneType &winner) {
    if (winner == EMPTY) return 0;
    if (winner == board_.GetCurrentPlayer()) return 1;
    return -1;
  }

  Stone MyMoveInternal() {
    unordered_map<Stone, WinCnt, pair_hash> mvs;
    for (const Stone &mv : board_.GetPossibleMoves()) {
      mvs[mv] = {0, 0};
    }
    if (mvs.size() == 9) {
      return {1, 1};
    }
    int reps = FLAT_MC_REPS;
    if (VERBOSE) cerr << "FLAT MC moves available " << mvs.size() << endl;

    while (reps--) {
      //   if (VERBOSE) cerr << "reps left " << reps;
      auto it = mvs.begin();
      advance(it, rand() % mvs.size());
      const Stone &mv = it->first;
      const StoneType &winner = board_.RandomPolicy(mv);
      mvs[mv].second++;
      mvs[mv].first += WinnerToRate(winner);
    }

    float best = WinRatio(mvs.begin()->second);
    Stone best_mv = mvs.begin()->first;
    if (VERBOSE) cerr << "Ratings:" << endl;
    for (const auto &[stone, stats] : mvs) {
      float curr_win_ratio = WinRatio(stats);
      if (VERBOSE) cerr << "stone: " << stone << " stats: " << stats << endl;
      if (curr_win_ratio > best) {
        best = curr_win_ratio;
        best_mv = stone;
      }
    }

    return best_mv;
  }

  Stone MyMove() override {
    const Stone best_mv = MyMoveInternal();
    board_.PlaceStone(best_mv, board_.GetCurrentPlayer());
    return best_mv;
  }
};

int main() {
  //   SimpleBoard sb({{{0, 0}, O}, {{1, 1}, O}}, X);
  SimpleBoard sb(X);
  FlatMCAgent<SimpleBoard> flat_mc_agent(sb, X);

  // cout << flat_mc_agent.MyMove();

  // game loop
  while (1) {
    int opponentRow;
    int opponentCol;
    cin >> opponentRow >> opponentCol;
    // cerr << "opponentRow" << opponentRow << endl;
    cin.ignore();
    flat_mc_agent.OpponentMove({opponentRow, opponentCol});
    int validActionCount;
    cin >> validActionCount;
    cin.ignore();
    vector<Stone> moves(validActionCount);
    for (int i = 0; i < validActionCount; i++) {
      int row;
      int col;
      cin >> row >> col;
      cin.ignore();
      moves[i] = {row, col};
    }

    // Write an action using cout. DON'T FORGET THE "<< endl"
    // To debug: cerr << "Debug messages..." << endl;

    // int random = rand() % validActionCount;
    // cout << moves[random].first << " " << moves[random].second << endl;

    // cerr << "Debug messages..." << endl;

    auto [x, y] = flat_mc_agent.MyMove();
    cout << x << " " << y << endl;
  }
}

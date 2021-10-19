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

using Stone = pair<int, int>;
using WinCnt = pair<int, int>;
using PossibleMoves = unordered_map<Stone, WinCnt, pair_hash>;
enum StoneType { EMPTY, MY, OTHER };

StoneType OpponentStoneType(const StoneType &my_type) {
  return my_type == MY ? OTHER : MY;
}

class IBoard {
 public:
  virtual void PlaceStone(const Stone &place, const StoneType xo) = 0;
};


class SimpleBoard : public IBoard{
    void PlaceStone(const Stone &place, const StoneType xo) override {
        const auto [x, y] = place;
        tab[x][y] = xo;
    }
    StoneType tab[3][3];
};

template <typename TBoard>
class IAgent {
 public:
  IAgent(const TBoard &board, StoneType my_stone_type)
      : board_{board}, my_stone_type_{my_stone_type_} {
    static_assert(is_base_of<IBoard, TBoard>::value == true);
  }
  virtual Stone MyMove() = 0;
  void OpponentMove(const Stone &position) {
    board_.PlaceStone(position, OpponentStoneType(my_stone_type_));
  }

 private:
  TBoard &board_;
  StoneType my_stone_type_;
};

float WinRatio(WinCnt stats) {
  if (stats.second == 0)
    return 0.;
  else
    return 1. * stats.first / stats.second;
}

struct Board {
  Board(const vector<Stone> &my, const vector<Stone> &other) {
    for (int i = 0; i < 3; ++i) {
      for (int j = 0; j < 3; ++j) {
        tab[i][j] = EMPTY;
      }
    }
    for (auto [x, y] : my) {
      tab[x][y] = MY;
    }
    for (auto [x, y] : other) {
      tab[x][y] = OTHER;
    }

    for (int i = 0; i < 3; ++i) {
      for (int j = 0; j < 3; ++j) {
        possMoves.insert({{i, j}, {0, 0}});
      }
    }
  }

  Board(const Board &other) {
    for (int i = 0; i < 3; ++i) {
      for (int j = 0; j < 3; ++j) {
        tab[i][j] = other.tab[i][j];
      }
    }
    possMoves = other.possMoves;
  }

  StoneType Winner() const {
    for (int i = 0; i < 3; ++i) {
      int cnt_me = 0;
      int cnt_other = 0;
      for (int j = 0; j < 3; ++j) {
        if (tab[i][j] == MY) {
          cnt_me++;
        } else if (tab[i][j] == OTHER) {
          cnt_other++;
        }
      }
      if (cnt_other == 3) {
        return OTHER;
      }
      if (cnt_me == 3) {
        return MY;
      }
    }

    for (int i = 0; i < 3; ++i) {
      int cnt_me = 0;
      int cnt_other = 0;
      for (int j = 0; j < 3; ++j) {
        if (tab[j][i] == MY) {
          cnt_me++;
        } else if (tab[i][j] == OTHER) {
          cnt_other++;
        }
      }
      if (cnt_other == 3) {
        return OTHER;
      }
      if (cnt_me == 3) {
        return MY;
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

  Board Move(const Stone move, const StoneType player) {
    auto [x, y] = move;
    Board cpy = *this;
    cpy.tab[x][y] = player;
    cpy.possMoves.erase(move);
    return cpy;
  }

  StoneType NextPlayer(const StoneType current) {
    if (current == MY)
      return OTHER;
    else
      return MY;
  }

  bool PlayRandomlyUntillEnd(const Stone &my_mv) {
    //   cerr << "Playing randomly" << endl;
    Board cpy = Move(my_mv, MY);
    StoneType turn = OTHER;

    while (cpy.Winner() == EMPTY && !cpy.possMoves.empty()) {
      auto it = cpy.possMoves.begin();
      advance(it, rand() % cpy.possMoves.size());
      Stone move = it->first;
      cpy = cpy.Move(move, turn);
      //   cerr << "Player " << turn << "  --- ";
      //   cerr << "Chose " << move.first << " "<< move.second;
      turn = NextPlayer(turn);
    }
    return cpy.Winner() == MY;
  }

  void FlatMC(int tries = 100) {
    //   cerr << "FlatMC" << endl;
    while (tries--) {
      // cerr << "tries left " << tries << endl;
      auto it = possMoves.begin();
      advance(it, rand() % possMoves.size());
      const Stone &mv = it->first;
      bool result = PlayRandomlyUntillEnd(mv);
      it->second.first += result;
      it->second.second++;
    }
  }

  Stone FindBestMove() {
    if (possMoves.size() == 9) {
      return {1, 1};
    }
    FlatMC();
    float best = WinRatio(possMoves.begin()->second);
    Stone best_mv = possMoves.begin()->first;
    for (const auto [stone, stats] : possMoves) {
      float curr_win_ratio = WinRatio(stats);
      if (curr_win_ratio > best) {
        best = curr_win_ratio;
        best_mv = stone;
      }
    }
    return best_mv;
    // vector<float> ratios;
    // transform(possMoves.begin(), possMoves.end(), back_inserter(ratios),
    //           [](const pair<Stone, WinCnt> &x) { return WinRatio(x.second);
    //           });
  }

  PossibleMoves possMoves;
  StoneType tab[3][3];
};

int main() {
  cerr << WinRatio({7, 10});
  SimpleBoard sb;

  // game loop
  Board board({}, {});
  while (1) {
    int opponentRow;
    int opponentCol;
    cin >> opponentRow >> opponentCol;
    cin.ignore();
    board = board.Move({opponentRow, opponentCol}, OTHER);
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

    auto [x, y] = board.FindBestMove();
    cout << x << " " << y << endl;

    board = board.Move({x, y}, MY);
  }
}

#include <bits/stdc++.h>

using namespace std;

array<int, 8> X = {-1, -1, 0, 1, 1, 1, 0, -1};
array<int, 8> Y = {0, 1, 1, 1, 0, -1, -1, -1};

void Hammer() {
  for (int i = 0; i < 8; ++i) {
    int prev = (i + 7) % 8;
    int next = (i + 1) % 8;
    cout << "{{" << X[prev] << "," << Y[prev] << "},{" << X[i] << "," << Y[i]
         << "},{" << X[next] << "," << Y[next] << "}},";
  }
}


void Scythe(){
  for (int i = 0; i < 8; ++i) {
    cout << "{{" << X[i] << "," << Y[i] << "},{" << 2 * X[i] << "," << 2 * Y[i]
         << "}},";
  }  
}

void Bow(){
  for (int i = -3; i < 4; ++i) {
      for (int j = -3; j < 4; ++j) {
          cout << "{{" << i << ","<<j <<"}},";
      }
  }
}

int main() {
  priority_queue<int> Q;
  Q.push(1);
  Q.push(2);
  cout<< Q.top()<<endl;
}
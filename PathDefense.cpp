#include <iostream>
#include <vector>
#include <map>
#include <algorithm>
#include <limits.h>
#include <string>
#include <string.h>
#include <sstream>
#include <set>
#include <cstdio>
#include <cstdlib>
#include <cmath>
#include <stack>
#include <queue>

using namespace std;

typedef long long ll;

const int UNDEFINED = -1; // 値が未定義

const int MAX_N = 60 + 2; // ボードの最大長(番兵込み)

// 敵を表す構造体
typedef struct enemy {
  int health;     // 体力
  int money;      // 倒した時に得られる報酬

  enemy(int health = UNDEFINED, int money = UNDEFINED){
    this->health = health;
    this->money  = money;
  }
} ENEMY;


// ゲーム・ボード
int g_board[MAX_N][MAX_N];

class PathDefense{
  public:

    // Initialize method
    int init(vector<string> board, int money, int creepHealth, int creepMoney, vector<int> towerType){
      fprintf(stderr,"init =>\n");
      return 0;
    }

    vector<int> placeTowers(vector<int> creep, int money, vector<int> baseHealth){
      vector<int> ret;

      return ret;
    }
};

int main(){
  int n, money, creepHealth, creepMoney;
  int nt;
  string row;
  vector<string> board;

  cin >> n;
  cin >> money;

  for(int y = 0; y < n; y++){
    cin >> row;
    board.push_back(row);
  }

  cin >> creepHealth;
  cin >> creepMoney;

  cin >> nt;
  vector<int> towerType(nt);

  for(int i = 0; i < nt; i++){
    cin >> towerType[i];
  }

  PathDefense pd;

  pd.init(board, money, creepHealth, creepMoney, towerType);
  int nc, b;

  for(int turn = 0; turn < 2000; turn++){
    cin >> money;
    cin >> nc;

    vector<int> creep(nc);

    for(int creepId = 0; creepId < nc; creepId++){
      cin >> creep[creepId];
    }

    cin >> b;
    vector<int> baseHealth(b);

    for(int baseId = 0; baseId < b; baseId++){
      cin >> baseHealth[baseId];
    }

    vector<int> ret = pd.placeTowers(creep, money, baseHealth);

    cout << ret.size() << endl;
    for(int i = 0; i < ret.size(); i++){
      cout << ret[i] << endl;
    }
  }

  return 0;
}

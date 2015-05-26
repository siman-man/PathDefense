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
const int MAX_B = 10;     // 基地の最大数(実際は8が最大)
  
/*
 * Cellの種別を作成
 *  0: 番兵(GUARD)
 *  1: 経路(PATH)
 *  2: 基地(BASE)
 *  3: 平地(PLAIN)
 */
enum CellType{
  GUARD,
  PATH,
  BASE,
  PLAIN
};

// 敵を表す構造体
typedef struct enemy {
  int id;         // ID
  int health;     // 体力
  int y;          // y座標
  int x;          // x座標

  // 初期化
  enemy(int health = UNDEFINED, int y = UNDEFINED, int x = UNDEFINED){
    this->health = health;
    this->y      = y;
    this->x      = x;
  }
} CREEP;

// 基地を表す構造体
typedef struct base {
  int id;         // ID
  int health;     // 体力
  int y;          // y座標
  int x;          // x座標
} BASE;

// 文字を数値に変える関数
int chat2int(char ch){
  return ch - '0';
}

// ボード
int g_board[MAX_N][MAX_N];

// 敵を倒すと得られる報酬
int g_money;

// 敵の初期体力
int g_creepHealth;

// ボードの横幅
int g_boardWidth;

// ボードの縦幅
int g_boardHeight;

// 基地のリスト

class PathDefense{
  public:

    // Initialize method
    int init(vector<string> board, int money, int creepHealth, int creepMoney, vector<int> towerType){
      fprintf(stderr,"init =>\n");
      // ボードを全て番兵で初期化
      memset(g_board, GUARD, sizeof(g_board));

      // ボードの縦幅を取得
      g_boardHeight = board.size();

      // ボードの横幅を取得
      g_boardWidth  = board[0].size();

      // ボードの初期化
      for(int y = 0; y <= g_boardHeight; y++){
        for(int x = 0; x <= g_boardWidth; x++){
          // y = 0 or x = or or y = g_boardHeight or x = g_boardWidth
          // の場合は番兵を設置
          if(y == 0 || x == 0 || y == g_boardWidth || x == g_boardWidth){
            g_board[y][x] = GUARD;
          // '#'は平地
          }else if(board[y][x] == '#'){
            g_board[y][x] = PLAIN;
          // '.'は経路
          }else if(board[y][x] == '.'){
            g_board[y][x] = PATH;
          // それ以外は基地
          }else{
            g_board[y][x] = BASE;
          }
        }
      }

      // 報酬の初期化
      g_money = money; 

      // 敵の初期体力の初期化
      g_creepHealth = creepHealth;

      // ボードの初期化
      
      return 0;
    }

    /*
     * 敵を作成する
     *   id: creepのID 
     *    y: 敵のy座標
     *    x: 敵のx座標
     *
     * 返り値
     *   CREEP
     */
    CREEP createCreep(int id, int y, int x){
      // 敵のインスタンスを作成

      // 体力を初期化
      // もし500ターンを超えていた場合は2倍する
      

      return CREEP();
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

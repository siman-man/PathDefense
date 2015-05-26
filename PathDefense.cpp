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

const int UNDEFINED   = -1;     // 値が未定義
const int MAX_N       = 60 + 2; // ボードの最大長(番兵込み)
const int MAX_Z       = 2015;   // 敵の最大数(実際は2000が最大)
const int MAX_B       = 10;     // 基地の最大数(実際は8が最大)
const int MAX_T       = 25;     // タワーの最大数(実際は20が最大)
const int BASE_HEALTH = 1000;   // 基地の初期体力(1000固定)
  
/*
 * Cellの種別を作成
 *  0: 番兵(GUARD)
 *  1: 経路(PATH)
 *  2: 拠点(BASE_POINT)
 *  3: 平地(PLAIN)
 */
enum CellType{
  GUARD,
  PATH,
  BASE_POINT,
  PLAIN
};

// 敵を表す構造体
typedef struct creep {
  int id;             // ID
  int health;         // 体力
  int y;              // y座標
  int x;              // x座標
  int created_at;     // 出現時のターン数
  int disappeared_at; // 消失時のターン数

  // 初期化
  creep(int id = UNDEFINED, int health = UNDEFINED, int y = UNDEFINED, int x = UNDEFINED){
    this->id          = id;
    this->health      = health;
    this->y           = y;
    this->x           = x;
    this->created_at  = UNDEFINED;
  }
} CREEP;

// 基地を表す構造体
typedef struct base {
  int id;         // ID
  int health;     // 体力
  int y;          // y座標
  int x;          // x座標

  // 初期化
  base(int id = UNDEFINED, int y = UNDEFINED, int x = UNDEFINED){
    this->id     = id;
    this->health = BASE_HEALTH;
    this->y      = y;
    this->x      = x;
  }
} BASE;

// タワーを表す構造体
typedef struct tower {
  int id;       // ID
  int range;    // 射程距離
  int damage;   // 攻撃力
  int cost;     // 建設コスト

  tower(int id = UNDEFINED, int range = UNDEFINED, int damage = UNDEFINED, int cost = UNDEFINED){
    this->id      = id;
    this->range   = range;
    this->damage  = damage;
    this->cost    = cost;
  }
} TOWER;

// 文字を数値に変える関数
int char2int(char ch){
  return ch - '0';
}

// 現在のターン
int g_currentTurn;

// ボード
int g_board[MAX_N][MAX_N];

// 現在の所持金
int g_currentAmountMoney;

// 敵を倒すと貰える報酬
int g_reward;

// 敵の初期体力
int g_creepHealth;

// ボードの横幅
int g_boardWidth;

// ボードの縦幅
int g_boardHeight;

// 基地のリスト
BASE g_baseList[MAX_B];

// 敵のリスト
CREEP g_creepList[MAX_Z];

// タワーのリスト
TOWER g_towerList[MAX_T];

class PathDefense{
  public:

    /*
     * 初期化関数
     *         board: ボード情報
     *         money: 初期所持金
     *   creepHealth: 敵の初期体力(500ターン後は2倍)
     *    creepMoney: 敵を倒した時に得られるお金
     */
    int init(vector<string> board, int money, int creepHealth, int creepMoney, vector<int> towerType){
      fprintf(stderr,"init =>\n");
      // ボードを全て番兵で初期化
      memset(g_board, GUARD, sizeof(g_board));

      // ターンを初期化
      g_currentTurn = 0;

      initBoardData(board);

      // 初期の所持金
      g_currentAmountMoney = money;

      // 報酬の初期化
      g_reward = creepMoney;

      // 敵の初期体力の初期化
      g_creepHealth = creepHealth;
      
      return 0;
    }

    /*
     * ボードの初期化
     *   board: 初期ボード
     */
    void initBoardData(vector<string> &board){
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
            // 基地のIDを取得
            int baseId = char2int(g_board[y][x]);
            g_board[y][x] = BASE_POINT;

            // 基地を作成
            BASE base = createBase(baseId, y, x);

            // 基地リストに入れる
            g_baseList[baseId] = base;
          }
        }
      }
    }

    /*
     * 敵を作成する
     *      id: creepのID 
     *  health: 体力
     *       y: 敵のy座標
     *       x: 敵のx座標
     *
     * 返り値
     *   CREEP
     */
    CREEP createCreep(int id, int health, int y, int x){
      // 敵のインスタンスを作成
      CREEP creep(id, health, y, x);

      // もし500ターンを超えていた場合は2倍する
      if(g_currentTurn >= 500){
        creep.health *= 2;
      }
      
      // 現在のターン時に出現したことを記録
      creep.created_at = g_currentTurn;

      return creep;
    }

    /*
     * 基地を作成する
     *   id: baseのID
     *    y: 基地のy座標
     *    x: 基地のx座標
     *
     * 返り値
     *   BASE
     */
    BASE createBase(int baseId, int y, int x){
      // 基地のインスタンスを作成
      BASE base(baseId, y, x);

      return base;
    }

    /*
     * 指定したIDの基地を取得する
     *   baseId: 基地ID
     */
    BASE* getBase(int baseId){
      return &g_baseList[baseId];
    }

    /*
     * 指定したIDの敵を取得する
     *   creepId: 敵ID
     */
    CREEP* getCreep(int creepId){
      return &g_creepList[creepId];
    }

    /*
     * ボードの状態を更新する
     *   - 所持金の更新
     *   - 敵情報の更新
     *   - 基地情報の更新
     */
    void updateBoardData(vector<int> &creeps, int money, vector<int> &baseHealth){
      // 現在の所持金の更新
      g_currentAmountMoney = money;

      // 敵情報の更新
      updateCreepsData(creeps);

      // 基地情報の更新
      updateBasesData(baseHealth);
    }

    /*
     * 敵情報の更新
     *   creeps: 敵の情報リスト
     */
    void updateCreepsData(vector<int> &creeps){
      // 敵の数
      int creepCount = creeps.size() / 4;

      // 各敵情報を更新する
      for(int i = 0; i < creepCount; i++){
        int creepId = creeps[i*4];    // 敵IDの取得
        int health  = creeps[i*4+1];  // 体力
        int x       = creeps[i*4+2];  // x座標
        int y       = creeps[i*4+3];  // y座標

        CREEP *creep = getCreep(creepId);

        // 各値を更新
        creep->health = health;
        creep->y      = y;
        creep->x      = x;
      }
    }

    /*
     * 基地情報の更新を行う
     *   baseHealth: 基地の体力情報のリスト
     */
    void updateBasesData(vector<int> &baseHealth){
      // 基地の数
      int baseCount = baseHealth.size();

      // 各基地の体力を更新
      for(int baseId = 0; baseId < baseCount; baseId++){
        BASE *base = getBase(baseId);

        base->health = baseHealth[baseId];
      }
    }

    vector<int> placeTowers(vector<int> creeps, int money, vector<int> baseHealth){
      vector<int> ret;

      // ゲーム情報の更新
      updateBoardData(creeps, money, baseHealth);

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

    vector<int> creeps(nc);

    for(int creepId = 0; creepId < nc; creepId++){
      cin >> creeps[creepId];
    }

    cin >> b;
    vector<int> baseHealth(b);

    for(int baseId = 0; baseId < b; baseId++){
      cin >> baseHealth[baseId];
    }

    vector<int> ret = pd.placeTowers(creeps, money, baseHealth);

    cout << ret.size() << endl;
    for(int i = 0; i < ret.size(); i++){
      cout << ret[i] << endl;
    }
  }

  return 0;
}

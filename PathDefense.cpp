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

const int UNDEFINED        = -1;     // 値が未定義
const int MAX_N            = 60 + 2; // ボードの最大長(番兵込み)
const int MAX_Z            = 2015;   // 敵の最大数(実際は2000が最大)
const int MAX_B            = 10;     // 基地の最大数(実際は8が最大)
const int MAX_T            = 25;     // タワーの最大数(実際は20が最大)
const int BASE_INIT_HEALTH = 1000;   // 基地の初期体力(1000固定)
const int LIMIT_TURN       = 2000;   // ターンの上限

/*
 * それぞれの方角と数値の対応
 *
 *         0
 *         ↑
 *     3 ←   → 1
 *         ↓
 *         2
 */
const int DY[4] = { -1, 0, 1, 0 };
const int DX[4] = {  0, 1, 0, -1};
  
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

/*
 * 座標を表す構造体
 */
typedef struct coord {
  int y;      // y座標
  int x;      // x座標
  int dist;   // 幅優先で使用する

  coord(int y = UNDEFINED, int x = UNDEFINED, int dist = UNDEFINED){
    this->y = y;
    this->x = x;
    this->dist = dist;
  }
} COORD;

// 敵を表す構造体
typedef struct creep {
  int id;             // ID
  int health;         // 体力
  int y;              // y座標
  int x;              // x座標
  int created_at;     // 出現時のターン数
  int disappeared_at; // 消失時のターン数
  int target;         // 狙っている基地

  // 初期化
  creep(int id = UNDEFINED, int health = UNDEFINED, int y = UNDEFINED, int x = UNDEFINED){
    this->id          = id;
    this->health      = health;
    this->y           = y;
    this->x           = x;
    this->created_at  = UNDEFINED;
    this->target      = UNDEFINED;
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
    this->health = BASE_INIT_HEALTH;
    this->y      = y;
    this->x      = x;
  }
} BASE;

// タワーを表す構造体
typedef struct tower {
  int id;       // ID
  int y;        // y座標
  int x;        // x座標
  int range;    // 射程距離
  int damage;   // 攻撃力
  int cost;     // 建設コスト

  tower(int id = UNDEFINED, int range = UNDEFINED, int damage = UNDEFINED, int cost = UNDEFINED){
    this->id      = id;
    this->range   = range;
    this->damage  = damage;
    this->cost    = cost;

    // 位置情報は建設時に設定
    this->y       = UNDEFINED;
    this->x       = UNDEFINED;
  }
} TOWER;

// マップの各要素を表す構造体
typedef struct cell {
  int type;     // Cellのタイプ
  int damage;   // この場所で与えられる最大ダメージ

  cell(int type = UNDEFINED){
    this->type = type;
  }
} CELL;

// 文字を数値に変える関数
int char2int(char ch){
  return ch - '0';
}


/*
 * 2点間の大雑把な距離を計算
 *   fromY: 出発地点のy座標
 *   fromX: 出発地点のx座標
 *   destY: 目標地点のy座標
 *   destX: 目標地点のx座標
 */
int calcRoughDist(int fromY, int fromX, int destY, int destX){
  return (fromY-destY) * (fromY-destY) + (fromX-destX) * (fromX-destX);
}

// 現在のターン
int g_currentTurn;

// ボード
CELL g_board[MAX_N][MAX_N];

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

// 生存中の敵のIDリスト
set<int> g_aliveCreepIdList;

// タワーのリスト
TOWER g_towerList[MAX_T];

// 建設済みのタワーリスト
vector<TOWER> g_buildedTowerList;

// 出現ポイントのリスト
vector<COORD> g_spawnPointList;

// 建設したタワーの数
int g_buildedTowerCount;

class PathDefense{
  public:

    /*
     * 初期化関数
     *         board: ボード情報
     *         money: 初期所持金
     *   creepHealth: 敵の初期体力(500ターン毎に倍々で増える)
     *    creepMoney: 敵を倒した時に得られるお金
     */
    int init(vector<string> board, int money, int creepHealth, int creepMoney, vector<int> towerType){
      fprintf(stderr,"init =>\n");

      // ターンを初期化を行う
      g_currentTurn = 0;

      // ボードの初期化を行う
      initBoardData(board);

      // タワーの初期化を行う
      initTowerData(towerType);

      // 初期の所持金
      g_currentAmountMoney = money;

      // 建設したタワーの数を初期化
      g_buildedTowerCount = 0;

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
      fprintf(stderr,"initBoardData =>\n");
      // ボードの縦幅を取得
      g_boardHeight = board.size();

      // ボードの横幅を取得
      g_boardWidth  = board[0].size();

      // ボードの初期化
      for(int y = 0; y < g_boardHeight; y++){
        for(int x = 0; x < g_boardWidth; x++){
          CELL cell = createCell();

          // '#'は平地
          if(board[y][x] == '#'){
            cell.type = PLAIN;

          // '.'は経路
          }else if(board[y][x] == '.'){
            cell.type = PATH;
            // マップの端であればスポーン地点の追加を行う
            if(y == 0 || x == 0 || y == g_boardHeight-1 || x == g_boardWidth-1){
              COORD coord(y, x);
              g_spawnPointList.push_back(coord);
            }

          // それ以外は基地
          }else{
            // 基地のIDを取得
            int baseId = char2int(board[y][x]);
            cell.type = BASE_POINT;

            // 基地を作成
            BASE base = createBase(baseId, y, x);

            // 基地リストに入れる
            g_baseList[baseId] = base;
          }

          // セルを代入
          g_board[y][x] = cell;
        }
      }
    }

    /*
     * タワー情報の初期化を行う
     *   towerType: タワーの情報が格納されている
     */
    void initTowerData(vector<int> &towerType){
      // タワーの種類の数
      int towerCount = towerType.size() / 3;

      fprintf(stderr,"towerCount = %d\n", towerCount);

      for(int towerId = 0; towerId < towerCount; towerId++){
        int range  = towerType[towerId*3];
        int damage = towerType[towerId*3+1];
        int cost   = towerType[towerId*3+2];

        TOWER tower = createTower(towerId, range, damage, cost);

        g_towerList[towerId] = tower;

        showTowerData(towerId);
      }
    }

    /*
     * [not yet]
     *   各出現ポイントから基地までの最短経路を計算
     */
    void initSpawnToBaseShortPath(){
    }

    /*
     * セルの作成
     *   type: セルのタイプ
     */
    CELL createCell(){
      CELL cell; 

      return cell;
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

      // もし500ターン毎に体力が倍々に増える
      creep.health *= 1 * (g_currentTurn/500);
      
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
     * タワーの作成を行う(初期化時のみ使用)
     *   towerId: タワーのID
     *     range: 攻撃範囲
     *    damage: 攻撃力
     *      cost: 建設コスト
     */
    TOWER createTower(int towerId, int range, int damage, int cost){
      TOWER tower(towerId, range, damage, cost);

      return tower;
    }

    /*
     * タワー情報の表示
     *   towerId: タワーID
     */
    void showTowerData(int towerId){
      TOWER tower = getTower(towerId);

      double value = tower.range * tower.damage / (double)tower.cost;
      fprintf(stderr,"towerId = %d, range = %d, damage = %d, cost = %d, value = %4.2f\n", 
          towerId, tower.range, tower.damage, tower.cost, value);
    }

    /*
     * タワーの建設を行う(ゲーム中に使用)
     *   - 入力
     *   towerId: 建設するタワーの種類
     *         y: 建設を行うy座標
     *         x: 建設を行うx座標
     */
    void buildTower(int towerId, int y, int x){
      TOWER tower = getTower(towerId);
      tower.y = y;
      tower.x = x;

      // 建設したタワーリストに追加
      g_buildedTowerList.push_back(tower);
    }

    /*
     * 指定したIDの基地を取得する
     *   - 入力
     *     baseId: 基地ID
     *   - 返り値
     *     基地情報のポインタ
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
     * 指定したIDのタワー情報を取得
     *   towerId: タワーID
     */
    TOWER getTower(int towerId){
      return g_towerList[towerId];
    }

    /*
     * 指定した座標のセル情報を取得する
     *   - 入力
     *     y: Y座標
     *     x: x座標
     *   - 返り値
     *     セル情報のぽいんた　
     */
    CELL* getCell(int y, int x){
      return &g_board[y][x];
    }

    /*
     * マップの中にいるかどうかをチェック
     *   - 入力
     *     y: Y座標
     *     x: X座標
     *   - 返り値
     *     true: マップ内
     *    false: マップ外
     */
    bool isInsideMap(int y, int x){
      return (y >= 0 && x >= 0 && y < g_boardHeight && x < g_boardWidth); 
    }

    /*
     * 画面外に出ていないかをチェック
     *   - 入力
     *     y: Y座標
     *     x: X座標
     *   - 返り値
     *     マップから出ているかどうかを返す
     */
    bool isOutsideMap(int y, int x){
      return (y < 0 || x < 0 || y >= g_boardHeight || x >= g_boardWidth);
    }

    /*
     * タワーが建設可能かどうかを調べる
     *   - 入力
     *     y: Y座標
     *     x: X座標
     *   - 返り値
     *     建設可能かどうかを返す
     */
    bool canBuildTower(int y, int x){
      CELL *cell = getCell(y, x);

      return (isOutsideMap(y, x) && cell->type == PLAIN);
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

        // もしcreated_atが設定されていない場合は新しくcreepを作成する
        if(creep->created_at == UNDEFINED){
          CREEP newCreep = createCreep(creepId, health, y, x);

          g_creepList[creepId] = newCreep;
        }else{
          // そうでない場合は各値を更新
          creep->health = health;
          creep->y      = y;
          creep->x      = x;
        }
      }
    }

    /*
     * 基地情報の更新を行う
     *   - 入力
     *     baseHealth: 基地の体力情報のリスト
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

    /*
     * ある地点から生存中の敵で一番近い敵のIDを返す
     *   - 入力
     *    fromY: 出発地点のy座標
     *    fromX: 出発地点のx座標
     *   - 返り値
     *    mostNearCreepId: 一番近い敵のID
     */
    int searchMostNearCreepId(int fromY, int fromX){
      int mostNearCreepId = UNDEFINED;
      int roughDist;
      int minDist = INT_MAX;

      set<int>::iterator it = g_aliveCreepIdList.begin();

      // 生存中の敵をそれぞれ処理
      while(it != g_aliveCreepIdList.end()){
        int creepId = (*it);
        CREEP *creep = getCreep(creepId);

        // 敵との距離を計算
        roughDist = calcRoughDist(fromY, fromX, creep->y, creep->x);

        if(minDist > roughDist){
          minDist = roughDist;
          mostNearCreepId = creepId;
        }

        it++;
      }

      return mostNearCreepId;
    }

    /*
     * [not yet]
     * ある敵が特定の基地まで、指定した体力を残しながら到達できるかどうかを調べる
     *  - 引数
     *   creepId: 敵ID
     *    baseId: 基地ID
     * 
     *  - 返り値
     *   true: 到達可能
     *  false: 到達不可
     */
    bool canReachBasePoint(int creepId, int baseId){
      return true;
    }

    /* 
     *   ある地点からどれだけの経路をカバーできるかを計算
     *   - 入力
     *     fromY: 出発地点のY座標
     *     fromX: 出発地点のX座標
     *     range: 攻撃範囲
     *   - 返り値
     *     カバーしている経路の数
     */
    int calcCoverdCellCount(int fromY, int fromX, int range){
      int coverdCellCount = 0;

      queue<COORD> que;
      que.push(COORD(fromY, fromX, 0));
      map<int, bool> checkList;

      // 座標情報が無くなるまで繰り返す
      while(!que.empty()){
        COORD coord = que.front(); que.pop();

        // 調査済みの座標であれば処理を飛ばす
        if(checkList[coord.y*g_boardHeight+coord.x]) continue;
        checkList[coord.y*g_boardHeight+coord.x] = true;

        // もし距離が攻撃範囲であれば処理を続ける
        if(calcRoughDist(fromY, fromX, coord.y, coord.x) <= range * range){
          CELL *cell = getCell(coord.y, coord.x);

          // もしセルの種別が平地であればカバーする範囲を増やす
          if(cell->type == PLAIN){
            coverdCellCount += 1;
          }

          // 上下左右のセルを追加
          for(int i = 0; i < 4; i++){
            int ny = coord.y + DY[i];
            int nx = coord.x + DX[i];

            // もし画面外で無い場合はキューに追加
            if(isInsideMap(ny, nx)){
              que.push(COORD(ny, nx));
            }
          }
        }
      }

      return coverdCellCount;
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

  for(int turn = 0; turn < LIMIT_TURN; turn++){
    fprintf(stderr,"turn = %d\n", turn);
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

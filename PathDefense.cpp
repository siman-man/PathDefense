/**
 * @file    PathDefense.cpp
 * @brief   PathDefenseを解く用の何か
 * @author  siman
 * @date    2015/05/30
 *
 * @detail
 * 関数名の横のコメントのタイプ
 *  [not yet]: まだ実装中
 *    [maybe]: 実装はしたけどバグあるかも or 追加の機能あるかも
 * [complete]: 完璧です
 *
 * 細かい戦略についてはREADME.mdに残すつもり
 */
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
#include <cassert>
#include <cmath>
#include <stack>
#include <queue>

using namespace std;

typedef long long ll;

const int UNDEFINED        = -1;     //! 値が未定義
const int UNLOCK           = -1;     //! 敵をロックしていない状態
const int NOT_FOUND        = -1;     //! 敵を見つけたか見つけてないかの判定
const int MAX_N            = 60 + 2; //! ボードの最大長(番兵込み)
const int MAX_Z            = 2015;   //! 敵の最大数(実際は2000が最大)
const int MAX_B            = 10;     //! 基地の最大数(実際は8が最大)
const int MAX_T            = 25;     //! タワーの最大数(実際は20が最大)
const int MAX_R            = 5;      //! 攻撃範囲の最大値
const int BASE_INIT_HEALTH = 1000;   //! 基地の初期体力(1000固定)
const int LIMIT_TURN       = 2000;   //! ターンの上限

/*
 * それぞれの方角と数値の対応
 *
 *         2
 *         ↑
 *     1 ←   → 3
 *         ↓
 *          0
 */
const int DY[4] = { 1, 0, -1, 0 };
const int DX[4] = { 0, -1, 0, 1 };

/**
 * @brief 移動できる方向のマスク
 */
const int DOWN  = 1; //! 0001
const int LEFT  = 2; //! 0010
const int UP    = 4; //! 0100
const int RIGHT = 8; //! 1000

const int directMask[4] = {DOWN, LEFT, UP, RIGHT};
  
/**
 * @enum Enum 
 * Cellの種別を作成
 */
enum CellType{
  //! 番兵(GUARD)
  GUARD,

  //! 経路(PATH)
  PATH,

  //! 拠点(BASE_POINT)
  BASE_POINT,

  //! 平地(PLAIN)
  PLAIN,

  //! スポーン地点(SPAWN_POINT)
  SPAWN_POINT
};

/**
 * @enum Enum
 * 敵の状態を作成
 */
enum CreepState {
  //! 生きている(ALIVE)
  ALIVE,

  //! 倒された(DEAD)
  DEAD,

  //! 倒される予定(MAYBE_DEAD)
  MAYBE_DEAD,

  //! 倒せない(NON_STOP)
  NON_STOP
};

/**
 * @enum Enum
 * 基地の状態を表す
 */
enum BaseState {
  //! 安全な状態(SAFETY)
  SAFETY,

  //! 警告(WARNING)
  WARNING,

  //! 危険(DANGER)
  DANGER,

  //! 破壊された
  BROKEN
};

/**
 * @enum Enum
 * タワーの状態を表す
 */
enum TowerState {
};

/**
 * @brief 座標を表す構造体
 */
typedef struct coord {
  int y;      // Y座標
  int x;      // X座標
  int dist;   // 幅優先で使用する

  coord(int y = UNDEFINED, int x = UNDEFINED, int dist = UNDEFINED){
    this->y = y;
    this->x = x;
    this->dist = dist;
  }
} COORD;

/**
 * @brief 建設情報を表す構造体
 */
typedef struct buildInfo {
  int towerId;  // タワーID
  int y;        // Y座標
  int x;        // X座標

  buildInfo(int towerId = UNDEFINED, int y = UNDEFINED, int x = UNDEFINED){
    this->towerId = towerId;
    this->y       = y;
    this->x       = x;
  }
} BUILD_INFO;

/**
 * @brief スポーン地点を表す構造体
 */
typedef struct spawn {
  int id;                 // ID
  int y;                  // Y座標
  int x;                  // X座標
  set<int> targetBases;    // 狙っている基地のリスト(候補が1つとは限らない)
  int popUpCreepCount;    // 出現した敵の数

  spawn(int id = UNDEFINED, int y = UNDEFINED, int x = UNDEFINED){
    this->id = id;
    this->y  = y;
    this->x  = x;
    this->popUpCreepCount = 0;
  }
} SPAWN;

/**
 * @fn [complete]
 * 2点間の大雑把な距離を計算
 * @param (fromY) 出発地点のy座標
 * @param (fromX) 出発地点のx座標
 * @param (destY) 目標地点のy座標
 * @param (destX) 目標地点のx座標
 *
 * @return sqrtで元に戻す前の距離
 * @detail どちらの座標が近い/遠いを判断するだけならsqrtで元に戻さなくても比較出来る。
 */
int calcRoughDist(int fromY, int fromX, int destY, int destX){
  return (fromY-destY) * (fromY-destY) + (fromX-destX) * (fromX-destX);
}

/**
 * @brief 敵を表す構造体
 */
typedef struct creep {
  int id;             // ID
  int health;         // 体力
  int y;              // y座標
  int x;              // x座標
  int prevY;          // 前のターンに居たY座標
  int prevX;          // 前のターンに居たX座標
  int created_at;     // 出現時のターン数
  int disappeared_at; // 消失時のターン数
  int targetBases;    // 狙っている基地候補(1つとは限らない)
  CreepState state;   // 敵の状態

  // 初期化
  creep(int id = UNDEFINED, int health = UNDEFINED, int y = UNDEFINED, int x = UNDEFINED){
    this->id          = id;
    this->health      = health;
    this->y           = y;
    this->x           = x;
    this->prevY       = UNDEFINED;
    this->prevX       = UNDEFINED;
    this->created_at  = UNDEFINED;
    this->state       = ALIVE;
  }

  /**
   * @fn [complete]
   * 倒されたかどうかの判定
   *
   * @return 倒されたかどうかの判定値
   */
  bool isDead(){
    return health <= 0;
  }
} CREEP;

/*
 * 基地を表す構造体
 */
typedef struct base {
  int id;           // ID
  int health;       // 体力
  int y;            // y座標
  int x;            // x座標
  BaseState state;  // 基地の状態

  // 初期化
  base(int id = UNDEFINED, int y = UNDEFINED, int x = UNDEFINED){
    this->id     = id;
    this->health = BASE_INIT_HEALTH;
    this->y      = y;
    this->x      = x;
    this->state  = SAFETY;
  }

  /**
   * @fn [complete]
   * 破壊されたかどうかの確認を行う
   *
   * @return 破壊されたかどうかの判定値
   */
  bool isBroken(){
    return health == 0;
  }

  /**
   * @fn [complete]
   * 破壊されていないかどうかの確認を行う
   *
   * @return 破壊されていないかどうかの判定値
   */
  bool isNotBroken(){
    return !isBroken();
  }
} BASE;

/* 
 * @brief タワーを表す構造体
 */
typedef struct tower {
  int id;             // ID
  int type;           // タワーの種類
  int y;              // y座標
  int x;              // x座標
  int range;          // 射程距離
  int damage;         // 攻撃力
  int cost;           // 建設コスト
  int lockedCreepId;  // ロックしている敵のID

  tower(int type = UNDEFINED, int range = UNDEFINED, int damage = UNDEFINED, int cost = UNDEFINED){
    this->id      = id;
    this->type    = type;
    this->range   = range;
    this->damage  = damage;
    this->cost    = cost;

    // 位置情報は建設時に設定
    this->y       = UNDEFINED;
    this->x       = UNDEFINED;
  }

  /**
   * @fn
   * 情報をリセット
   *
   * @detail
   * - ロックしている敵の情報をリセット
   */
  void reset(){
    lockedCreepId = NOT_FOUND;
  }

  /**
   * @fn
   * ロックオンしているかどうかを確認
   *
   * @return ロックしているかどうかの判定値
   */
  bool isLocked(){
    return lockedCreepId != NOT_FOUND;
  }

  /**
   * @fn [not yet]
   * 指定した座標が攻撃範囲に含まれているかどうかを判定
   * @param (y) Y座標
   * @param (x) X座標
   *
   * @return 範囲内かどうかを返す
   */
  bool isInsideAttackRange(int destY, int destX){
    int roughDist = calcRoughDist(y, x, destY, destX);
    return roughDist <= range * range;
  }
} TOWER;

/*
 * @brief マップの各要素を表す構造体
 */
typedef struct cell {
  int type;                   //! Cellのタイプ
  int y;                      //! Y座標
  int x;                      //! X座標
  int damage;                 //! この場所で与えられる最大ダメージ
  int coveredCount[MAX_R+1];  //! ここにタワーを立てることでカバーできる経路の数
  int baseId;                 //! 基地がある場合はそのID
  int spawnId;                //! スポーン地点の場合はID
  int basicValue;             //! 基礎点
  int defenseValue;           //! セルの防御価値(値が高い程守る優先度が高い)
  set<int> basePaths;         //! どの基地の経路になっているかを調べる

  cell(int y = UNDEFINED, int x = UNDEFINED, int type = UNDEFINED){
    this->y             = y;
    this->x             = x;
    this->type          = type;
    this->damage        = 0;
    this->baseId        = UNDEFINED;
    this->spawnId       = UNDEFINED;
    this->basicValue    = 0;
    this->defenseValue  = 0;
  }

  /**
   * @fn [complete]
   * 平地かどうかを返す
   * @return 平地かどうかの判定値
   */
  bool isPlain(){
    return type == PLAIN;
  }

  /**
   * @fn [complete]
   * 平地でないかどうかを返す
   * @return 平地でないかどうかの判定値
   */
  bool isNotPlain(){
    return !isPlain();
  }

  /**
   * @fn [complete]
   * 基地かどうかを返す
   * @return 基地かどうかの判定値
   */
  bool isBasePoint(){
    return type == BASE_POINT;
  }

  /**
   * @fn [complete]
   * 基地でないかどうかを返す
   * @return 基地でないかどうかの判定値
   */
  bool isNotBasePoint(){
    return !isBasePoint();
  }

  /**
   * @fn [complete]
   * 経路かどうかを返す
   * @return 経路かどうかの判定値
   */
  bool isPath(){
    return type == PATH;
  }

  /**
   * @fn [complete]
   * 進めるCellかどうかを調べる
   * @return 進めるCellかどうかの判定値
   */
  bool canMove(){
    return type != PLAIN;
  }

} CELL;

/**
 * @fn [complete]
 * 文字を数値に変える関数
 * @param (ch) 変換したい文字
 *
 * @return 変換された数値
 */
int char2int(char ch){
  return ch - '0';
}

/**
 * @fn [complete]
 * @return 乱数
 */
unsigned long long xor128(){
  static unsigned long long rx=123456789, ry=362436069, rz=521288629, rw=88675123;
  unsigned long long rt = (rx ^ (rx<<11));
  rx=ry; ry=rz; rz=rw;
  return (rw=(rw^(rw>>19))^(rt^(rt>>8)));
}


//! 現在のターン
int g_currentTurn;

//! ボード
CELL g_board[MAX_N][MAX_N];

//! 現在の所持金の合計
int g_currentAmountMoney;

//! 敵を倒すと貰える報酬
int g_reward;

//! 敵の初期体力
int g_creepHealth;

//! 基地の総数
int g_baseCount;

//! タワーの総数
int g_towerCount;

//! ボードの横幅
int g_boardWidth;

//! ボードの縦幅
int g_boardHeight;

//! 今までに出現した敵の総数
int g_totalCreepCount;

/** 
 * @brief 最短路を得るためのマップ
 * [y][x][baseId] - その基地に向かうための方向がわかる
 *
 * @detail
 * 数値のフラグで管理
 * - 0000の場合はどこにも進めない
 * - 1111は全方向進める
 */
int g_shortestPathMap[MAX_N][MAX_N][MAX_B+1];

//! 基地のリスト
BASE g_baseList[MAX_B];

//! 敵のリスト
CREEP g_creepList[MAX_Z];

//! 生存中の敵のIDリスト
set<int> g_aliveCreepsIdList;

//! タワーのリスト
TOWER g_towerList[MAX_T];

//! 建設済みのタワーリスト
vector<TOWER> g_buildedTowerList;

//! 建設済みのタワーの数
int g_buildedTowerCount;

//! スポーン地点のリスト
vector<SPAWN> g_spawnList;

//! 前に行動したstepを覚える配列
int g_prevStep[MAX_N][MAX_N];

/**
 * @fn [complete]
 * (y,x)を1次元に直した場合の値を出す
 * @param (y) Y座標
 * @param (x) X座標
 *
 * @return 1次元座標表現時の値
 */
inline int calcZ(int y, int x){
  return y * g_boardHeight + x;
}

/**
 * @fn [complete]
 * 2点間のマンハッタン距離を返す
 * @param (fromY) 起点のY座標
 * @param (fromX) 起点のX座標
 * @param (destY) 目的地のY座標
 * @param (destX) 目的地のX座標
 *
 * @return 2点間のマンハッタン距離
 */
int calcManhattanDist(int fromY, int fromX, int destY, int destX){
  return abs(fromY-destY) + abs(fromX-destX);
}

class PathDefense{
  public:
    vector<int> m_buildTowerData;

    /**
     * @fn
     * 初期化関数
     * @param (board)       ボード情報
     * @param (money)       初期所持金
     * @param (creepHealth) 敵の初期体力(500ターン毎に倍々で増える)
     * @param (creepMoney)  敵を倒した時に得られるお金
     *
     * @detail
     * ゲームを始めるにあたって必要な情報を初期化しておく
     */
    int init(vector<string> board, int money, int creepHealth, int creepMoney, vector<int> towerType){
      fprintf(stderr,"init =>\n");

      // ターンを初期化を行う
      g_currentTurn = 0;

      // 出現した敵の総数を初期化
      g_totalCreepCount = 0;

      // 最短路マップの初期化
      memset(g_shortestPathMap, UNDEFINED, sizeof(g_shortestPathMap));

      // ボードの初期化を行う
      initBoardData(board);

      // タワーの初期化を行う
      initTowerData(towerType);

      // カバーできる経路の数を計算
      initCoveredCellCount();

      // スポーン地点から基地までの最短路を計算
      initSpawnToBaseShortestPath();

      // 初期の所持金
      g_currentAmountMoney = money;

      // 建設したタワーの数を初期化
      g_buildedTowerCount = 0;

      // 基地の数を初期化
      g_baseCount = 0;

      // 報酬の初期化
      g_reward = creepMoney;

      // 敵の初期体力の初期化
      g_creepHealth = creepHealth;

      // ゲーム情報の表示
      showGameData();
      
      return 0;
    }

    /**
     * @fn [maybe]
     * ゲームの基礎情報を表示
     *   - 敵の基礎体力
     *   - 敵を倒した時の報酬
     */
    void showGameData(){
      fprintf(stderr,"-----------------------------------------------\n");
      fprintf(stderr,"BoardSize = Y: %d, X: %d\n", g_boardHeight, g_boardWidth);
      fprintf(stderr,"creepHealth = %d\n", g_creepHealth);
      fprintf(stderr,"reward = %d\n", g_reward);
      fprintf(stderr,"-----------------------------------------------\n");
    }

    /**
     * @fn [maybe]
     * ボードの初期化
     * @param (board) 初期ボード
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
          CELL cell = createCell(y, x);

          // '#'は平地
          if(board[y][x] == '#'){
            cell.type = PLAIN;

          // '.'は経路
          }else if(board[y][x] == '.'){
            cell.type = PATH;
            // マップの端であればスポーン地点の追加を行う
            if(isEdgeOfMap(y,x)){
              int spawnId = g_spawnList.size();
              // cellのタイプをスポーン地点で上書き
              cell.type = SPAWN_POINT;
              cell.spawnId = spawnId;

              // スポーン地点の追加
              addSpawnPoint(spawnId, y, x);
            }

          // それ以外は基地
          }else{
            //! 基地のIDを取得
            int baseId = char2int(board[y][x]);
            cell.type = BASE_POINT;
            cell.baseId = baseId;

            // 基地を作成
            BASE base = createBase(baseId, y, x);

            // 基地の数を更新
            g_baseCount += 1;

            // 基地リストに入れる
            g_baseList[baseId] = base;
          }

          // セルを代入
          g_board[y][x] = cell;
        }
      }
    }

    /**
     * @fn [maybe]
     * カバー出来る経路の数を初期化
     */
    void initCoveredCellCount(){
      // マップ全体を更新
      for(int y = 0; y < g_boardHeight; y++){
        for(int x = 0; x < g_boardWidth; x++){
          CELL *cell = getCell(y,x);

          // 基地が建設出来ない場所は飛ばす
          if(cell->type != PLAIN) continue;

          // 攻撃範囲1-5までを処理
          for(int range = 1; range <= MAX_R; range++){
            int coveredCount = calcCoveredCellCount(y, x, range);
            cell->coveredCount[range] = coveredCount;
          }
        }
      }
    }

    /**
     * @fn [maybe]
     * タワー情報の初期化を行う
     * @param (towerType) タワーの情報が格納されているリスト
     */
    void initTowerData(vector<int> &towerType){
      // タワーの種類の数
      g_towerCount = towerType.size() / 3;

      fprintf(stderr,"towerCount = %d\n", g_towerCount);

      for(int towerId = 0; towerId < g_towerCount; towerId++){
        int range  = towerType[towerId*3];
        int damage = towerType[towerId*3+1];
        int cost   = towerType[towerId*3+2];

        // タワーの作成
        TOWER tower = createTower(towerId, range, damage, cost);

        // タワーの追加
        g_towerList[towerId] = tower;

        showTowerData(towerId);
      }
    }

    /**
     * @fn [complete]
     * マップの端っこかどうかを調べる
     * @param (y) Y座標
     * @param (x) X座標
     *
     * @return マップの端かどうかの判定値
     */
    bool isEdgeOfMap(int y, int x){
      return (y == 0 || x == 0 || y == g_boardHeight-1 || x == g_boardWidth-1);
    }

    /**
     * @fn [complete]
     * マップの端までの距離を計算
     * @param (y) Y座標
     * @param (x) X座標
     *
     * @return マップの端までの距離
     */
    int calcDistanceToEdge(int y, int x){
      return min(min(y, g_boardHeight-y-1), min(x, g_boardWidth-1));
    }

    /**
     * @fn [not yet]
     * タワーを建設するべきかどうかを調べる
     * 
     * @return 建設するか否かの判定値
     * @detail 
     * なるべく後出しでタワーを建設したいのでギリギリまで建てないようにする
     */
    bool toBuildTower(){
      return true;
    }

    /**
     * @fn [maybe]
     * どこにタワーを立てるのが良いかを調べてその座標を返す
     *
     * @return 建設する場所とタワーID
     */
    BUILD_INFO searchBestBuildPoint(){
      int bestValue = INT_MIN;
      int bestY;
      int bestX;
      int bestId;

      // マップ全体に対して処理を行う
      for(int y = 0; y < g_boardHeight; y++){
        for(int x = 0; x < g_boardWidth; x++){
          // セル情報を取得
          CELL *cell = getCell(y,x);

          // 平地以外は処理を飛ばす
          if(!cell->isPlain()) continue;


          // 全てのタワーで処理を行う
          for(int towerId = 0; towerId < g_towerCount; towerId++){
            TOWER *tower = getTower(towerId);
            int coveredCount = calcCoveredCellCount(y, x, tower->range);
            int value = coveredCount;

            // 評価値が更新されたら建設情報を更新
            if(bestValue < value){
              bestValue = value;
              bestId = towerId;
              bestY = y;
              bestX = x;
            }
          }
        }
      }

      return BUILD_INFO(bestId, bestY, bestX);
    }

    /**
     * @fn [not yet]
     * いずれかの敵が基地に到達出来るかどうかを確認
     *
     * @sa canReachBasePoint
     * @return どこかの基地に到達できるかどうかの判定値
     * @detail
     * 現在のタワーの建設状態から何もしなくても敵を倒せるのかどうかを調べる
     */
    bool isAnyCreepReachableBase(){
      set<int>::iterator it = g_aliveCreepsIdList.begin();
      // 全ての敵に対して処理する
      while(it != g_aliveCreepsIdList.end()){
        int creepId = (*it);
        it++;
      }
    }     

    /**
     * @fn [maybe]
     *   各出現ポイントから基地までの最短経路を計算
     *
     * @detail
     * 最短経路を計算しておき、敵が出現した際に狙われる基地をリストアップ出来るように
     * しておく。ここでの最短経路は「マンハッタン距離」より長くならない経路を指す
     */
    void initSpawnToBaseShortestPath(){
      int spawnCount = g_spawnList.size();

      // スポーン地点毎に処理を行う
      for(int spawnId = 0; spawnId < spawnCount; spawnId++){
        calcSpawnToBaseShortestPath(spawnId);
      }
    }

    /**
     * @fn [maybe]
     * 出現ポイントから基地までの最短経路を出す
     * @param (spawnY) スポーン地点のY座標
     * @param (spawnX) スポーン地点のX座標
     * @sa initSpawnToBaseShortestPath
     *
     * @detail 
     * 最短距離は幅優先探索で出す
     */
    void calcSpawnToBaseShortestPath(int spawnId){
      map<int, bool> checkList;
      SPAWN *spawn = getSpawn(spawnId);
      queue<COORD> que;
      que.push(COORD(spawn->y, spawn->x, 0));

      // 前回行動した情報のリセット
      memset(g_prevStep, UNDEFINED, sizeof(g_prevStep));

      while(!que.empty()){
        // 座標情報の取得
        COORD coord = que.front(); que.pop();

        // セル情報を取得
        CELL *cell = getCell(coord.y, coord.x);

        // 基地に辿り着いてそれがマンハッタン距離と同等の場合は経路を復元して登録を行う
        if(cell->isBasePoint() && coord.dist <= calcManhattanDist(spawn->y, spawn->x, coord.y, coord.x)){
          int baseId = cell->baseId;
          // 最短経路の登録
          registShortestPath(spawnId, baseId);
          // スポーン地点の候補基地リストに追加
          spawn->targetBases.insert(baseId);
        }else{
          for(int direct = 0; direct < 4; direct++){
            int ny = coord.y + DY[direct];
            int nx = coord.x + DX[direct];
            int nz = calcZ(ny, nx);

            // 行動出来るセルであれば進む
            // が、もしチェックしたセルであれば処理を飛ばす
            if(canMoveCell(ny, nx) && !checkList[nz]){
              checkList[nz] = true;
              que.push(COORD(ny, nx, coord.dist+1));
              g_prevStep[ny][nx] = direct;
            }
          }
        }
      }
    }

    /**
     * @fn [maybe]
     * 最短経路の登録を行う
     * @param (spawnId) スポーン地点のID
     * @param (baseId)  基地のID
     *
     * @detail
     * マップに「このセルからこの基地へはこの方角が最短ですよ」情報を書き込む
     * 各Cellに「この基地への最短路の経路になってます」情報を書き込む
     */
    void registShortestPath(int spawnId, int baseId){
       BASE  *base = getBase(baseId);
      SPAWN *spawn = getSpawn(spawnId);
      int y = base->y;
      int x = base->x;

      fprintf(stderr,"regist shortest path: %d -> %d\n", spawnId, baseId);

      // 逆算して最短路の登録を行う
      while(y != spawn->y || x != spawn->x){
        int prev = g_prevStep[y][x];
        assert(prev != -1);
        y += DY[(prev+2)%4];
        x += DX[(prev+2)%4];

        CELL *cell = getCell(y,x);
        g_shortestPathMap[y][x][baseId] |= directMask[prev];
        cell->basePaths.insert(baseId);
      }
    }

    /**
     * @fn [maybe]
     * Cellの作成を行う
     * @param (y) Y座標
     * @param (x) X座標
     *
     * @return Cell情報
     */
    CELL createCell(int y, int x){
      CELL cell(y, x); 

      return cell;
    }

    /**
     * @fn [maybe]
     * 敵の作成を行う
     * @param (creepId) creepのID 
     * @param (health)  体力
     * @param (y)       敵のY座標
     * @param (x)       敵のX座標
     *
     * @return 敵情報
     * @detail
     * 敵の作成を行い、敵の総数のカウントを1増やす
     */
    CREEP createCreep(int creepId, int health, int y, int x){
      // セル情報の取得
      CELL *cell = getCell(y,x);

      // スポーン地点から出現していないとおかしい
      assert(cell->spawnId != UNDEFINED);

      SPAWN *spawn = getSpawn(cell->spawnId);

      // 敵のインスタンスを作成
      CREEP creep(creepId, health, y, x);

      // もし500ターン毎に体力が倍々に増える
      creep.health *= (1 << (g_currentTurn/500));
      
      // 現在のターン時に出現したことを記録
      creep.created_at = g_currentTurn;

      // スポーン地点から出現した敵の数を更新する
      spawn->popUpCreepCount += 1;

      // 全体の出現数を更新する
      g_totalCreepCount += 1;

      return creep;
    }

    /**
     * @fn [maybe]
     * 基地を作成する
     * @param (baseId) baseのID
     * @param (y)      基地のY座標
     * @param (x)      基地のX座標
     *
     * @return 基地情報
     */
    BASE createBase(int baseId, int y, int x){
      // 基地のインスタンスを作成
      BASE base(baseId, y, x);

      return base;
    }

    /**
     * @fn [maybe]
     * タワーの作成を行う(初期化時のみ使用)
     * @param (towerId) タワーのID
     * @param (range)   攻撃範囲
     * @param (damage)  攻撃力
     * @param (cost)    建設コスト
     *
     * @return タワーの情報
     */
    TOWER createTower(int towerId, int range, int damage, int cost){
      TOWER tower(towerId, range, damage, cost);

      return tower;
    }

    /**
     * @fn [maybe]
     * スポーン地点の追加を行う
     * @param (spawnId) スポーンID
     * @param (y)       Y座標
     * @param (x)       X座標
     * 
     * @detail
     * 出現ポイントの追加
     */
    void addSpawnPoint(int spawnId, int y, int x){
      SPAWN spawn(spawnId, y, x);
      g_spawnList.push_back(spawn);

      fprintf(stderr,"Add spwan %d point: y = %d, x = %d\n", spawnId, y, x);
    }

    /**
     * @fn [maybe]
     * タワー情報の表示
     * @param (towerType) タワーの種別
     */
    void showTowerData(int towerType){
      TOWER *tower = getTower(towerType);

      double value = tower->range * (tower->damage*tower->damage) / (double)tower->cost/4.0;
      fprintf(stderr,"towerId = %d, range = %d, damage = %d, cost = %d, value = %4.2f\n", 
          towerType, tower->range, tower->damage, tower->cost, value);
    }

    /**
     * @fn [maybe]
     * タワーの建設を行う(ゲーム中に使用)
     * @param (towerId) 建設するタワーの種類
     * @param (y)       建設を行うY座標
     * @param (x)       建設を行うX座標
     * 
     * @detail 建設情報もここで追加を行う
     */
    void buildTower(int towerType, int y, int x){
      TOWER tower = buyTower(towerType);
      tower.id  = g_buildedTowerCount;
      tower.y   = y;
      tower.x   = x;

      // 建設したタワーリストに追加
      g_buildedTowerList.push_back(tower);

      // 建設したタワーの数を更新
      g_buildedTowerCount += 1;

      // セルの「攻撃ダメージ」を更新
      updateCellDamageData(tower.id);

      // 建設情報の追加
      m_buildTowerData.push_back(tower.x);
      m_buildTowerData.push_back(tower.y);
      m_buildTowerData.push_back(tower.type);
    }

    /**
     * @fn [maybe]
     * 与えられるダメージの更新
     * @param (towerId) タワーID
     *
     * @detail
     * 基地を建てた時にマップ上のCellに「この場所でのダメージ」情報を更新する
     */
    void updateCellDamageData(int towerId){
      queue<COORD> que;
      map<int, bool> checkList;
      TOWER *tower = getTower(towerId);
      que.push(COORD(tower->y, tower->x, 0));

      while(!que.empty()){
        COORD coord = que.front(); que.pop();

        CELL *cell = getCell(coord.y, coord.x);
        
        // 経路であれば攻撃力を更新
        if(cell->isPath()){
          cell->damage += tower->damage;
        }

        for(int direct = 0; direct < 4; direct++){
          int ny = coord.y + DY[direct];
          int nx = coord.x + DX[direct];
          int z = calcZ(ny, nx);

          // マップ内であり、未探索で、タワーの攻撃範囲内であれば追加
          if(isInsideMap(ny, nx) && !checkList[z] && tower->isInsideAttackRange(ny, nx)){
            checkList[z] = true;
            que.push(COORD(ny, nx, coord.dist+1));
          }
        }
      }
    }

    /**
     * @fn [complete]
     * 指定したIDの基地を取得する
     * @param (baseId) 基地ID
     *
     * @return 基地情報のポインタ
     */
    BASE* getBase(int baseId){
      return &g_baseList[baseId];
    }

    /**
     * @fn [complete]
     * 指定したIDの敵を取得する
     * @param (creepId) 敵ID
     *
     * @return 敵情報のポインタ
     */
    CREEP* getCreep(int creepId){
      return &g_creepList[creepId];
    }

    /**
     * @fn
     * タワーを購入する
     * @param (towerId) タワーID
     * @return タワー情報
     */
    TOWER buyTower(int towerType){
      TOWER tower = g_towerList[towerType];

      // 建設コストより所持金が少ない状態でタワーは購入出来ない
      assert(tower.cost <= g_currentAmountMoney);
      // 所持金を減らす
      g_currentAmountMoney -= tower.cost;

      return tower;
    }

    /**
     * @fn [complete]
     * 指定したIDのタワー情報を取得
     * @param (towerId) タワーID
     *
     * @return タワー情報のポインタ
     */
    TOWER* getTower(int towerId){
      return &g_buildedTowerList[towerId];
    }

    /**
     * @fn [not yet]
     * 指定した種別のタワー情報を取得
     * @param (towerType) タワーの種別
     *
     * @return タワー情報
     */
    TOWER* referTower(int towerType){
      return &g_towerList[towerType];
    }

    /**
     * @fn [complete]
     * スポーン地点の取得
     * @param (spawnId) スポーンID
     * 
     * @return スポーン地点の情報を指すポインタ
     */
    SPAWN* getSpawn(int spawnId){
      return &g_spawnList[spawnId];
    }

    /**
     * @fn [complete]
     * 指定した座標のセル情報を取得する
     * @param (y) Y座標
     * @param (x) X座標
     *
     * @return セル情報のポインタ
     */
    CELL* getCell(int y, int x){
      return &g_board[y][x];
    }

    /**
     * @fn [complete]
     * マップの内側にいるかどうかをチェック
     * @param (y) Y座標
     * @param (x) X座標
     *
     * @return マップの内側にいるかどうかの判定値
     */
    inline bool isInsideMap(int y, int x){
      return (y >= 0 && x >= 0 && y < g_boardHeight && x < g_boardWidth); 
    }

    /**
     * @fn [complete]
     * 指定した座標に移動出来るかどうかを判定
     *
     * @param (y) Y座標
     * @param (x) X座標
     * 
     * @return 進めるかどうかの判定値
     * @detail
     * マップ内であり、かつ壁ではない場合は進める
     */
    bool canMoveCell(int y, int x){
      if(!isInsideMap(y, x)) return false;

      CELL *cell = getCell(y,x);

      return cell->canMove();
    }

    /**
     * @fn [complete]
     * 画面外に出ていないかをチェック
     * @param (y) Y座標
     * @param (x) X座標
     *
     * @return マップから出ているかどうかを返す
     */
    inline bool isOutsideMap(int y, int x){
      return (y < 0 || x < 0 || y >= g_boardHeight || x >= g_boardWidth);
    }

    /**
     * @fn [complete]
     * タワーが建設可能かどうかを調べる
     * @param (towerType) 建設したいタワーの種別
     * @param (y)         Y座標
     * @param (x)         X座標
     *
     * @return 建設可能かどうかの判定値
     */
    bool canBuildTower(int towerType, int y, int x){
      CELL *cell = getCell(y, x);
      TOWER *tower = getTower(towerType);

      // マップ内であり、平地であり、所持金が足りている場合は建設可能
      return (isInsideMap(y, x) && cell->isPlain() && tower->cost <= g_currentAmountMoney);
    }

    /**
     * @fn
     * ボードの状態を更新する
     * @param (creeps)     現在マップ上に存在する敵の情報リスト
     * @param (money)      現在の所持金
     * @param (baseHealth) 基地の体力リスト
     *
     * @detail
     *   - 所持金の更新
     *   - 防御価値のリセット
     *   - 敵情報の更新
     *   - 基地情報の更新
     */
    void updateBoardData(vector<int> &creeps, int money, vector<int> &baseHealth){
      // 現在の所持金の更新
      g_currentAmountMoney = money;

      // タワー情報のリセット
      resetTowerData();

      // 各Cellの防御価値をリセット
      resetCellDefenseValue();

      // 敵情報の更新
      updateCreepsData(creeps);

      // 仮想的に敵を行動させる
      moveCreeps();

      // 各タワー情報の更新
      updateTowerData();

      // タワー攻撃
      attackTowers();

      // 基地情報の更新
      updateBasesData(baseHealth);
    }

    /**
     * @fn [maybe]
     * Cellの防御価値をリセット
     *
     * @detail
     * 毎ターン防御価値は変化するので初期化を行っておく、基礎点は変えない
     */
    void resetCellDefenseValue(){
      // 全てのCellに対して処理を行う
      for(int y = 0; y < g_boardHeight; y++){
        for(int x = 0; x < g_boardWidth; x++){
          CELL *cell = getCell(y,x);

          // 平地はリセットしない(する意味が無い)
          if(cell->isPlain()) continue;

          // 防御価値を0に初期化する
          cell->defenseValue = 0;
        }
      }
    }

    /**
     * @fn
     * Towerの情報をリセット
     *
     * @detail
     * ロックしている敵を解除
     */
    void resetTowerData(){
      // 全てのタワーに対して処理を行う
      for(int towerId = 0; towerId < g_buildedTowerCount; towerId++){
        TOWER *tower = getTower(towerId);

        // ロック情報を解除
        tower->lockedCreepId = UNLOCK;
      }
    }

    /**
     * @fn
     * 敵情報の更新
     * @param (creeps) 敵の情報リスト
     *
     * @detail
     * 敵の生存リストを更新
     */
    void updateCreepsData(vector<int> &creeps){
      //! 敵の数
      int creepCount = creeps.size() / 4;

      // 生存中の敵リストをリセット
      g_aliveCreepsIdList.clear();

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
          creep->prevY  = creep->y;
          creep->prevX  = creep->x;
          creep->y      = y;
          creep->x      = x;
        }

        // 生存中の敵リストに追加
        g_aliveCreepsIdList.insert(creepId);
      }
    }

    /**
     * @fn [maybe]
     * 仮想的に敵を行動させる
     *
     * @detail
     * ボードのシミュレート予測を向上させる
     */
    void moveCreeps(){
      set<int>::iterator it = g_aliveCreepsIdList.begin();
      // トライする回数
      int tryLimit = 10;
      int ny, nx;

      // 生存中の全ての敵が行動する
      while(it != g_aliveCreepsIdList.end()){
        int tryCount = 0;
        int creepId = (*it);
        CREEP *creep = getCreep(creepId);

        do {
          tryCount += 1;
          int direct = xor128() % 4;
          ny = creep->y + DY[direct];
          nx = creep->x + DX[direct];

          if(tryCount > tryLimit) break;
        }while(!canMoveCell(ny, nx) || (ny != creep->prevY && nx != creep->prevX));

        it++;
      }
    }

    /**
     * @fn
     * 敵に対して攻撃を行う
     */
    void attack(int creepId, int damage){
      CREEP *creep = getCreep(creepId);
      
      // 敵に攻撃
      creep->health -= damage;

      // もしHPが0以下の場合は倒した
      if(creep->isDead()){
        creep->state = DEAD;
      }
    }

    /**
     * @fn
     * タワーが敵に対して攻撃
     */
    void attackTowers(){
      for(int towerId = 0; towerId < g_buildedTowerCount; towerId++){
        TOWER *tower = getTower(towerId);

        // 敵をロックしていた場合攻撃
        if(tower->isLocked()){
          attack(tower->lockedCreepId, tower->damage);
        }
      }
    }

    /**
     * @fn [maybe]
     * タワー情報の更新を行う
     * 
     * @detail
     * 主に次にロックする敵を決める
     */
    void updateTowerData(){
      for(int towerId = 0; towerId < g_buildedTowerCount; towerId++){
        TOWER *tower = getTower(towerId);

        int creepId = searchMostNearCreepId(tower->y, tower->x);

        // 敵が見つかった場合はそれをロック
        if(creepId != NOT_FOUND){
          fprintf(stderr,"tower locked %d\n", creepId);
          tower->lockedCreepId = creepId;
        }
      }
    }

    /**
     * @fn [maybe]
     * 基地情報の更新を行う
     * @param (baseHealth) 基地の体力情報のリスト
     *
     * @detail
     * 基地情報の更新、体力が0になった基地は状態を「BROKEN」に変更
     */
    void updateBasesData(vector<int> &baseHealth){

      // 各基地の体力を更新
      for(int baseId = 0; baseId < g_baseCount; baseId++){
        BASE *base = getBase(baseId);

        // 体力を更新する
        base->health = baseHealth[baseId];

        updateBaseState(base);
      }
    }

    /**
     * @fn [maybe]
     * 基地の状態の更新を行う
     * @param (base) 基地の情報ポインタ
     *
     * @detail
     *   - 体力が0の基地は「破壊状態」に移行
     */
    void updateBaseState(BASE *base){
      // もし体力が0になっていた場合、状態を「破壊された(BROKEN)」に変更
      if(base->isBroken()){
        base->state = BROKEN;
      }
    }

    /**
     * @fn [maybe]
     * セルの防御価値の基礎点を算出する(この値が高いほど守る価値がある)
     *
     * @detail
     *   - 基地の周りのセルの防御価値は高い
     */
    void initCellBasicValue(){
      // 各基地に対して処理を行う
      for(int baseId = 0; baseId < g_baseCount; baseId++){
        BASE *base = getBase(baseId);
        CELL *cell = getCell(base->y, base->x);

        // マップの端までの距離が一定ラインを以下の場合は価値を下げる
        if(calcDistanceToEdge(base->y, base->x) <= 5){
          cell->basicValue -= 1;
        }
      }
    }

    /**
     * @fn [not yet]
     * タワーが倒された時に守る必要が無くなった経路を探す
     * @param (baseId) 倒された基地ID
     *
     * @detail
     * 倒されたタワーによって経路が永久に塞がれているものを探す
     */
    void updateDeadPathInfo(int baseId){
      BASE *base = getBase(baseId);
      // 破壊されていない基地が来るのはおかしい
      assert(base->isBroken());
    }

    /**
     * @fn [not yet]
     * セルの防御価値を計算する。防御価値の高いセルは優先的に守る
     * @param (y) Y座標
     * @param (x) X座標
     *
     * @detail
     */
    int updateCellDefenseValue(int y, int x){
      CELL *cell = getCell(y, x);

      return cell->basicValue;
    }

    /**
     * @fn [maybe]
     * 敵の予測経路のポイントを高くする
     * @param (creepId) 敵のID
     * 
     * @detail
     * 敵が行動する経路を計算して、その部分の防御優先度を高くする
     * 進行方向の値だけを伸ばす
     */
    void setCreepMovePathValue(int creepId){
      CREEP *creep = getCreep(creepId);
      CELL *cell = getCell(creep->y, creep->x);

      set<int>::iterator it = cell->basePaths.begin();

      while(it != cell->basePaths.end()){
        int baseId = (*it);
        putFootPrint(cell->y, cell->x, baseId);
        it++;
      }
    }

    /**
     * @fn [maybe]
     * 基地までの足跡をつける
     * @param (y)      Y座標
     * @param (x)      X座標
     * @param (baseId) 目的地の基地ID
     * @param (limit)  距離の限界
     *
     * @sa setCreepMovePathValue
     * @detail
     * 基地までの足跡をつける(そのまま評価値に反映)
     */
    void putFootPrint(int y, int x, int baseId, int limit = 5){
      BASE *base = getBase(baseId);
      map<int, bool> checkList;
      int dist = 0;

      while((base->y != y || base->x != x) || dist < limit){
        int direct = g_shortestPathMap[y][x][baseId];
        assert(direct != UNDEFINED);
        int ny = y + DY[direct];
        int nx = x + DX[direct];

        CELL *cell = getCell(ny, nx);
        cell->defenseValue += 1;

        dist += 1;
      }
    }

    /**
     * @fn [not yet]
     * 防御価値を設定(基地周辺)
     * @param (baseId) 基地ID
     *
     * @detail
     *   - 基地周辺の防御価値を高くする
     */
    void setBaseDefenseValue(int baseId){
      //! 基地周辺の距離
      int LIMIT = 5;
      BASE *base = getBase(baseId);
      map<int, bool> checkList;
      queue<COORD> que;
      que.push(COORD(base->y, base->x, 0));

      while(!que.empty()){
        COORD coord = que.front(); que.pop();
        int z = calcZ(coord.y, coord.x);

        // LIMITを越えた場合はスキップ
        if(coord.dist > LIMIT) continue;

        // 既に訪れていた場合はスキップ
        if(checkList[z]) continue;
        checkList[z] = true;

        CELL *cell = getCell(coord.y, coord.x);
        cell->defenseValue += 1;

        for(int direct = 0; direct < 4; direct++){
          int ny = coord.y + DY[direct];
          int nx = coord.x + DX[direct];

          if(isInsideMap(ny, nx)){
            que.push(COORD(ny, nx, coord.dist+1));
          }
        }
      }
    }

    /**
     * @fn [maybe]
     * ゲーム中、毎回呼ばれる関数
     * @param (creeps)     敵の情報リスト
     * @param (money)      現在の所持金
     * @param (baseHealth) 基地の体力情報
     *
     * @return タワーの建設情報
     */
    vector<int> placeTowers(vector<int> creeps, int money, vector<int> baseHealth){
      m_buildTowerData.clear();

      // ゲーム情報の更新
      updateBoardData(creeps, money, baseHealth);

      // ターンを1増やす
      g_currentTurn += 1;

      // タワーの建設情報を返して終わり
      return m_buildTowerData;
    }

    /**
     * @fn [maybe]
     * ある地点から生存中の敵で一番近い敵のIDを返す
     * @param (fromY) 出発地点のy座標
     * @param (fromX) 出発地点のx座標
     *
     * @return 一番近い敵のID
     * @detail
     * タワーが敵をロックするときに使う
     */
    int searchMostNearCreepId(int fromY, int fromX){
      int mostNearCreepId = NOT_FOUND;
      int roughDist;
      int minDist = INT_MAX;

      set<int>::iterator it = g_aliveCreepsIdList.begin();

      // 生存中の敵をそれぞれ処理
      while(it != g_aliveCreepsIdList.end()){
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

    /**
     * @fn [not yet]
     * ある敵が特定の基地まで、指定した体力を残しながら到達できるかどうかを調べる
     * @param (creepId) 敵ID
     * @param (baseId) 基地ID
     * 
     * @sa isAnyBaseReachable
     * @return 到達かどうかを示す判定値
     */
    bool canReachBasePoint(int creepId, int baseId){
      return true;
    }

    /**
     * @fn
     * ある地点からどれだけの経路をカバーできるかを計算
     * @param (fromY) 出発地点のY座標
     * @param (fromX) 出発地点のX座標
     * @param (range) 攻撃範囲
     *
     * @return カバーしている経路の数
     */
    int calcCoveredCellCount(int fromY, int fromX, int range){
      int coveredCellCount = 0;

      queue<COORD> que;
      que.push(COORD(fromY, fromX, 0));
      map<int, bool> checkList;

      // 座標情報が無くなるまで繰り返す
      while(!que.empty()){
        COORD coord = que.front(); que.pop();
        int z = calcZ(coord.y, coord.x);

        // 調査済みの座標であれば処理を飛ばす
        if(checkList[z]) continue;
        checkList[z] = true;

        // もし距離が攻撃範囲であれば処理を続ける
        if(calcRoughDist(fromY, fromX, coord.y, coord.x) <= range * range){
          CELL *cell = getCell(coord.y, coord.x);

          // もしセルの種別が平地であればカバーする範囲を増やす
          if(cell->type == PLAIN){
            coveredCellCount += 1;
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

      return coveredCellCount;
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
    //fprintf(stderr,"turn = %d\n", turn);
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

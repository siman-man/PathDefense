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
const int NOT_REACH        = -1;     //! 敵は基地に到達出来ない
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

  //! タワー(TOWER_POINT
  TOWER_POINT,

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

  //! 基地に到着(NON_STOP)
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
 * @brief 経路情報を表す構造体
 */
typedef struct route {
  int y;
  int x;
  int dist;
  vector<COORD> routes;

  route(int y = UNDEFINED, int x = UNDEFINED, int dist = UNDEFINED){
    this->y = y;
    this->x = x;
    this->dist = dist;
  }
} ROUTE;

/**
 * @brief 建設情報を表す構造体
 */
typedef struct buildInfo {
  int type;  // タワーの種別
  int y;          // Y座標
  int x;          // X座標

  buildInfo(int type = UNDEFINED, int y = UNDEFINED, int x = UNDEFINED){
    this->type = type;
    this->y    = y;
    this->x    = x;
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
  int originHealth;   // 元の体力
  int health;         // 体力
  int y;              // y座標
  int x;              // x座標
  int originY;        // ターンに居たY座標
  int originX;        // ターンに居たX座標
  int created_at;     // 出現時のターン数
  int disappeared_at; // 消失時のターン数
  int targetBase;     // 狙っている基地
  CreepState state;   // 敵の状態

  // 初期化
  creep(int id = UNDEFINED, int health = UNDEFINED, int y = UNDEFINED, int x = UNDEFINED){
    this->id           = id;
    this->originHealth = health;
    this->health       = health;
    this->y            = y;
    this->x            = x;
    this->originY      = UNDEFINED;
    this->originX      = UNDEFINED;
    this->created_at   = UNDEFINED;
    this->state        = ALIVE;
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
    return health <= 0;
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
  double value;       // タワーの価値

  tower(int type = UNDEFINED, int range = UNDEFINED, int damage = UNDEFINED, int cost = UNDEFINED){
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

  bool operator >(const tower &t) const{
    return value < t.value;
  } 
} TOWER;

/*
 * @brief マップの各要素を表す構造体
 */
typedef struct cell {
  int type;                     //! Cellのタイプ
  int y;                        //! Y座標
  int x;                        //! X座標
  int basicDamage;              //! 基礎攻撃力
  int damage;                   //! この場所で与えられる最大ダメージ
  int baseId;                   //! 基地がある場合はそのID
  int spawnId;                  //! スポーン地点の場合はID
  int basicValue;               //! 基礎点
  int aroundPathCount;          //! 周辺の経路の数
  int pathCount;                //! 最短路の経路となっている数
  int defenseValue;             //! セルの防御価値(値が高い程守る優先度が高い)
  set<int> basePaths;           //! どの基地の経路になっているかを調べる
  set<int> spawnPaths;          //! 出現ポイントからの経路になっている

  cell(int y = UNDEFINED, int x = UNDEFINED, int type = UNDEFINED){
    this->y               = y;
    this->x               = x;
    this->type            = type;
    this->basicDamage     = 0;
    this->damage          = 0;
    this->baseId          = UNDEFINED;
    this->spawnId         = UNDEFINED;
    this->aroundPathCount = 0;
    this->pathCount       = 0;
    this->basicValue      = 0;
    this->defenseValue    = 0;
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
   * @fn [not yet]
   * スポーン地点かどうかを返す
   * @return スポーン地点かどうかの判定値
   */
  bool isSpawnPoint(){
    return type == SPAWN_POINT;
  }

  /**
   * @fn [complete]
   * 経路かどうかを返す
   * @return 経路かどうかの判定値
   */
  bool isPath(){
    return type == PATH || type == SPAWN_POINT;
  }

	/**
	 * @fn [not yet]
	 * タワーかどうか
	 * @return タワーかどうかの判定値
	 */
	bool isTowerPoint(){
		return type == TOWER_POINT;
	}

  /**
   * @fn [complete]
   * 進めるCellかどうかを調べる
   * @return 進めるCellかどうかの判定値
   */
  bool canMove(){
    return (type != PLAIN);
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

//! 倍率
int g_healthRate;

//! 全滅フラグ
bool g_allBaseBroken;

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

//! 本当のタワーの総数
int g_realTowerCount;

//! タワー建設の最小費用
int g_towerMinCost;

//! ボードの横幅
int g_boardWidth;

//! 狙われやすい基地の目安
int g_targetedBasePoint[MAX_B];

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

//! 一時保存用
set<int> g_tempAliveCreepsIdList;

//! タワーのリスト
TOWER g_towerList[MAX_T];

//! ここにタワーを立てることでカバーできる経路の数
int coverPathCount[MAX_N][MAX_N][MAX_R+1];

//! 建設済みのタワーリスト
vector<TOWER> g_buildedTowerList;

//! 建設済みのタワーの数
int g_buildedTowerCount;

//! スポーン地点のリスト
vector<SPAWN> g_spawnList;

//! スポーン地点の数
int g_spawnCount;

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
    int init(vector<string> board, int money, int creepHealth, int creepMoney, vector<int> towerTypes){
      fprintf(stderr,"init =>\n");

      // ターンを初期化を行う
      g_currentTurn = 0;


      // 出現した敵の総数を初期化
      g_totalCreepCount = 0;

      // 基地の数を初期化
      g_baseCount = 0;

      // 最短路マップの初期化
      memset(g_shortestPathMap, UNDEFINED, sizeof(g_shortestPathMap));

			// 狙われ安さの初期化
			memset(g_targetedBasePoint, 0, sizeof(g_targetedBasePoint));

      // 敵の初期体力の初期化
      g_creepHealth = creepHealth;

      // ボードの初期化を行う
      initBoardData(board);

      // カバーできる経路の数を計算
      initCoverCellCount();

      // タワーの初期化を行う
      initTowerData(towerTypes);

      // スポーン地点から基地までの最短路を計算
      initCellToBaseShortestPath();

      // 初期の所持金
      g_currentAmountMoney = money;

      // 建設したタワーの数を初期化
      g_buildedTowerCount = 0;

      // 報酬の初期化
      g_reward = creepMoney;

      // セルの防御価値を初期化
      initCellBasicValue();

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
			fprintf(stderr,"Base count = %d\n", g_baseCount);
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

          assert(cell.damage == 0);
          // セルを代入
          g_board[y][x] = cell;
        }
      }
    }

    /**
     * @fn [maybe]
     * カバー出来る経路の数を初期化
     */
    void initCoverCellCount(){
      // マップ全体を更新
      for(int y = 0; y < g_boardHeight; y++){
        for(int x = 0; x < g_boardWidth; x++){
          CELL *cell = getCell(y,x);

          // 基地が建設出来ない場所は飛ばす
          if(cell->isNotPlain()) continue;

          // 攻撃範囲1-5までを処理
          for(int range = 1; range <= MAX_R; range++){
            int pathCount = calcCoverPathCount(y, x, range);
            coverPathCount[y][x][range] = pathCount;
          }
        }
      }
    }

    /**
     * @fn [maybe]
     * タワー情報の初期化を行う
     * @param (towerType) タワーの情報が格納されているリスト
     *
     * @detail
     * コストパフォーマンスの良いタワー上位5つを残す
     */
    void initTowerData(vector<int> &towerTypes){
      // タワーの種類の数
      g_towerCount = towerTypes.size() / 3;
			g_realTowerCount = g_towerCount;

      //fprintf(stderr,"towerCount = %d\n", g_towerCount);
      priority_queue<TOWER, vector<TOWER>, greater<TOWER> > pque;

      for(int towerType = 0; towerType < g_towerCount; towerType++){
        int range  = towerTypes[towerType*3];
        int damage = towerTypes[towerType*3+1];
        int cost   = towerTypes[towerType*3+2];

        // タワーの作成
        TOWER tower = createTower(towerType, range, damage, cost);

        // タワーの追加
        g_towerList[towerType] = tower;

        pque.push(tower);
      }

      g_towerCount = min(g_towerCount, 1);
      g_towerMinCost = INT_MAX;

      for(int id = 0; id < g_towerCount; id++){
        TOWER tower = pque.top(); pque.pop();
        showTowerData(tower.type);
        tower.id = id;
        g_towerList[id] = tower;
        g_towerMinCost = min(g_towerMinCost, tower.cost);
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
      int bestY = UNDEFINED;
      int bestX = UNDEFINED;
      int bestType = UNDEFINED;

      // マップ全体に対して処理を行う
      for(int y = 0; y < g_boardHeight; y++){
        for(int x = 0; x < g_boardWidth; x++){
          // セル情報を取得
          CELL *cell = getCell(y,x);

          // 平地以外は処理を飛ばす
          if(cell->isNotPlain()) continue;

          // 全てのタワーで処理を行う
          for(int towerType = 0; towerType < g_towerCount; towerType++){
            TOWER *tower = referTower(towerType);
            int value = calcBuildValue(y, x, tower->range, tower->damage);

            // 評価値が更新されたら建設情報を更新
            if(bestValue < value){
              bestValue = value;
              bestType = towerType;
              bestY = y;
              bestX = x;
            }
          }
        }
      }

			//assert(bestType != UNDEFINED);
      return BUILD_INFO(bestType, bestY, bestX);
    }

    /**
     * @fn [not yet]
     * いずれかの敵が基地に到達出来るかどうかを確認
     *
     * @return どこかの基地に到達できるかどうかの判定値
     * @detail
     * 現在のタワーの建設状態から何もしなくても敵を倒せるのかどうかを調べる
     */
    int isAnyCreepReachableBase(){
			int turn = 0;
      // 全ての敵に対して処理する
      while(!g_aliveCreepsIdList.empty()){
				if(g_currentTurn + turn >= 2000) break;
				if(turn >= g_boardWidth/2) break;
        // 敵の移動
        moveCreeps();

        // いずれかの敵が基地に到着していたらtrueを返す
        int baseId = isAnyCreepReachBase();
        if(baseId != NOT_REACH) return baseId;

        // 敵のロック
        updateTowerData();

        // 敵の状態を更新
        // updateCreepState();

				turn += 1;
      }

      return NOT_REACH;
    }     

    /**
     * @fn [maybe]
     *   各出現ポイントから基地までの最短経路を計算
     *
     * @detail
     * 最短経路を計算しておき、敵が出現した際に狙われる基地をリストアップ出来るように
     * しておく。ここでの最短経路は「マンハッタン距離」より長くならない経路を指す
     */
    void initCellToBaseShortestPath(){
      // 全てのセルを始点
      for(int y = 0; y < g_boardHeight; y++){
        for(int x = 0; x < g_boardWidth; x++){
          CELL *cell = getCell(y, x);

          if(cell->isPath()){
            calcToBaseShortestPath(y, x);
          }
          if(cell->isSpawnPoint()){
            calcSpawnToBaseShortestPath(cell->spawnId, y, x);
          }
        }
      }
    }

    /**
     * @fn [maybe]
     * 出現ポイントから基地までの最短経路を出す
     * @param (fromY) 開始地点のY座標
     * @param (fromX) 開始地点のX座標
     * @sa initCellToBaseShortestPath
     *
     * @detail 
     * 最短距離は幅優先探索で出す
     */
    void calcToBaseShortestPath(int fromY, int fromX){
      map<int, bool> checkList;
      queue<COORD> que;
      que.push(COORD(fromY, fromX, 0));

      // 前回行動した情報のリセット
      memset(g_prevStep, UNDEFINED, sizeof(g_prevStep));

      while(!que.empty()){
        // 座標情報の取得
        COORD coord = que.front(); que.pop();

        // セル情報を取得
        CELL *cell = getCell(coord.y, coord.x);

        assert(cell->isNotPlain());

        // 基地に辿り着いてそれがマンハッタン距離と同等の場合は経路を復元して登録を行う
        if(cell->isBasePoint() && coord.dist <= calcManhattanDist(fromY, fromX, coord.y, coord.x)){
          int baseId = cell->baseId;
          // 最短経路の登録
          registShortestPath(fromY, fromX, baseId, isEdgeOfMap(fromY, fromX));
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
     * 出現ポイントから基地までの最短経路を出す
     * @param (fromY) 開始地点のY座標
     * @param (fromX) 開始地点のX座標
     * @sa initCellToBaseShortestPath
     *
     * @detail 
     * 最短距離は幅優先探索で出す
     */
    void calcSpawnToBaseShortestPath(int spawnId, int fromY, int fromX){
      assert(spawnId != UNDEFINED);
      /*
			int checkList[MAX_N][MAX_N];
      memset(checkList, UNDEFINED, sizeof(checkList));
			/*/
      map<int, bool> checkList;
			//*/
      queue<ROUTE> que;
      ROUTE start(fromY, fromX, 0);
      que.push(start);

      while(!que.empty()){
        // 座標情報の取得
        ROUTE route = que.front(); que.pop();

        // セル情報を取得
        CELL *cell = getCell(route.y, route.x);

        assert(cell->isNotPlain());

        // 基地に辿り着いてそれがマンハッタン距離と同等の場合は経路を復元して登録を行う
        if(cell->isBasePoint() && route.dist <= calcManhattanDist(fromY, fromX, route.y, route.x)){
					g_targetedBasePoint[cell->baseId] += 1;
          // 最短経路の登録
          registPath(spawnId, route.routes);
        }else{
          for(int direct = 0; direct < 4; direct++){
            int ny = route.y + DY[direct];
            int nx = route.x + DX[direct];

            // 行動出来るセルであれば進む
            // が、もしチェックしたセルであれば処理を飛ばす
						//*
            int nz = calcZ(ny, nx);
            if(canMoveCell(ny, nx) && !checkList[nz]){
              checkList[nz] = true; 
						/*/
            if(canMoveCell(ny, nx) && (checkList[ny][nx] == UNDEFINED || (route.dist+1) <= checkList[ny][nx])){
              if(checkList[ny][nx] == UNDEFINED){
                checkList[ny][nx] = route.dist+1;
              }else{
                checkList[ny][nx] = min(checkList[ny][nx], route.dist+1);
              }
							//*/
              ROUTE next(ny, nx, route.dist+1);
              next.routes = route.routes;
              next.routes.push_back(COORD(ny, nx));
              que.push(next);
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
    void registShortestPath(int destY, int destX, int baseId, bool isSpawnPoint = false){
      BASE  *base = getBase(baseId);
      int y = base->y;
      int x = base->x;

      CELL *cell;

      // 逆算して最短路の登録を行う
      while(y != destY || x != destX){
        int prev = g_prevStep[y][x];
        assert(prev != UNDEFINED);
        y += DY[(prev+2)%4];
        x += DX[(prev+2)%4];

				assert(isInsideMap(y, x));
        cell = getCell(y,x);
        g_shortestPathMap[y][x][baseId] = prev;
        cell->basePaths.insert(baseId);

        if(isSpawnPoint){
          cell->pathCount += 1;
        }
      }
      cell = getCell(destY, destX);
      cell->basePaths.insert(baseId);
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
    void registPath(int spawnId, vector<COORD> routes){
      int size = routes.size();

      for(int i = 0; i < size; i++){
        COORD coord = routes[i];
        CELL *cell = getCell(coord.y, coord.x);
        cell->spawnPaths.insert(spawnId);
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

      //fprintf(stderr,"turn = %d, Creep %d spawn, y = %d, x = %d\n", g_currentTurn, creepId, y, x);

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
     * @param (towerType) タワーの種類
     * @param (range)     攻撃範囲
     * @param (damage)    攻撃力
     * @param (cost)      建設コスト
     *
     * @return タワーの情報
     */
    TOWER createTower(int towerType, int range, int damage, int cost){
      TOWER tower(towerType, range, damage, cost);
      double count = (8 * g_creepHealth)/damage;
			/*
      double value = (tower.range * (tower.damage)) / (double)tower.cost - count;
			/*/
      double value = (tower.range * (tower.damage)) / (double)tower.cost;
			//*/
			if(tower.range == 1) value -= 1.0;

      tower.value = value;

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

			g_spawnCount = g_spawnList.size();

      //fprintf(stderr,"Add spwan %d point: y = %d, x = %d\n", spawnId, y, x);
    }

    /**
     * @fn [maybe]
     * タワー情報の表示
     * @param (towerType) タワーの種別
     */
    void showTowerData(int towerType){
      TOWER *tower = referTower(towerType);

      fprintf(stderr,"towerId = %d, range = %d, damage = %d, cost = %d, value = %4.2f\n", 
          towerType, tower->range, tower->damage, tower->cost, tower->value);
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
      //fprintf(stderr,"buildTower type: %d\n", towerType);
      TOWER tower = buyTower(towerType);
      tower.id  = g_buildedTowerCount;
      tower.y   = y;
      tower.x   = x;
      CELL *cell = getCell(y, x);

      // 建設したタワーリストに追加
      g_buildedTowerList.push_back(tower);

      // 建設したタワーの数を更新
      g_buildedTowerCount += 1;

      // セルの「攻撃ダメージ」を更新
      updateCellDamageData(tower.id);

      // セルの種別を(TOWER_POINT)に変更
      cell->type = TOWER_POINT;

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
      //fprintf(stderr,"updateCellDamageData =>\n");
      queue<COORD> que;
      map<int, bool> checkList;
      TOWER *tower = getTower(towerId);
      que.push(COORD(tower->y, tower->x, 0));

      while(!que.empty()){
        COORD coord = que.front(); que.pop();

        CELL *cell = getCell(coord.y, coord.x);
        
        // 経路であれば攻撃力を更新
        if(cell->isPath()){
          cell->basicDamage += tower->damage;
          // 守りが堅くなったので守りの優先度は低くする
          cell->basicValue = max(cell->basicValue - cell->damage, 0);
          //cell->basicValue = max(cell->basicValue - tower->damage, 0);
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
     * @fn [complete]
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
      assert(isInsideMap(y, x));
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
      if(isOutsideMap(y, x)) return false;

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
			if(isOutsideMap(y, x)) return false;
      CELL *cell = getCell(y, x);
      TOWER *tower = referTower(towerType);

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

			// 倍率の更新
			g_healthRate = (1 << g_currentTurn/500);

      //fprintf(stderr,"turn = %d, g_currentAmountMoney = %d\n", g_currentTurn, g_currentAmountMoney);

      // タワー情報のリセット
      resetTowerData();

      // 各Cellの防御価値をリセット
      resetCellDefenseValue();

      // 敵情報の更新
      updateCreepsData(creeps);

      // 敵の狙う基地を決める
      setTargetBase();

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

          // セルの攻撃力を元に戻す
          cell->damage = cell->basicDamage;
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
     * 敵の生存リストを更新を行う
     */
    void updateCreepsData(vector<int> &creeps){
      //! 現在の敵の数
      int currentCreepCount = creeps.size() / 4;

      // 生存中の敵リストをリセット
      g_aliveCreepsIdList.clear();

      // 各敵情報を更新する
      for(int i = 0; i < currentCreepCount; i++){
        int creepId = creeps[i*4];    // 敵IDの取得
        int health  = creeps[i*4+1];  // 体力
        int x       = creeps[i*4+2];  // x座標
        int y       = creeps[i*4+3];  // y座標

        CREEP *creep = getCreep(creepId);

        // もしcreated_atが設定されていない場合は新しくcreepを作成する
        if(creep->created_at == UNDEFINED){
          CREEP newCreep = createCreep(creepId, health, y, x);
          newCreep.originHealth = health;
          newCreep.originY      = y;
          newCreep.originX      = x;

          g_creepList[creepId] = newCreep;

        // そうでない場合は各値を更新
        }else{
          creep->health       = health;
          creep->originHealth = health;
          creep->originY      = y;
          creep->originX      = x;
          creep->y            = y;
          creep->x            = x;
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
      //fprintf(stderr,"moveCreeps =>\n");
      set<int>::iterator it = g_aliveCreepsIdList.begin();

      // 生存中の全ての敵が行動する
      while(it != g_aliveCreepsIdList.end()){
        int creepId = (*it);
        CREEP *creep = getCreep(creepId);

        /*
        fprintf(stderr,"%d step (%d, %d) => (%d, %d)\n",
            creepId, creep->y, creep->x, creep->y + DY[direct], creep->x + DX[direct]);
            */

        int direct = g_shortestPathMap[creep->y][creep->x][creep->targetBase];
        assert(direct != UNDEFINED);
        creep->y += DY[direct];
        creep->x += DX[direct];

				assert(isInsideMap(creep->y, creep->x));
        CELL *cell = getCell(creep->y, creep->x);
        //cell->defenseValue += creep->health;
        cell->defenseValue += cell->pathCount * creep->health;

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
        g_aliveCreepsIdList.erase(creep->id);
      }
    }

    /**
     * @fn
     * タワーが敵に対して攻撃
     */
    void attackTowers(){
      //fprintf(stderr,"attackTowers =>\n");

      for(int towerId = 0; towerId < g_buildedTowerCount; towerId++){
        TOWER *tower = getTower(towerId);

        // 敵をロックしていた場合攻撃
        if(tower->isLocked()){
          //attack(tower->lockedCreepId, tower->damage);
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
      //fprintf(stderr,"updateTowerData =>\n");

      for(int towerId = 0; towerId < g_buildedTowerCount; towerId++){
        TOWER *tower = getTower(towerId);

        // ロック情報を解除
        tower->lockedCreepId = UNLOCK;

        int creepId = searchMostNearCreepId(tower);

        // 敵が見つかった場合はそれをロック
        if(creepId != NOT_FOUND){
          //fprintf(stderr,"tower locked %d\n", creepId);
          tower->lockedCreepId = creepId;
          attack(tower->lockedCreepId, tower->damage);
        }
      }
    }
    
    /**
     * 敵情報のリセット
     */
    void resetCreepData(){
      set<int>::iterator it = g_aliveCreepsIdList.begin();

      while(it != g_aliveCreepsIdList.end()){
        int creepId = (*it);
        CREEP *creep = getCreep(creepId);

        /*
        fprintf(stderr,"%d - %d reset: (%d, %d) => (%d, %d)\n", 
            g_currentTurn, creepId, creep->y, creep->x, creep->originY, creep->originX);
            */

        creep->health = creep->originHealth;
        creep->y      = creep->originY;
        creep->x      = creep->originX;


        it++;
      }
    }

    /**
     * 敵情報の更新
     */
    void updateCreepState(){
      set<int> tmp = g_aliveCreepsIdList;
      set<int>::iterator it = tmp.begin();

      while(it != tmp.end()){
        int creepId = (*it);
        CREEP *creep = getCreep(creepId);

        if(creep->state == NON_STOP){
          g_aliveCreepsIdList.erase(creepId);
        }

        it++;
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
      g_allBaseBroken = true;

      assert(g_baseCount == baseHealth.size());

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
        assert(base->health <= 0);
        base->state = BROKEN;
      }else{
        g_allBaseBroken = false;
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
        setBaseDefenseValue(baseId);
      }

      for(int y = 0; y < g_boardHeight; y++){
        for(int x = 0; x < g_boardWidth; x++){
          CELL *cell = getCell(y, x);

          cell->aroundPathCount = calcCrossPath(y, x);
        }
      }
    }
    
    /**
     * @fn
     * 十字路や三叉路の値を評価
     */
    int calcCrossPath(int y, int x){
      int pathCount = 0;

      for(int direct = 0; direct < 4; direct++){
        int ny = y + DY[direct];
        int nx = x + DX[direct];

        if(canMoveCell(ny, nx)){
          pathCount += 1;
        }
      }

      return pathCount;
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
			assert(isInsideMap(y, x));
      CELL *cell = getCell(y, x);

      return cell->basicValue;
    }

    /**
     * @fn [maybe]
		 * 防御の評価値を更新
     * @param (y)			始点のY座標
     * @param (x)			始点のX座標
     * @param (range) 範囲
		 * @param (value) 評価値
     */
    void updateDefenseValue(BASE *base, int range, int value = 1){
      map<int, bool> checkList;
      queue<COORD> que;
      que.push(COORD(base->y, base->x, 0));

      while(!que.empty()){
        COORD coord = que.front(); que.pop();

        if(coord.dist > range) continue;
        CELL *cell = getCell(coord.y, coord.x);

        if(cell->isPath()){
					//cell->defenseValue += g_targetedBasePoint[base->id];
        	if(cell->basePaths.find(base->id) != cell->basePaths.end()){
          	cell->defenseValue += value;
					}else{
          	cell->defenseValue += value;
					}
        }

        for(int direct = 0 ; direct < 4; direct++){
          int ny = coord.y + DY[direct];
          int nx = coord.x + DX[direct];
          int nz = calcZ(ny, nx);

          if(isInsideMap(ny, nx) & !checkList[nz]){
            que.push(COORD(ny, nx, coord.dist+1));
          }
        }
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
      int LIMIT = 3;
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

        if(cell->isPath()){
          cell->basicValue += g_creepHealth * 8;
          //cell->defenseValue += coord.dist;
        }

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
     * @fn
		 * 敵の目的地をランダムに設定する
     */
    void setTargetBase(){
      set<int>::iterator it = g_aliveCreepsIdList.begin();

      while(it != g_aliveCreepsIdList.end()){
        int creepId = (*it);
        CREEP *creep = getCreep(creepId);

        int targetId = selectTargetBase(creepId);
        creep->targetBase = targetId;

        it++;
      }
    }
		
		/**
		 * @fn [maybe]
		 * 敵の目的地をランダムに設定する
		 * @param (creepId) 敵のID
		 *
		 * @detail
		 * あとでシミュレーションするときに使用する
		 */
		int selectTargetBase(int creepId){
			CREEP *creep = getCreep(creepId);
			CELL *cell = getCell(creep->y, creep->x);
			set<int>::iterator it = cell->basePaths.begin();

			advance(it, xor128() % cell->basePaths.size());
      return (*it);
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

      g_tempAliveCreepsIdList = g_aliveCreepsIdList;

			// 全ての基地が破壊されたか、お金が無いときは何も行動しない
      if(!g_allBaseBroken && g_currentAmountMoney >= g_towerMinCost){
        for(int i = 0; i < 2 && g_currentAmountMoney >= g_towerMinCost; i++){
      	  // 敵が生きているかどうかをチェック
          int baseId = isAnyCreepReachableBase();

      	  if(baseId != NOT_REACH){
            BASE *base = getBase(baseId);
            updateDefenseValue(base, 5, g_healthRate * g_creepHealth);
        	  BUILD_INFO buildData = searchBestBuildPoint();

        	  if(canBuildTower(buildData.type, buildData.y, buildData.x)){
          	  buildTower(buildData.type, buildData.y, buildData.x);
        	  }
      	  }else{
            break;
          }

          g_aliveCreepsIdList = g_tempAliveCreepsIdList;
          resetCreepData();
        }
			}

      // ターンを1増やす
      g_currentTurn += 1;

			if(g_currentTurn == 2000){
				finalResult();
			}

      // タワーの建設情報を返して終わり
      return m_buildTowerData;
    }

		/**
		 * @fn
		 * 最終的な統計情報の出力
		 */
		void finalResult(){
			int spawnCount = g_spawnList.size();

			/*
			for(int spawnId = 0; spawnId < spawnCount; spawnId++){
				SPAWN *spawn = getSpawn(spawnId);

				fprintf(stderr,"Spawn %d: pop up creep count = %d\n", spawnId, spawn->popUpCreepCount);
			}
			*/

			for(int baseId = 0; baseId < g_baseCount; baseId++){
				fprintf(stderr,"Targeted Point %d = %d\n", baseId, g_targetedBasePoint[baseId]);
			}
		}

    /**
     * @fn [maybe]
     * ある地点から生存中の敵で一番近い敵のIDを返す
     * @param (tower) タワー情報
     *
     * @return 一番近い敵のID
     * @detail
     * タワーが敵をロックするときに使う
     */
    int searchMostNearCreepId(TOWER *tower){
      int mostNearCreepId = NOT_FOUND;
      int roughDist;
      int range = tower->range;
      int minDist = INT_MAX;

      set<int>::iterator it = g_aliveCreepsIdList.begin();

      // 生存中の敵をそれぞれ処理
      while(it != g_aliveCreepsIdList.end()){
        int creepId = (*it);
        CREEP *creep = getCreep(creepId);

        // 敵との距離を計算
        roughDist = calcRoughDist(tower->y, tower->x, creep->y, creep->x);

        if(minDist > roughDist && roughDist <= range * range){
          minDist = roughDist;
          mostNearCreepId = creepId;
        }

        it++;
      }

      return mostNearCreepId;
    }

    /**
     * @fn [not yet]
     * いずれかの敵が基地に到達したかどうかを調べる
     */
    int isAnyCreepReachBase(){
      set<int>::iterator it = g_aliveCreepsIdList.begin();

      while(it != g_aliveCreepsIdList.end()){
        int creepId = (*it);
        CREEP *creep = getCreep(creepId);
        CELL *cell = getCell(creep->y, creep->x);

        if(cell->isBasePoint()){
          //*
          return cell->baseId;
          /*/
          BASE *base = getBase(cell->baseId);
          if(base->isNotBroken()){
          }
          creep->state = NON_STOP;
          //*/
        }

        it++;
      }

      return NOT_REACH;
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
    int calcCoverPathCount(int fromY, int fromX, int range){
      int pathCount = 0;

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

          // もしセルの種別が経路であればカバーする範囲を増やす
          if(cell->isPath()){
            pathCount += 1;
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

      return pathCount;
    }

    /**
     * @fn
     * この地点にタワーを建てた時の評価値を計算する
     * @param (fromY) 出発地点のY座標
     * @param (fromX) 出発地点のX座標
     * @param (range) 攻撃範囲
     *
     * @return カバーしている経路の数
     * @detail
     * 「経路」のセルだけを評価対象にいれる
     */
    int calcBuildValue(int fromY, int fromX, int range, int damage){
      int value = 0;
			CELL *rootCell = getCell(fromY, fromX);

			if(g_realTowerCount == 1 && rootCell->aroundPathCount >= 3){
				value += 10000 * rootCell->aroundPathCount;
			}

      queue<COORD> que;
      que.push(COORD(fromY, fromX, 0));
      map<int, bool> checkList;
      set<int> coverSpawnList;

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

          if(cell->isPath()){
						if(cell->basicDamage == 0){
            	value += 4 * damage + cell->basicValue + cell->defenseValue + 2 * cell->pathCount;
						}else if(g_creepHealth <= 10){
            	value += cell->basicValue + cell->defenseValue + damage * cell->pathCount + min(cell->basicDamage, g_creepHealth * 8);
						}else{
            	value += cell->basicValue + cell->defenseValue + damage * cell->pathCount - min(cell->basicDamage, g_creepHealth * 8);
						}

            set<int>::iterator it = cell->spawnPaths.begin();
            while(it != cell->spawnPaths.end()){
              coverSpawnList.insert(*it);
              it++;
            }

            if(cell->aroundPathCount > 2){
              value += damage * (cell->aroundPathCount-1);
            }
					}else if(cell->isBasePoint()){
            value -= 1;
          }else if(cell->isPlain()){
            value -= 1;
          }else if(cell->isTowerPoint()){
            value -= 1;
					}

          // 上下左右のセルを追加
          for(int i = 0; i < 4; i++){
            int ny = coord.y + DY[i];
            int nx = coord.x + DX[i];

            // もし画面外で無い場合はキューに追加
            if(isInsideMap(ny, nx)){
              que.push(COORD(ny, nx, coord.dist+1));
            // 画面外をなるべく含めないように
            }else{
              value -= g_boardHeight/2;
            }
          }
        }
      }

      return value;
    }
};

int main(){
  int n, nc, b, money, creepHealth, creepMoney, nt;string row;vector<string> board;
  cin >> n;
  cin >> money;for(int y = 0; y < n; y++){cin >> row; board.push_back(row);}
  cin >> creepHealth;
  cin >> creepMoney;
  cin >> nt;
  vector<int> towerType(nt);
  for(int i = 0; i < nt; i++){cin >> towerType[i];}
  PathDefense pd;pd.init(board, money, creepHealth, creepMoney, towerType);
  for(int turn = 0; turn < LIMIT_TURN; turn++){
    //fprintf(stderr,"turn = %d\n", turn);
    cin >> money;
    cin >> nc;vector<int> creeps(nc);
    for(int creepId = 0; creepId < nc; creepId++){cin >> creeps[creepId];}
    cin >> b;vector<int> baseHealth(b);
    for(int baseId = 0; baseId < b; baseId++){cin >> baseHealth[baseId];}
    vector<int> ret = pd.placeTowers(creeps, money, baseHealth);
    cout << ret.size() << endl;
    for(int i = 0; i < ret.size(); i++){cout << ret[i] << endl;}}return 0;}

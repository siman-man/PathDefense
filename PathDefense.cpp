/**
 * @file    PathDefense.cpp
 * @brief   PathDefenseを解く用の何か
 * @author  siman
 * @date    2015/05/28
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
 *         0
 */
const int DY[4] = { 1, 0, -1, 0 };
const int DX[4] = {  0, -1, 0, 1};
  
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

/*
 * 座標を表す構造体
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

/*
 * スポーン地点を表す構造体
 */
typedef struct spawn {
  int id;                 // ID
  int y;                  // Y座標
  int x;                  // X座標

  spawn(int id = UNDEFINED, int y = UNDEFINED, int x = UNDEFINED){
    this->id = id;
    this->y  = y;
    this->x  = x;
  }
} SPAWN;

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

/*
 * @brief マップの各要素を表す構造体
 */
typedef struct cell {
  int type;                   //! Cellのタイプ
  int damage;                 //! この場所で与えられる最大ダメージ
  int coveredCount[MAX_R+1];  //! カバーできる経路の数
  int baseId;                 //! 基地がある場合はそのID
  int spawnId;                //! スポーン地点の場合はID
  int popUpCreepCount; // 出現した敵の数

  cell(int type = UNDEFINED){
    this->type    = type;
    this->damage  = UNDEFINED;
    this->baseId  = UNDEFINED;
    this->spawnId = UNDEFINED;
    this->popUpCreepCount = 0;
  }

  /**
   * @fn
   * 平地かどうかを返す
   * @return 平地かどうかの判定値
   */
  bool isPlain(){
    return type == PLAIN;
  }

  /**
   * @fn
   * 基地かどうかを返す
   * @return 基地かどうかの判定値
   */
  bool isBase(){
    return type == BASE_POINT;
  }

  /**
   * @fn
   * 経路かどうかを返す
   * @return 経路かどうかの判定値
   */
  bool isPath(){
    return type == PATH;
  }
} CELL;

/**
 * @fn
 * 文字を数値に変える関数
 * @param (ch) 変換したい文字
 *
 * @return 変換された数値
 */
int char2int(char ch){
  return ch - '0';
}

/**
 * @fn
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

//! ボードの横幅
int g_boardWidth;

//! ボードの縦幅
int g_boardHeight;

//! 最短路を得るためのマップ
int g_shortestPathMap[MAX_N][MAX_N][MAX_B*10+1][MAX_B+1];

//! 基地のリスト
BASE g_baseList[MAX_B];

//! 敵のリスト
CREEP g_creepList[MAX_Z];

//! 生存中の敵のIDリスト
set<int> g_aliveCreepIdList;

//! タワーのリスト
TOWER g_towerList[MAX_T];

//! 建設済みのタワーリスト
vector<TOWER> g_buildedTowerList;

//! スポーン地点のリスト
vector<SPAWN> g_spawnList;

//! 前に行動したstepを覚える配列
int g_prevStep[MAX_N][MAX_N];

//! 建設したタワーの数
int g_buildedTowerCount;

/**
 * @fn
 * (y,x)を1次元に直した場合の値を出す
 * @param (y) Y座標
 * @param (x) X座標
 *
 * @return 1次元座標表現時の値
 */
inline int calcZ(int y, int x){
  return y * g_boardHeight + x;
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
     */
    int init(vector<string> board, int money, int creepHealth, int creepMoney, vector<int> towerType){
      fprintf(stderr,"init =>\n");

      // ターンを初期化を行う
      g_currentTurn = 0;

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

      // 報酬の初期化
      g_reward = creepMoney;

      // 敵の初期体力の初期化
      g_creepHealth = creepHealth;

      // ゲーム情報の表示
      showGameData();
      
      return 0;
    }

    /**
     * @fn
     * ゲームの基礎情報を表示
     *   - 敵の基礎体力
     *   - 敵を倒した時の報酬
     */
    void showGameData(){
      fprintf(stderr,"-----------------------------------------------\n");
      fprintf(stderr,"creepHealth = %d\n", g_creepHealth);
      fprintf(stderr,"reward = %d\n", g_reward);
      fprintf(stderr,"-----------------------------------------------\n");
    }

    /**
     * @fn
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
          CELL cell = createCell();

          // '#'は平地
          if(board[y][x] == '#'){
            cell.type = PLAIN;

          // '.'は経路
          }else if(board[y][x] == '.'){
            cell.type = PATH;
            // マップの端であればスポーン地点の追加を行う
            if(y == 0 || x == 0 || y == g_boardHeight-1 || x == g_boardWidth-1){
              int spawnId = g_spawnList.size();
              // cellのタイプをスポーン地点で上書き
              cell.type = SPAWN_POINT;
              cell.spawnId = spawnId;

              // スポーン地点の追加
              addSpawnPoint(spawnId, y, x);
            }

          // それ以外は基地
          }else{
            // 基地のIDを取得
            int baseId = char2int(board[y][x]);
            cell.type = BASE_POINT;
            cell.baseId = baseId;

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

    /**
     * @fn
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
     * @fn
     * タワー情報の初期化を行う
     * @param (towerType) タワーの情報が格納されているリスト
     */
    void initTowerData(vector<int> &towerType){
      // タワーの種類の数
      int towerCount = towerType.size() / 3;

      fprintf(stderr,"towerCount = %d\n", towerCount);

      for(int towerId = 0; towerId < towerCount; towerId++){
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
     * @fn [not yet]
     * ある地点から出現した敵がどこかの基地にたどり着けるかどうかを確認
     * @param (fromY)  出発地点のY座標
     * @param (fromX)  出発地点のX座標
     * @param (health) 体力
     *
     * @sa canReachBasePoint
     * @return どこかの基地に到達できるかどうかの判定値
     */
    bool isAnyBaseReachable(int fromY, int fromX, int health){
      // 全ての基地に対して実行を行う
      for(int baseId = 0; baseId < g_baseCount; baseId++){
      }
    }     

    /**
     * @fn [not yet]
     *   各出現ポイントから基地までの最短経路を計算
     */
    void initSpawnToBaseShortestPath(){
      int spawnCount = g_spawnList.size();

      // スポーン地点毎に処理を行う
      for(int spawnId = 0; spawnId < spawnCount; spawnId++){
        calcSpawnToBaseShortestPath(spawnId);
      }
    }

    /**
     * @fn
     * 出現ポイントから基地までの最短経路を出す
     * @param (spawnY) スポーン地点のY座標
     * @param (spawnX) スポーン地点のX座標
     * @sa initSpawnToBaseShortestPath
     * @detail 最短距離は幅優先探索で出す
     */
    void calcSpawnToBaseShortestPath(int spawnId){
      map<int, bool> checkList;
      SPAWN *spawn = getSpawn(spawnId);
      queue<COORD> que;
      que.push(COORD(spawn->y, spawn->x));

      // 前回行動した情報のリセット
      memset(g_prevStep, UNDEFINED, sizeof(g_prevStep));

      while(!que.empty()){
        COORD coord = que.front(); que.pop();
        int z = calcZ(coord.y, coord.x);

        // もしチェックしたセルであれば処理を飛ばす
        if(checkList[z]) continue;
        checkList[z] = true;

        // セル情報を取得
        CELL *cell = getCell(coord.y, coord.x);

        // 基地に辿り着いた場合は経路を復元して登録を行う
        if(cell->type == BASE_POINT){
          int baseId = cell->baseId;
          // 最短経路の登録
          registShortestPath(spawnId, baseId);
        }else{
          for(int direct = 0; direct < 4; direct++){
            int ny = coord.y + DY[direct];
            int nx = coord.x + DX[direct];
            int nz = calcZ(ny, nx);

            if(isInsideMap(ny, nx) && !checkList[nz]){
              que.push(COORD(ny,nx));
              g_prevStep[ny][nx] = direct;
            }
          }
        }
      }
    }

    /**
     * @fn
     * 最短経路の登録を行う
     * @param (spawnId) スポーン地点のID
     * @param (baseId)  基地のID
     */
    void registShortestPath(int spawnId, int baseId){
       BASE  *base = getBase(baseId);
      SPAWN *spawn = getSpawn(spawnId);
      int y = base->y;
      int x = base->x;

      while(y != spawn->y || x != spawn->x){
        int prev = g_prevStep[y][x];
        assert(prev != -1);
        y += DY[(prev+2)%4];
        x += DX[(prev+2)%4];
        g_shortestPathMap[y][x][spawnId][baseId] = prev;
      }
    }

    /**
     * @fn
     * セルの作成を行う
     */
    CELL createCell(){
      CELL cell; 

      return cell;
    }

    /**
     * @fn
     * 敵を作成する
     * @param (creepId) creepのID 
     * @param (health)  体力
     * @param (y)       敵のY座標
     * @param (x)       敵のX座標
     *
     * @return 敵情報
     */
    CREEP createCreep(int creepId, int health, int y, int x){
      // セル情報の取得
      CELL *cell = getCell(y,x);

      // 敵のインスタンスを作成
      CREEP creep(creepId, health, y, x);

      // もし500ターン毎に体力が倍々に増える
      creep.health *= (1 << (g_currentTurn/500));
      
      // 現在のターン時に出現したことを記録
      creep.created_at = g_currentTurn;

      // 出現した敵の数を更新する
      cell->popUpCreepCount += 1;

      return creep;
    }

    /**
     * @fn
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
     * @fn
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
     * @fn
     * スポーン地点の追加を行う
     * @param (spawnId) スポーンID
     * @param (y)       Y座標
     * @param (x)       X座標
     */
    void addSpawnPoint(int spawnId, int y, int x){
      SPAWN spawn(spawnId, y, x);
      g_spawnList.push_back(spawn);

      fprintf(stderr,"Add spwan %d point: y = %d, x = %d\n", spawnId, y, x);
    }

    /**
     * @fn
     * タワー情報の表示
     * @param (towerId) タワーID
     */
    void showTowerData(int towerId){
      TOWER tower = getTower(towerId);

      double value = tower.range * (tower.damage*tower.damage) / (double)tower.cost/4.0;
      fprintf(stderr,"towerId = %d, range = %d, damage = %d, cost = %d, value = %4.2f\n", 
          towerId, tower.range, tower.damage, tower.cost, value);
    }

    /**
     * @fn
     * タワーの建設を行う(ゲーム中に使用)
     * @param (towerId) 建設するタワーの種類
     * @param (y)       建設を行うY座標
     * @param (x)       建設を行うX座標
     * 
     * @detail 建設情報もここで追加を行う
     */
    void buildTower(int towerId, int y, int x){
      TOWER tower = getTower(towerId);
      tower.y = y;
      tower.x = x;

      // 建設したタワーリストに追加
      g_buildedTowerList.push_back(tower);

      // 建設情報の追加
      m_buildTowerData.push_back(tower.x);
      m_buildTowerData.push_back(tower.y);
      m_buildTowerData.push_back(tower.id);
    }

    /**
     * @fn
     * 指定したIDの基地を取得する
     * @param (baseId) 基地ID
     *
     * @return 基地情報のポインタ
     */
    BASE* getBase(int baseId){
      return &g_baseList[baseId];
    }

    /**
     * @fn
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
     * 指定したIDのタワー情報を取得
     * @param (towerId) タワーID
     */
    TOWER getTower(int towerId){
      return g_towerList[towerId];
    }

    /**
     * @fn
     * スポーン地点の取得
     * @param (spawnId) スポーンID
     * 
     * @return スポーン地点の情報を指すポインタ
     */
    SPAWN* getSpawn(int spawnId){
      return &g_spawnList[spawnId];
    }

    /**
     * @fn
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
     * @fn
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
     * @fn
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
     * @fn
     * タワーが建設可能かどうかを調べる
     * @param (towerId) 建設したいタワーのID
     * @param (y)       Y座標
     * @param (x)       X座標
     *
     * @return 建設可能かどうかの判定値
     */
    bool canBuildTower(int towerId, int y, int x){
      CELL *cell = getCell(y, x);

      // マップ内であり、平地であり、所持金が足りている場合は建設可能
      return (isOutsideMap(y, x) && cell->isPlain() && g_towerList[towerId].cost <= g_currentAmountMoney);
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

    /**
     * @fn
     * 敵情報の更新
     * @param (creeps) 敵の情報リスト
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

    /**
     * @fn
     * 基地情報の更新を行う
     * @param (baseHealth) 基地の体力情報のリスト
     */
    void updateBasesData(vector<int> &baseHealth){
      // 基地の数
      g_baseCount = baseHealth.size();

      // 各基地の体力を更新
      for(int baseId = 0; baseId < g_baseCount; baseId++){
        BASE *base = getBase(baseId);

        base->health = baseHealth[baseId];
      }
    }

    /**
     * @fn
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
     * @fn
     * ある地点から生存中の敵で一番近い敵のIDを返す
     * @param (fromY) 出発地点のy座標
     * @param (fromX) 出発地点のx座標
     *
     * @return 一番近い敵のID
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

    /**
     * @fn [not yet]
     * ある敵が特定の基地まで、指定した体力を残しながら到達できるかどうかを調べる
     * @param (creepId) 敵ID
     * @param (baseId) 基地ID
     * 
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

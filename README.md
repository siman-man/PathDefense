# PathDefense  

You are given a square map containing N by N cells. On the map you have B bases at fixed locations. 
あなたにN * Nのセルで構成された正方形のマップが与えられます。 マップ上には固定されたB個の基地が存在します。

A number of P paths lead from the boundary of the map towards your bases. Your bases are under attack and 
Pはマップの端から基地に向かう経路の数を表します。                               あなたの基地は現在攻撃を受けており、

you need to defend them by placing defensive towers on the map. Luckily creeps only walk on the paths and 
あなたは防御するためのタワーを設置することで基地を守る必要があります。    幸いなことに侵略者達は経路上しか歩けず、

can not attack your towers. You are rewarded with a fixed amount of money for each creep that your towers kill. 
あなたのタワーを攻撃することが出来ません。 あなたは、自身のタワーが侵略者を倒すたびに一定の報酬を得ます。

You can build T types of towers, each tower will cost you money and can only attack creeps within a specific range and
あなたはTタイプの数のタワーの構築を行うことが可能であり、それぞれのタワーは一定のコストがかかります。また、各タワーは攻撃範囲内に存在

will do a fixed amount of damage when attacking the closest creep. Only one tower can be placed on each cell in the map.
する一番近い侵略者に対して固定ダメージを与えます。                        タワーはマップ上にあるセルの上にしか建設できません。

Multiple creeps can occupy the same cell. Creeps will spawn randomly on the boundary of the map and select random paths
複数の侵略者は同じセルを占領することが出来ます。 侵略者はランダムな経路から出現し、あなたの基地を目指して進みます。

towards your bases. Additionally, at random times waves of creeps will rush towards some of your bases. 
                     また、        ランダムな時間帯ではあなたの基地に向かって、侵略者による一斉攻撃が開始されます。
A creep will do damage to your base when it reaches it and disappear after that (The amount of damage 
侵略者はタワーに到達した際に、タワーに対してダメージを与え、その後に消えます。(侵略者がタワーに与えるダメージは、)

will be equal to the amount of health of the creep).
タワーに到達した際の侵略者の体力です。

Your task is to maximize the sum of the amount of money you have and the total base health at the end of the simulation.
あなたの目的は、ゲーム終了時に基地の体力と所持金を最大にすることです。

# Implementation Details
実装詳細

Your code should implement the method init(vector <string> board, int money, int creepHealth, int creepMoney, vector <int> towerType). 
あなたはinit(vector <string> board, int money, int creepHealth, int creepMoney, vector<int> towerType)を実装する必要があります。

Your init method will be called once, before the simulation starts. You can return any int from init() method, it will be ignored.
シミュレーション開始前にinitメソッドが呼ばれます、その際に値は何を返しても構いません。（その値は無視されます）

- board gives you the map containing N by N cells. Each string contains a row of the map. Any numeric character ('0'..'9')
 denotes a base and it's index, '.' a path and '#' a cell where towers can be placed.
boardはN * Nのセルを含んでいます。各文字列はマップの行を表しており、('0'..'9')は基地の番号を、'.'は経路を'#'はタワーが建設できる
セルを表しています。

- money 
gives you the amount of money in hand at the start of the simulation.
moneyはシミュレーション開始時にあなたが使用できる資金の合計です。

- creepHealth 
gives you the amount of health points a creep will initially spawn with. 
Note that after every 500 simulation steps, the starting health of newly spawned creeps will double.
侵略者の初期体力です。500ターン以降に出現する侵略者の体力は2倍になります。

- creepMoney
gives you the amount of money received for killing a creep.
侵略者を倒した際に得られる報酬金額です。

towerType 
gives you information about each type of tower that can be placed. 
あなたには設置出来るタワーのタイプが提供されます。

Tower type i will shoot creeps within a range of towerType[i*3] cells and decrease the health of the creep by 
タイプiのタワーは射程内(towerType[i*3])に存在する侵略者を攻撃します。攻撃された侵略者はタワーの攻撃力(towerType[i*3+1])

towerType[i*3+1] points. It will cost you towerType[i*3+2] to place the tower.
の分だけ体力が減少します。   タワーの建設するためのコストは[i*3+2]です。

Your code should implement the method placeTowers(vector <int> creep, int money, vector <int> baseHealth). 
あなたの提出するコードはplaceTowers(vector<int> creep, int money, vector<int> baseHealth)メソッドを実装する必要
があります。

Your method will be called 2000 times, once for every simulation step.
このメソッドはシミューレーションで毎ターン呼び出されます(2000回)

- creep 
gives you information about the current creeps on the map. Each creep is described by 4 values. 
現在マップ上に存在する敵の情報です。各侵略者毎に4つの情報が提供されます。

Creep i is at column creep[i*4+2] and row creep[i*4+3]. creep[i*4+1] contains the health of the creep. 
侵略者iの位置情報(y - creep[i*4+3], x - creep[i*4+2])、侵略者iの体力[i*4+1]

creep[i*4] contains the unique creep id that can be used to track the creep movements.
creep[i*4]には追跡用に侵略者に割り振られた固定IDが含まれています。

- money 
gives you the current amount of money in hand.

- baseHealth
gives you the health of your bases. baseHealth[i] contains the health of base i.

You must return the location and type of towers that you want to buy and place on the map. 
あなたは新しくタワーを建設したい場合、タワーのタイプと位置を返す必要があります。

Each 3 elements of your return must describe one tower placement. Let vector <int> ret be your return, 
1つのタワーにつき3つの要素を含める必要があります。                        retを例にすると。

then ret[j*3] contains the column, ret[j*3+1] the row and ret[j*3+2] the tower type of your j-th tower to be 
タワーの位置(y: ret[j*3+1], x: ret[j*3])、タワーのタイプ[j*3+2]、タワーは現在のターン中に作成が行われます。
placed during the current simulation step. 
You can only buy towers that you can afford with the current money in hand.
タワーの建設は必要なコストが存在する場合に建設可能です。

Trying to buy a tower that you can not afford will result in a zero score. You don't have to place a tower at each step,
もし、タワーの建設を行おうとして失敗した場合はスコアが0になります。                  あなたは毎ターン、タワーを建設する必要はありません。
you can return an empty array.
建設を行わない場合は空の配列を返して下さい。

# Scoring

Your score for an individual test case will be the sum of the amount of money in hand and base health at the 
あなたのスコアは各テストケースにおいて、シミュレーション終了時の所持金とタワーの体力の合計値となります。

end of the 2000 simulation steps. If your return has invalid format or specifies any invalid tower placements, 
                                   もし不正なフォーマットで解答を行った場合のテストケースではスコアは0となります。
your score for the test case will be 0. Your overall score will be calculated in the following way: for each 
                                         あなたのスコアは以下のように算出されます。
test case where your score is not 0, you get 1 point for each competitor you beat on this test case 
もし、あなたのスコアが0でない場合         1番良いスコアを獲得した場合は1がもらえます。
(i.e., your score on a test case is larger than this competitor's score) and 0.5 points for each competitor you 
tie with (a tie with yourself is not counted); finally, the sum of points is divided by (the number of competitors - 1),
then multiplied by 1000000 and divided by the number of test cases.

# Clarifications

- There will be 2000 simulation steps for each test case.
各テストケースにおいてシミュレーションのステップは2000まで行います。

- Each base will start with a health of 1000.
基地の体力は1000です。

- Once a tower has been placed, it will stay there for the remaining part of the simulation. It can not be destroyed, moved or upgraded.
一度タワーが設置された場合、そのタワーはそのセルに居続けます、破壊や移動、更新等の操作はできません。

- Each tower will only attack once in each step and will only attack the nearest creep (if in range). If more than one creep is at the same nearest distance, the one with the lowest unique id will be attacked.
タワーは1ターンに1度しか攻撃出来ません。攻撃は射程内に存在する一番近い敵に対して行われます。
複数の敵が同じ距離で存在する場合はIDが低い敵に対して攻撃が行われます。

- If the tower is located at position (X1,Y1) and the range of a tower is R, then the tower can attack a creep at position (X2,Y2) only if (X1-X2)*(X1-X2)+(Y1-Y2)*(Y1-Y2)<=R*R.
タワーの位置が(X1, Y1)で射程がRの場合、敵の位置が(X2, Y2)だとして、(X1-X2) * (X1-X2) + (Y1-Y2) * (Y1-Y2) <= R*R
の場合に攻撃します。

- Creeps will move one cell each simulation step.
敵は1stepに1マス移動します。

- Creep will attack any base, even if the base has zero health.
敵は基地の体力が0であっても任意の基地を攻撃します。

- Creep will disappear after it attacked a base.
敵は基地を攻撃後に消滅します。

- After every 500 simulation steps, the starting health of newly spawned creeps will double.
500ターン目以降の敵の体力は初期体力の2倍になります。

- Multiple creeps can occupy the same cell on the map.
1つのセルに対して複数の敵が存在できます。

- Towers can only be placed on cells that are not a base or a path, at most one tower per cell.
タワーは経路上でも基地の上でも無いセルの上に建設することが可能です。1つのセルに1つのタワーまで建設出来ます。

- The amount of damage a creep will cause to a base is the creep's current health when reaching the base.
敵の基地に対する攻撃は基地に到達した段階での敵の体力となります。

- The order of execution for each simulation step will be: Place your new towers, Creeps move and attack, your towers attack creeps.
各ターンでは以下のように処理が行われます。
* 新たなタワーの建設
* 敵の進行、攻撃
* タワーの攻撃

- Your towers will attack in sequence, those that were build earlier will attack first. If a tower kills a creep, the next attacking tower will not try to attack the dead creep.
タワーの攻撃は連続的に行われます。建設した順番で攻撃が行われます。もしタワーの攻撃によって敵が倒された場合、次のタワーは
倒された敵を攻撃することはありません。

- The base health can't go lower than zero.
体力は0より小さくなりません。

# Test Case Generation

Please look at the visualizer source code for more detail about test case generation. Each test case is generated as follows:

- The map size N is randomly selected between 20 and 60, inclusive.
マップサイズNは20から60の値を取ります。

- The creep starting health is chosen between 1 and 20. The money reward for a creep is chosen between 1 and 20.
敵の初期体力は1から20の値を取ります。また敵を倒した時の報酬も1から20の値を取ります。

- The number of tower types T is randomly selected between 1 and 20, inclusive. For each tower type the range is chosen between 1 and 5. The damage done is chosen between 1 and 5. The cost is chosen between 5 and 40.
タワーのタイプの数は1から20の値を取ります。射程距離は1から5、攻撃力は1から5、建設コストは5から40の値を取ります。

- The starting amount of money is equal to the sum of all tower type costs.
初期の所持金は建設可能なタワーの建設コストの合計値です

- The number of bases B is randomly selected between 1 and 8, inclusive. The location of each base is selected randomly and will be at least 4 cells away from the boundary of the map.
基地IDの1から8の値がランダムに割り振られます。また、基地の初期配置はランダムですが、マップの端から最低でも4マスは離れているものとします。

- The number of paths P is randomly selected between B and B*10. P random paths are then created between a random boundary location and random base.
経路の数PはBからB*10の間の値からランダムに選択されます。 Pはランダムに選択された基地への経路です

- The number of total creeps Z is randomly selected between 500 and 2000. Z random creeps are created to spawn at specific simulation times at specific boundary points on paths.
敵の総数Zは500から2000の間からランダムに選択されます。各敵はランダムに選択された経路Pのマップの端から出現します。

- The number of waves W is randomly selected between 1 and 15. W waves of attacking creeps are selected to be spawned at the same boundary location. The wave of creeps will spawn closely together at a random time during the simulation.
敵の波状攻撃Wは1から15の値を取ります。波状攻撃ではかなり短い間隔で同じ場所から敵が出現します。、波状攻撃の時間はランダムです。

- All values are chosen uniformly and independently, at random.
各選択される値は一様に分布したランダム関数です。

# Notes

- The time limit is 20 seconds per test case (this includes only the time spent in your code). The memory limit is 1024 megabytes.
各テストケースの実行時間は20秒です。またメモリの使用上限は1GBです。

- There is no explicit code size limit. The implicit source code size limit is around 1 MB (it is not advisable to submit codes of size close to that or larger). Once your code is compiled, the binary size should not exceed 1 MB.
コードのサイズ上限はハッキリとは示しませんが、1MBの周辺が上限となります。

- The compilation time limit is 30 seconds. You can find information about compilers that we use and compilation options here.
コンパイルの制限時間は30秒です。コンパイルオプションはここから閲覧することができます。

- There are 10 example test cases and 50 full submission (provisional) test cases.
Example Testでは10ケース、Full Submissionでは50ケースでテストを行います。


# 今回意識すること

- 今回はローカルのテストのシード値を1001-1100で行う
  - 本番テストでスコア下がっても気にしない

# 欲しい機能

## 敵の総数を予測

敵の総数を予測することで建設したほうが得かそうでないかを判断する


## 建設するにあたって効率の良い場所

効率よくタワーを立てることで敵の侵入を防ぐ

### 効率の良い場所とは

- 経路を多くカバー出来る場所

## 各出現ポイントから各基地への経路予測

- testerのソースコードを読んで敵の経路生成部分を把握する。

# 知りたいこと

## 敵は最短の基地を狙うのかどうか


## スコアの最大値について

### 初期スコア

- 基地の体力は1000
- 初期資金は「建設可能なタワーのコストの合計値」

なので

1000 * 基地の数 + 初期資金

となる。

ここからまず

- 基地の体力は増えることは無い
- 初期資金は増える

ことを前提に

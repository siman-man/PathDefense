/*
Change log
----------
2015-05-25 :
Initial release

2015-05-26 :
Creep health of waves incorrect,
// creeps[wi].health = creepType.health;

fix2:
creeps[wi].health = creepType.health * (1<<(creeps[wi].spawnTime/500));

*/

import javax.swing.*;
import java.awt.*;
import java.awt.event.KeyAdapter;
import java.awt.event.KeyEvent;
import java.awt.event.WindowAdapter;
import java.awt.event.WindowEvent;
import java.io.BufferedReader;
import java.io.InputStream;
import java.io.InputStreamReader;
import java.io.PrintWriter;
import java.security.SecureRandom;
import java.util.*;
import java.util.List;

class Constants {
    public static final int SIMULATION_TIME = 2000;
    public static final int[] DY = new int[]{1, 0, -1, 0};
    public static final int[] DX = new int[]{0, -1, 0, 1};
}

class CreepType {
    public static final int MIN_CREEP_HEALTH = 1;
    public static final int MAX_CREEP_HEALTH = 20;
    public static final int MIN_CREEP_MONEY = 1;
    public static final int MAX_CREEP_MONEY = 20;
    int health;
    int money;
    public CreepType(Random rnd) {
        // 敵の体力をランダムに設定
        health = rnd.nextInt(MAX_CREEP_HEALTH - MIN_CREEP_HEALTH + 1) + MIN_CREEP_HEALTH;
        // 敵の報酬をランダムに設定
        money = rnd.nextInt(MAX_CREEP_MONEY - MIN_CREEP_MONEY + 1) + MIN_CREEP_MONEY;
    }
}

class Creep {
    int id;
    int health;
    int x, y;
    int spawnTime;
    ArrayList<Integer> moves = new ArrayList<Integer>();
}

class TowerType {
    public static final int MIN_TOWER_RANGE = 1;
    public static final int MAX_TOWER_RANGE = 5;
    public static final int MIN_TOWER_DAMAGE = 1;
    public static final int MAX_TOWER_DAMAGE = 5;
    public static final int MIN_TOWER_COST = 5;
    public static final int MAX_TOWER_COST = 40;
    int range;
    int damage;
    int cost;
    public TowerType(Random rnd) {
        // 攻撃範囲をランダムに設定
        range = rnd.nextInt(MAX_TOWER_RANGE - MIN_TOWER_RANGE + 1) + MIN_TOWER_RANGE;
        // 攻撃力をランダムに設定
        damage = rnd.nextInt(MAX_TOWER_DAMAGE - MIN_TOWER_DAMAGE + 1) + MIN_TOWER_DAMAGE;
        // 建設コストをランダムに設定
        cost = rnd.nextInt(MAX_TOWER_COST - MIN_TOWER_COST + 1) + MIN_TOWER_COST;
    }

}

class Tower {
    int x, y;
    int type;
}
class AttackVis {
    int x1, y1, x2, y2;
    public AttackVis(int _x1, int _y1, int _x2, int _y2) {
        x1 = _x1;
        y1 = _y1;
        x2 = _x2;
        y2 = _y2;
    }
}

class TestCase {
    public static final int MIN_CREEP_COUNT = 500;
    public static final int MAX_CREEP_COUNT = 2000;
    public static final int MIN_TOWER_TYPES = 1;
    public static final int MAX_TOWER_TYPES = 20;
    public static final int MIN_BASE_COUNT = 1;
    public static final int MAX_BASE_COUNT = 8;
    public static final int MIN_WAVE_COUNT = 1;
    public static final int MAX_WAVE_COUNT = 15;
    public static final int MIN_BOARD_SIZE = 20;
    public static final int MAX_BOARD_SIZE = 60;

    public int boardSize;
    public int money;

    public CreepType creepType;

    public int towerTypeCnt;
    public TowerType[] towerTypes;

    public char[][] board;
    public int pathCnt;
    public int[] spawnX;
    public int[] spawnY;
    public int[][] boardPath;
    public int tx, ty;

    public int baseCnt;
    public int[] baseX;
    public int[] baseY;

    public int creepCnt;
    public Creep[] creeps;
    public int waveCnt;

    public SecureRandom rnd = null;

    // 出現ポイントと基地の接続を行う
    //   x1: 出現ポイントのx座標
    //   y1: 出現ポイントのy座標
    //   x2: 基地のx座標
    //   y2: 基地のy座標
    public void connect(Random rnd, int x1, int y1, int x2, int y2) {
        // 目的地に到着するまで処理を続ける
        while (x1!=x2 || y1!=y2) {
            // 目的地以外の基地についたら抜ける
            if (board[y1][x1]>='0' && board[y1][x1]<='9') return;
            // 平地を経路に
            board[y1][x1] = '.';
            int x1_ = x1;
            int y1_ = y1;
            // x座標が一致している場合
            if (x1==x2) {
                if (y2>y1) {
                    y1++;
                } else {
                    y1--;
                }
            // y座標が一致している場合
            } else if (y1==y2) {
                if (x2>x1) {
                    x1++;
                } else {
                    x1--;
                }
            } else {
                int nx = x1;
                int ny = y1;
                if (x2>x1) nx++; else nx--;
                if (y2>y1) ny++; else ny--;
                // すでに経路が存在している場合はそこを優先
                if (board[ny][x1]=='.') { y1 = ny; }
                else if (board[y1][nx]=='.') { x1 = nx; }
                // どちらも平地の場合は50%の確率でどこかに進む
                else {
                    if (rnd.nextInt(2)==0)
                        y1 = ny;
                    else
                        x1 = nx;
                }
            }
            // 現在のセルからどの方角に行動できるのかをフラグで管理
            if (x1>x1_) boardPath[y1_][x1_] |= 8;
            else if (x1<x1_) boardPath[y1_][x1_] |= 2;
            else if (y1>y1_) boardPath[y1_][x1_] |= 1;
            else if (y1<y1_) boardPath[y1_][x1_] |= 4;
        }
    }

    // 経路の追加を行う
    public void addPath(Random rnd, int baseIdx) {
        int sx=0,sy=0;
        boolean nextTo = false;
        int tryEdge = 0;
        do
        {
            tryEdge++;
            if (tryEdge>boardSize) break;
            nextTo = false;
            // sxをランダムに設定
            sx = rnd.nextInt(boardSize-1)+1;
            // 50%の確率で↑
            if (rnd.nextInt(2)==0) {
                // syを0かboardSize-1に設定
                sy = rnd.nextInt(2)*(boardSize-1);
                if (sx>0 && board[sy][sx-1]=='.') nextTo = true;
                if (sx+1<boardSize && board[sy][sx+1]=='.') nextTo = true;
            } else
            {
                // y座標とx座標を入替え
                sy = sx;
                sx = rnd.nextInt(2)*(boardSize-1);
                if (sy>0 && board[sy-1][sx]=='.') nextTo = true;
                if (sy+1<boardSize && board[sy+1][sx]=='.') nextTo = true;
            }
        } while (nextTo || board[sy][sx]!='#');

        // 経路長がボードの長さを超えていたら何もしない
        if (tryEdge>boardSize) return;

        // 出現ポイントを作成
        board[sy][sx] = '.';
        spawnX[baseIdx] = sx;
        spawnY[baseIdx] = sy;
        if (sx==0) { boardPath[sy][sx] |= 8; sx++; }
        else if (sy==0) { boardPath[sy][sx] |= 1; sy++; }
        else if (sx==boardSize-1) { boardPath[sy][sx] |= 2; sx--; }
        else { boardPath[sy][sx] |= 4; sy--; }
        int b = baseIdx%baseCnt;
        // 最低でも1つの基地に向かう経路は作成して、あとはランダムで作る
        if (baseIdx>=baseCnt) b = rnd.nextInt(baseCnt);
        connect(rnd, sx, sy, baseX[b], baseY[b]);
    }

    public TestCase(long seed) {

        try {
            // 新しい乱数の生成
            rnd = SecureRandom.getInstance("SHA1PRNG");
        } catch (Exception e) {
            System.err.println("ERROR: unable to generate test case.");
            System.exit(1);
        }

        // シード値の設定を行う
        rnd.setSeed(seed);
        boolean genDone = true;
        // もし基地に到達できない敵が1体でもいたらフィールドを作成し直す
        do {
            // ボードサイズをランダムに設定
            boardSize = rnd.nextInt(MAX_BOARD_SIZE - MIN_BOARD_SIZE + 1) + MIN_BOARD_SIZE;
            // シードが1の場合は20固定
            if (seed==1) boardSize = 20;
            // ボードを作成
            board = new char[boardSize][boardSize];
            // ボードの経路を作成
            boardPath = new int[boardSize][boardSize];
            // 敵のタイプを決定
            creepType = new CreepType(rnd);

            // タワーの種類をランダムに設定
            towerTypeCnt = rnd.nextInt(MAX_TOWER_TYPES - MIN_TOWER_TYPES + 1) + MIN_TOWER_TYPES;
            towerTypes = new TowerType[towerTypeCnt];
            // 初期の所持金を初期化
            money = 0;
            for (int i = 0; i < towerTypeCnt; i++) {
                // タワーを作成
                towerTypes[i] = new TowerType(rnd);
                // 所持金をタワーのコスト分増やす
                money += towerTypes[i].cost;
            }
            // 基地の数を設定
            baseCnt = rnd.nextInt(MAX_BASE_COUNT - MIN_BASE_COUNT + 1) + MIN_BASE_COUNT;
            // ボードの初期化
            for (int y=0;y<boardSize;y++) {
                for (int x=0;x<boardSize;x++) {
                    board[y][x] = '#';
                    boardPath[y][x] = 0;
                }
            }
            baseX = new int[baseCnt];
            baseY = new int[baseCnt];
            for (int i=0;i<baseCnt;i++) {
                int bx,by;
                do
                {
                    // 基地の位置をランダムに設定(最低でも4マスは離れるようにする)
                    bx = rnd.nextInt(boardSize-8)+4;
                    by = rnd.nextInt(boardSize-8)+4;
                // 既に基地があるところには設置しない
                } while (board[by][bx]!='#');
                // ボードに基地の位置を設定
                board[by][bx] = (char)('0'+i);
                // 位置情報の設定
                baseX[i] = bx;
                baseY[i] = by;
            }

            // 経路の数をランダムに設定
            pathCnt = rnd.nextInt(baseCnt*10 - baseCnt + 1) + baseCnt;
            // 出現ポイント
            spawnX = new int[pathCnt];
            spawnY = new int[pathCnt];
            // 経路の数だけ新しく経路を生成する
            for (int i=0;i<pathCnt;i++) {
                addPath(rnd, i);
            }

            // 敵の総数を決定
            creepCnt = rnd.nextInt(MAX_CREEP_COUNT - MIN_CREEP_COUNT + 1) + MIN_CREEP_COUNT;
            if (seed==1) creepCnt = MIN_CREEP_COUNT;
            creeps = new Creep[creepCnt];
            // それぞれの敵情報を作成
            for (int i=0;i<creepCnt;i++) {
                Creep c = new Creep();
                int j = rnd.nextInt(pathCnt);
                // 出現座標を決定
                c.x = spawnX[j];
                c.y = spawnY[j];
                c.id = i;
                // 出現時間の設定
                c.spawnTime = rnd.nextInt(Constants.SIMULATION_TIME);
                // 体力の設定、500ターン毎に倍々で増えていく
                c.health = creepType.health * (1<<(c.spawnTime/500));
                creeps[i] = c;
            }
            // waves
            // 波状攻撃の回数を設定
            waveCnt = rnd.nextInt(MAX_WAVE_COUNT - MIN_WAVE_COUNT + 1) + MIN_WAVE_COUNT;
            int wi = 0;
            for (int w=0;w<waveCnt;w++) {
                // 出現ポイントをランダムで設定
                int wavePath = rnd.nextInt(pathCnt);
                // 波状攻撃を行う敵の数を設定(最低5,最大5+敵の総数/20-1)
                int waveSize = 5+rnd.nextInt(creepCnt/20);
                // 波状攻撃の始まる時間を設定
                int waveStartT = rnd.nextInt(Constants.SIMULATION_TIME-waveSize);
                for (int i=0;i<waveSize;i++) {
                    // 敵の総数を超えたら終了
                    if (wi>=creepCnt) break;
                    creeps[wi].x = spawnX[wavePath];
                    creeps[wi].y = spawnY[wavePath];
                    // 出現するタイミングをランダムで設定
                    creeps[wi].spawnTime = waveStartT + rnd.nextInt(waveSize);
                    creeps[wi].health = creepType.health * (1<<(creeps[wi].spawnTime/500));
                    wi++;
                }
                if (wi>=creepCnt) break;
            }

            // determine paths for each creep
            // 敵の経路を生成
            genDone = true;
            for (Creep c :creeps) {
                // 経路をクリア
                c.moves.clear();
                int x = c.x;
                int y = c.y;
                int prevx = -1;
                int prevy = -1;
                int tryPath = 0;
                // 基地に到達するまで繰り返す
                while (!(board[y][x]>='0' && board[y][x]<='9')) {
                    int dir = 0;
                    tryPath++;
                    if (tryPath>boardSize*boardSize) break;
                    // select a random direction that will lead to a base, don't go back to where the creep was in the previous time step
                    int tryDir = 0;
                    do
                    {
                        // 4方向全てためしたら-1にリセット
                        if (tryDir==15) { tryDir = -1; break; }
                        // ランダムで方向を決定する
                        dir = rnd.nextInt(4);
                        tryDir |= (1<<dir);
                        // 行動できない場所
                    } while ((boardPath[y][x]&(1<<dir))==0 ||
                                // 前回行動したマスであれば繰り返す
                              (x+Constants.DX[dir]==prevx && y+Constants.DY[dir]==prevy));
                    // 4方向全て試した場合はそのまま終了
                    if (tryDir<0) break;
                    // 方向を設定
                    c.moves.add(dir);
                    // 前の行動履歴を保存
                    prevx = x;
                    prevy = y;
                    // 位置情報の更新
                    x += Constants.DX[dir];
                    y += Constants.DY[dir];
                }
                if (!(board[y][x]>='0' && board[y][x]<='9')) {
                    genDone = false;
                    break;
                }
            }
        } while (!genDone);
    }
}

class Drawer extends JFrame {
    public static final int EXTRA_WIDTH = 200;
    public static final int EXTRA_HEIGHT = 100;

    public World world;
    public DrawerPanel panel;

    public int cellSize, boardSize;
    public int width, height;

    public boolean pauseMode = false;
    public boolean stepMode = false;
    public boolean debugMode = false;

    class DrawerKeyListener extends KeyAdapter {
        public void keyPressed(KeyEvent e) {
            synchronized (keyMutex) {
                if (e.getKeyChar() == ' ') {
                    pauseMode = !pauseMode;
                }
                if (e.getKeyChar() == 'd') {
                    debugMode = !debugMode;
                }
                if (e.getKeyChar() == 's') {
                    stepMode = !stepMode;
                }
                keyPressed = true;
                keyMutex.notifyAll();
            }
        }
    }

    class DrawerPanel extends JPanel {
        public void paint(Graphics g) {
            synchronized (world.worldLock) {
            int cCnt[][] = new int[boardSize][boardSize];
            g.setColor(new Color(0,128,0));
            g.fillRect(15, 15, cellSize * boardSize + 1, cellSize * boardSize + 1);
            g.setColor(Color.BLACK);
            for (int i = 0; i <= boardSize; i++) {
                g.drawLine(15 + i * cellSize, 15, 15 + i * cellSize, 15 + cellSize * boardSize);
                g.drawLine(15, 15 + i * cellSize, 15 + cellSize * boardSize, 15 + i * cellSize);
            }
            g.setColor(new Color(32,32,32));
            for (int i=0; i < boardSize; i++) {
                for (int j=0; j < boardSize; j++) {
                    if (world.tc.board[i][j]=='.') {
                        cCnt[i][j] = 0;
                        g.fillRect(15 + j * cellSize + 1, 15 + i * cellSize + 1, cellSize - 1, cellSize - 1);
                    }
                }
            }

            g.setColor(Color.WHITE);
            for (int i=0; i < boardSize; i++) {
                for (int j=0; j < boardSize; j++) {
                    if (world.tc.board[i][j]>='0' && world.tc.board[i][j]<='9') {
                        g.fillRect(15 + j * cellSize + 1, 15 + i * cellSize + 1, cellSize - 1, cellSize - 1);
                    }
                }
            }
            // draw the health of each base
            for (int b=0;b<world.tc.baseCnt;b++) {
                int col = world.baseHealth[b]*255/1000;
                g.setColor(new Color(col, col, col));
                g.fillRect(15 + world.tc.baseX[b] * cellSize + 1+cellSize/4, 15 + world.tc.baseY[b] * cellSize + 1+cellSize/4, cellSize/2 - 1, cellSize/2 - 1);
            }


            for (int i=0; i < boardSize; i++) {
                for (int j=0; j < boardSize; j++) {
                    if (world.tc.board[i][j]>='A') {
                        int ttype = world.tc.board[i][j]-'A';
                        float hue = (float)(ttype) / world.tc.towerTypeCnt;
                        // tower color
                        g.setColor(Color.getHSBColor(hue, 0.9f, 1.0f));
                        g.fillOval(15 + j * cellSize + 2, 15 + i * cellSize + 2, cellSize - 4, cellSize - 4);
                        if (debugMode) {
                            // draw area of attack
                            int r = world.tc.towerTypes[ttype].range;
                            g.drawOval(15 + (j-r) * cellSize + 1, 15 + (i-r) * cellSize + 1, cellSize*(r*2+1) - 1, cellSize*(r*2+1) - 1);
                            g.setColor(new Color(128, 128, 128, 30));
                            g.fillOval(15 + (j-r) * cellSize + 1, 15 + (i-r) * cellSize + 1, cellSize*(r*2+1) - 1, cellSize*(r*2+1) - 1);
                        }
                    }
                }
            }
            for (Creep c : world.tc.creeps)
            if (c.health>0 && c.spawnTime<world.curStep) {
                float h = Math.min(1.f, (float)(c.health) / (world.tc.creepType.health));
                g.setColor(new Color(h,0,0));
                g.fillRect(15 + c.x * cellSize + 1 + cCnt[c.y][c.x], 15 + c.y * cellSize + 1 + cCnt[c.y][c.x], cellSize - 1, cellSize - 1);
                cCnt[c.y][c.x]+=2;
            }

            g.setColor(Color.GREEN);
            for (AttackVis a : world.attacks) {
                g.drawLine(15 + a.x1 * cellSize + cellSize/2, 15 + a.y1 * cellSize + cellSize/2,
                           15 + a.x2 * cellSize + cellSize/2, 15 + a.y2 * cellSize + cellSize/2);
            }

            g.setColor(Color.BLACK);
            g.setFont(new Font("Arial", Font.BOLD, 12));
            Graphics2D g2 = (Graphics2D)g;

            int horPos = 40 + boardSize * cellSize;

            g2.drawString("Board size = " + boardSize, horPos, 30);
            g2.drawString("Simulation Step = " + world.curStep, horPos, 50);
            g2.drawString("Creeps Spawned = " + world.numSpawned, horPos, 70);
            g2.drawString("Creeps killed = " + world.numKilled, horPos, 90);
            g2.drawString("Towers placed = " + world.numTowers, horPos, 110);
            g2.drawString("Money gained = " + world.moneyIncome, horPos, 130);
            g2.drawString("Money spend = " + world.moneySpend, horPos, 150);
            g2.drawString("Money = " + world.totMoney, horPos, 170);
            int baseh = 0;
            for (int i=0;i<world.baseHealth.length;i++) {
                g.setColor(Color.GREEN);
                g.fillRect(horPos+30, 205+i*20, world.baseHealth[i]/10, 19);
                g.setColor(Color.BLACK);
                g.fillRect(horPos+30+world.baseHealth[i]/10, 205+i*20, 100 - world.baseHealth[i]/10, 19);
                g2.drawString(Integer.toString(world.baseHealth[i]), horPos, 220+i*20);
                baseh += world.baseHealth[i];
                g2.drawString(Integer.toString(i), 15 + world.tc.baseX[i] * cellSize + 2, 15 + (world.tc.baseY[i]+1) * cellSize-1);
            }
            g2.drawString("Base health = " + baseh, horPos, 195);
            int score = world.totMoney + baseh;
            g2.drawString("Score = " + score, horPos, 225+world.baseHealth.length*20);
            }
        }
    }

    class DrawerWindowListener extends WindowAdapter {
        public void windowClosing(WindowEvent event) {
            PathDefenseVis.stopSolution();
            System.exit(0);
        }
    }

    final Object keyMutex = new Object();
    boolean keyPressed;

    public void processPause() {
        synchronized (keyMutex) {
            if (!stepMode && !pauseMode) {
                return;
            }
            keyPressed = false;
            while (!keyPressed) {
                try {
                    keyMutex.wait();
                } catch (InterruptedException e) {
                    // do nothing
                }
            }
        }
    }

    public Drawer(World world, int cellSize) {
        super();

        panel = new DrawerPanel();
        getContentPane().add(panel);

        addWindowListener(new DrawerWindowListener());

        this.world = world;

        boardSize = world.tc.boardSize;
        this.cellSize = cellSize;
        width = cellSize * boardSize + EXTRA_WIDTH;
        height = cellSize * boardSize + EXTRA_HEIGHT;

        addKeyListener(new DrawerKeyListener());

        setSize(width, height);
        setTitle("Visualizer tool for problem PathDefense");

        setResizable(false);
        setVisible(true);
    }
}

class World {
    final Object worldLock = new Object();
    TestCase tc;
    int totMoney;
    int curStep = -1;
    int numSpawned;
    int numKilled;
    int numTowers;
    int moneyIncome;
    int moneySpend;
    int[] baseHealth;
    List<Tower> towers = new ArrayList<Tower>();
    List<AttackVis> attacks = new ArrayList<AttackVis>();

    public World(TestCase tc) {
        this.tc = tc;
        totMoney = tc.money;
        numSpawned = 0;
        numKilled = 0;
        numTowers = 0;
        moneyIncome = 0;
        moneySpend = 0;
        baseHealth = new int[tc.baseCnt];
        for (int i=0;i<tc.baseCnt;i++)
            baseHealth[i] = 1000;
    }

    public void updateCreeps() {
        synchronized (worldLock) {
            for (Creep c : tc.creeps)
            if (c.health>0 && c.spawnTime<curStep) {
                int dir = c.moves.get(curStep-c.spawnTime-1);
                c.x += Constants.DX[dir];
                c.y += Constants.DY[dir];
                if (tc.board[c.y][c.x]>='0' && tc.board[c.y][c.x]<='9') {
                    // reached a base
                    int b = tc.board[c.y][c.x]-'0';
                    // decrease the health of the base
                    baseHealth[b] = Math.max(0, baseHealth[b]-c.health);
                    c.health = 0;
                }
            } else if (c.spawnTime==curStep) {
                numSpawned++;
            }
        }
    }

    public void updateAttack() {
        synchronized (worldLock) {
            attacks.clear();
            for (Tower t : towers) {
                // search for nearest attackable creep
                int cidx = -1;
                int cdist = 1<<29;
                for (int i=0;i<tc.creeps.length;i++)
                if (tc.creeps[i].health>0 && tc.creeps[i].spawnTime<=curStep) {
                    int dst = (t.x-tc.creeps[i].x)*(t.x-tc.creeps[i].x) + (t.y-tc.creeps[i].y)*(t.y-tc.creeps[i].y);
                    // within range of tower?
                    if (dst<=tc.towerTypes[t.type].range*tc.towerTypes[t.type].range) {
                        // nearest creep?
                        if (dst<cdist) {
                            cdist = dst;
                            cidx = i;
                        } else if (dst==cdist) {
                            // creep with smallest id gets attacked first if they are the same distance away
                            if (tc.creeps[i].id<tc.creeps[cidx].id) {
                                cdist = dst;
                                cidx = i;
                            }
                        }
                    }
                }
                if (cidx>=0) {
                    // we hit something
                    tc.creeps[cidx].health -= tc.towerTypes[t.type].damage;
                    attacks.add(new AttackVis(t.x, t.y, tc.creeps[cidx].x, tc.creeps[cidx].y));
                    if (tc.creeps[cidx].health<=0) {
                        // killed it!
                        totMoney += tc.creepType.money;
                        numKilled++;
                        moneyIncome += tc.creepType.money;
                    }
                }
            }

        }
    }

    public void startNewStep() {
        curStep++;
    }

}

public class PathDefenseVis {
    public static String execCommand = null;
    public static long seed = 1;
    public static boolean vis = true;
    public static boolean debug = false;
    public static int cellSize = 12;
    public static int delay = 100;
    public static boolean startPaused = false;

    public static Process solution;

    public int runTest() {
        solution = null;

        try {
            solution = Runtime.getRuntime().exec(execCommand);
        } catch (Exception e) {
            System.err.println("ERROR: Unable to execute your solution using the provided command: "
                    + execCommand + ".");
            return -1;
        }

        BufferedReader reader = new BufferedReader(new InputStreamReader(solution.getInputStream()));
        PrintWriter writer = new PrintWriter(solution.getOutputStream());
        new ErrorStreamRedirector(solution.getErrorStream()).start();

        TestCase tc = new TestCase(seed);

        writer.println(tc.boardSize);
        writer.println(tc.money);
        // Board information
        for (int y=0;y<tc.boardSize;y++) {
            String row = "";
            for (int x=0;x<tc.boardSize;x++) {
                row += tc.board[y][x];
            }
            writer.println(row);
        }
        // Creep type information
        writer.println(tc.creepType.health);
        writer.println(tc.creepType.money);
        writer.flush();
        // Tower type information
        int[] towerTypeData = new int[tc.towerTypeCnt*3];
        int ii = 0;
        for (int i=0;i<tc.towerTypeCnt;i++) {
            towerTypeData[ii++] = tc.towerTypes[i].range;
            towerTypeData[ii++] = tc.towerTypes[i].damage;
            towerTypeData[ii++] = tc.towerTypes[i].cost;
        }
        writer.println(towerTypeData.length);
        for (int v : towerTypeData)
            writer.println(v);
        writer.flush();

        World world = new World(tc);
        Drawer drawer = null;
        if (vis) {
            drawer = new Drawer(world, cellSize);
            drawer.debugMode = debug;
            if (startPaused) {
                drawer.pauseMode = true;
            }
        }

        for (int t = 0; t < Constants.SIMULATION_TIME; t++) {
            world.startNewStep();

            writer.println(world.totMoney);
            int numLive = 0;
            for (Creep c : world.tc.creeps)
            if (c.health>0 && c.spawnTime<world.curStep) numLive++;

            int[] creeps = new int[numLive*4];
            int ci = 0;
            for (Creep c : world.tc.creeps)
            if (c.health>0 && c.spawnTime<world.curStep) {
                creeps[ci++] = c.id;
                creeps[ci++] = c.health;
                creeps[ci++] = c.x;
                creeps[ci++] = c.y;
            }
            writer.println(creeps.length);
            for (int v : creeps)
                writer.println(v);
            writer.println(world.baseHealth.length);
            for (int v : world.baseHealth)
                writer.println(v);
            writer.flush();

            int commandCnt;
            try {
                commandCnt = Integer.parseInt(reader.readLine());
                if (commandCnt>tc.boardSize*tc.boardSize*3) {
                    System.err.println("ERROR: Return array from placeTowers too large.");
                    return -1;
                }
                if ((commandCnt%3)!=0) {
                    System.err.println("ERROR: Return array from placeTowers must be a multiple of 3.");
                    return -1;
                }
                if (commandCnt>0) {
                    int[] newTowers = new int[commandCnt];
                    for (int i=0;i<commandCnt;i++) {
                        newTowers[i] = Integer.parseInt(reader.readLine());
                    }
                    for (int i=0;i<newTowers.length;i+=3) {
                        Tower newT = new Tower();
                        newT.x = newTowers[i];
                        newT.y = newTowers[i+1];
                        newT.type = newTowers[i+2];
                        if (newT.x<0 || newT.x>=tc.boardSize || newT.y<0 || newT.y>=tc.boardSize) {
                            System.err.println("ERROR: Placement (" + newT.x + "," + newT.y + ") outside of bounds.");
                            return -1;
                        }
                        if (tc.board[newT.y][newT.x]!='#') {
                            System.err.println("ERROR: Cannot place a tower at (" + newT.x + "," + newT.y + ").");
                            return -1;
                        }
                        if (newT.type<0 || newT.type>=tc.towerTypeCnt) {
                            System.err.println("ERROR: Trying to place an illegal tower type.");
                            return -1;
                        }
                        if (world.totMoney<tc.towerTypes[newT.type].cost) {
                            System.err.println("ERROR: Not enough money to place tower.");
                            return -1;
                        }
                        world.totMoney -= tc.towerTypes[newT.type].cost;
                        tc.board[newT.y][newT.x] = (char)('A'+newT.type);
                        world.towers.add(newT);
                        world.numTowers++;
                        world.moneySpend += tc.towerTypes[newT.type].cost;
                    }
                }
            } catch (Exception e) {
                System.err.println("ERROR: time step = " + t + " (0-based). Unable to get the build commands" +
                        " from your solution.");
                return -1;
            }

            world.updateCreeps();
            world.updateAttack();

            if (vis) {
                drawer.processPause();
                drawer.repaint();
                try {
                    Thread.sleep(delay);
                } catch (Exception e) {
                    // do nothing
                }
            }
        }

        stopSolution();

        int score = world.totMoney;
        for (int b=0;b<world.baseHealth.length;b++)
            score += world.baseHealth[b];

        System.err.println("Money = " + world.totMoney);
        System.err.println("Total base health = " + (score-world.totMoney));
	    
        return score;
    }

    public static void stopSolution() {
        if (solution != null) {
            try {
                solution.destroy();
            } catch (Exception e) {
                // do nothing
            }
        }
    }

    public static void main(String[] args) {
        for (int i = 0; i < args.length; i++)
            if (args[i].equals("-exec")) {
                execCommand = args[++i];
            } else if (args[i].equals("-seed")) {
                seed = Long.parseLong(args[++i]);
            } else if (args[i].equals("-novis")) {
                vis = false;
            } else if (args[i].equals("-debug")) {
                debug = true;
            } else if (args[i].equals("-sz")) {
                cellSize = Integer.parseInt(args[++i]);
            } else if (args[i].equals("-delay")) {
                delay = Integer.parseInt(args[++i]);
            } else if (args[i].equals("-pause")) {
                startPaused = true;
            } else {
                System.out.println("WARNING: unknown argument " + args[i] + ".");
            }

        if (execCommand == null) {
            System.err.println("ERROR: You did not provide the command to execute your solution." +
                    " Please use -exec <command> for this.");
            System.exit(1);
        }

        PathDefenseVis vis = new PathDefenseVis();
        try {
            int score = vis.runTest();
            System.out.println("Score = " + score);
        } catch (RuntimeException e) {
            System.err.println("ERROR: Unexpected error while running your test case.");
            e.printStackTrace();
            PathDefenseVis.stopSolution();
        }
    }
}

class ErrorStreamRedirector extends Thread {
    public BufferedReader reader;

    public ErrorStreamRedirector(InputStream is) {
        reader = new BufferedReader(new InputStreamReader(is));
    }

    public void run() {
        while (true) {
            String s;
            try {
                s = reader.readLine();
            } catch (Exception e) {
                //e.printStackTrace();
                return;
            }
            if (s == null) {
                break;
            }
            System.err.println(s);
        }
    }
}

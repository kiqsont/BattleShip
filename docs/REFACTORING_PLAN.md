# BattleShip 重构计划

> 目标：**先整理结构，再谈功能**。本次重构的唯一成功标准是——下一个改动它的人（包括半年后的你）能在 5 分钟内找到"该改哪个文件"，并且改一处不会连带崩掉三处。
>
> 范围：结构、分层、命名、数据模型、构建。**不改玩法、不加联网/AI、不换美术、不做存档**。
>
> 现状基线：Qt 5.14.2 / MinGW 64-bit / qmake，7 个类，手写代码约 1560 行（不含 `ui_*.h` 自动生成代码 1288 行）。

---

## 一、现状体检

### 1.1 代码地图

| 文件 | 行数 | 实际职责 | 问题定性 |
|---|---|---|---|
| `widget.cpp/h` | 154 | 主菜单 + **全局状态持有者** + 界面跳转调度 | 职责严重超标 |
| `configuration.cpp/h` | 537 | 布置界面 + **舰船摆放/删除规则** + 字符串解析 + 提示弹窗 | 逻辑与界面粘连 |
| `game.cpp/h` | 543 | 对战界面 + **回合/瞄准/齐射/胜负规则** + 200 个控件手工创建 | 一切塞进构造函数 |
| `button.cpp/h` | 174 | 棋盘格子按钮（状态机 + 贴图） | 命名与职责尚可，被大量外部字段直写 |
| `label.cpp/h` | 46 | ①布置界面格子 ②对战界面的**空信号发射器** | 一类别两用，第二种用法是 hack |
| `rulescene.cpp/h` | 98 | 规则图片翻页 | 基本健康 |
| `main.cpp` | 11 | 入口 | 健康 |

### 1.2 结构性问题（按严重度）

**A. 没有模型层——规则数据是一堆裸露的 `int[10][10]`**

`Player1Array`、`Player2Array`、`seaArray` 全是 public 裸数组，跨对象直接拷贝：

```cpp
// widget.cpp:54  —— 主菜单直接伸手进 Game 的成员变量赋值
game->Player1Array[i][j] = Player1Array[i][j];
// widget.cpp:86  —— 又伸手进 Configuration 的成员变量赋值
Player1Array[i][j] = player1->seaArray[i][j];
```

同一个 10×10 数组在三个类之间流窜，语义完全不同（布置阶段是"船体编码"，对战阶段是"待击目标"），却共用一个 `int`。没有任何一个地方能回答"这艘船现在还剩几格"。

**B. "一号玩家 / 二号玩家"靠字段名后缀复制一遍**

`seaButton1/seaButton2`、`togeShoot1/2`、`choic1/2`、`Shoot1/2`、`time1/2`、`full1/2`、`hit1/2`、`times1/2`——8 组平行字段，导致 `game.cpp` 里出现约 250 行几乎逐字重复的代码（瞄准/取消/发射/满员检查各写两遍）。改一条规则必须改两处，这正是当前 bug 的温床（见 1.4）。

**C. 构造函数即全部逻辑**

`Game::Game()` 从第 8 行到第 264 行，一个函数里干了：初始化 8 组状态、创建 20 个标签按钮、连线 6 个 lambda。真正的棋盘创建却放在**另一个**"构造函数" `install()` 里，而且由外部 `Widget` 手工调用——于是它被调用了几次就有几套控件（见 1.4 的 P0-1）。

**D. 界面之间靠 `hide()/show()` + 复制 geometry 硬连**

```cpp
instruction->setGeometry(this->geometry());   // 出现 6 次，逐窗口手工同步
this->hide(); instruction->show();
```

主菜单同时 `new` 了 `Game`、两个 `Configuration`、`RuleScene` 四个窗口并持有指针，谁也不能独立存在。窗口尺寸还不一致：主菜单/布置/规则是 460×740，对战窗口 `setFixedSize(480,900)`，跳转时靠 `setGeometry` 强行纠正，实际被 fixedSize 覆盖。

**E. 数据编码是"魔法数字叠加"**

`configuration` 里一个格子值同时编码三件事：

| 值 | 含义 |
|---|---|
| `0` | 空 |
| `1..4` | 船体，值 = 船长 |
| `type + 4` | 船头朝上（如战舰 8） |
| `type + 8` | 朝下（12） |
| `type + 12` | 朝左（16） |
| `type + 16` | 朝右（20） |

`button.cpp` 里用 `index % 4` 反推船型（战舰 4%4==0，靠"默认分支"兜住），用 `(v-1)/4` 反推方向。**而方向编号有两套互不兼容的约定**：

- 放置时 `handleDirection(d)`：`0=上, 1=下, 2=左, 3=右`
- 删除时 `DeleteShip(direction)`：`1=上, 2=下, 3=左, 4=右`

同一个 `1`，在放置里是"下"，在删除里是"上"。这不是理论风险，是随时会踩的坑。

**F. 命名问题**

`button`、`label` 这类小写类名与 Qt 习惯势力冲突（`QLabel`/`QPushButton` 就在旁边）；`togeShoot`（togetherShoot？）、`choic`（choice）、`tranformNum`（transform）、`ConfigFinsh`、`MainWindowBack`、`SetShipShang/Xia/Zuo/You`（中英混杂）——全部需要收口。

**G. 工程卫生**

`Makefile`、`Makefile.Debug/Release`、`object_script.*`、`ui_*.h`、`debug/`、`release/`、`.qmake.stash`、4 个 `*.pro.user*` 全部处于 git 暂存状态（**仓库尚无任何 commit**）。`Image/GameBackground.png` 未被 `res.qrc` 引用。无 `.gitignore`、无测试、无格式化配置。

### 1.3 目标分层

```
┌─────────────────────────────────────────────┐
│  ui/          只做"显示"和"把点击转成意图"    │
│   MainWindow / PlacementView / BattleView    │
│   RulesView / widgets/{CellButton,BoardWidget}│
└───────────────┬─────────────────────────────┘
                │ 调用方法 / 接收信号，不碰裸数组
┌───────────────▼─────────────────────────────┐
│  core/        纯 C++（仅依赖 QtCore），可单测  │
│   Board / Ship / GameSession / Rules / Types │
└─────────────────────────────────────────────┘
```

核心约束：**`core/` 里不许出现 `#include <QWidget>`、不许弹 `QMessageBox`**。规则错误通过返回值（`PlacementError`）上报，由 UI 决定怎么提示。这一条决定了以后能不能写测试、能不能换界面。

### 1.4 已知缺陷（重构时必须顺手修掉）

| 编号 | 位置 | 现象 | 严重度 |
|---|---|---|---|
| P0-1 | `widget.cpp:57` + `game.cpp:266` | 每次点"开始"都调 `game->install()`，重复 `new` 200 个按钮并重复连线。点 3 次 = 600 个控件、3 倍信号回调 | 高 |
| P0-2 | `game.cpp:324-333` | 玩家 2 棋盘的 `beShoot/calShoot` 连到了玩家 1 的 `btn`，导致选格计数错乱 | 高 |
| P0-3 | `game.cpp:249` | `times2->setNum(togeShoot1)` 应为 `togeShoot2`（复制粘贴残留） | 中 |
| P0-4 | `game.cpp:24-25` + `label.cpp:11` | `label(1)/label(2)` 不初始化任何东西，只用来 `emit full1->fullcheck()`——借用别人的信号，是纯粹的状态耦合 | 中 |
| P0-5 | `game.cpp:32` vs `widget.cpp:15` | 480×900 vs 460×740，跳转时窗口尺寸/位置跳变 | 中 |
| P0-6 | `configuration.cpp:428-432` | `DeleteShip` 在 `type==1` 时提前 return，未清空 `delX/delY` 输入框，与其它分支不一致 | 低 |
| P0-7 | `button.cpp:93-104` | `shootPix()` 未做 `scaled()`，与其它贴图尺寸不一致 | 低 |
| P0-8 | `configuration.cpp:238` | 方向输入非法时静默按"上"处理，无任何反馈 | 低 |
| P0-9 | 全局 | `Game`/`Configuration`/`RuleScene` 均 `new` 后无 parent、永不释放 | 低 |

---

## 二、目标结构

```
BattleShip/
├── CMakeLists.txt
├── .gitignore
├── .clang-format
├── src/
│   ├── main.cpp
│   ├── core/                     # 纯逻辑，无 Widget
│   │   ├── Types.h               # enum class Side / ShipType / Direction / Coord
│   │   ├── Rules.h               # 棋盘尺寸、舰队编成、齐射消耗 —— 唯一常量来源
│   │   ├── Ship.h/.cpp           # 单舰：长度、朝向、命中数、是否沉没
│   │   ├── Board.h/.cpp          # 10×10 网格、放置/校验/删除/命中
│   │   └── GameSession.h/.cpp    # 回合、瞄准、齐射、胜负、阶段状态机
│   └── ui/
│       ├── MainWindow.h/.cpp     # QStackedWidget 导航，替代 4 个窗口互 hide/show
│       ├── PlacementView.h/.cpp  # 替代 Configuration
│       ├── BattleView.h/.cpp     # 替代 Game
│       ├── RulesView.h/.cpp      # 替代 RuleScene
│       └── widgets/
│           ├── CellButton.h/.cpp # 替代 button
│           └── BoardWidget.h/.cpp# 10×10 棋盘控件（每侧一个，共 2 个实例）
├── resources/ (res.qrc + Image/)
└── tests/
    ├── tst_board.cpp
    └── tst_gamesession.cpp
```

### 2.1 数据模型：从"魔法整数"到"船是对象"

**现在**：一个格子的值同时表示"有没有船/什么船/朝哪"，删除全靠反算。

**改成**：

```cpp
// core/Types.h
enum class Side      { Player1, Player2 };
enum class ShipType  { Submarine, Destroyer, Cruiser, Battleship };
enum class Direction { Up, Down, Left, Right };
struct Coord { int row; int col; };

// core/Ship.h
struct Ship {
    ShipType  type;
    int       length;
    Coord     head;
    Direction dir;
    int       hitCount = 0;
    bool isSunk() const { return hitCount >= length; }
    bool occupies(Coord c) const;   // 一行判断，不再需要方向算术
};

// core/Board.h —— 唯一的棋盘真相
class Board {
public:
    void reset();
    PlacementError place(ShipType, Coord head, Direction);  // 返回错误码，不弹窗
    bool  removeShipOccupying(Coord);      // 按"点到了哪一格"删除，替掉 4 个 Delete 函数
    ShotResult shoot(Coord);               // Miss / Hit / Sunk / AlreadyHit
    CellView   view(Coord) const;           // 给 UI 的只读视图 {是否有船, 是否命中, 是否船头}
    bool  allSunk() const;
    int   remainingShips(ShipType) const;   // 直接替掉 Bship/Cship/Dship/Sship 四个计数器
private:
    std::array<Ship, 9>   ships_;
    std::array<Cell, 100> cells_;           // Cell { int shipIndex = -1; bool hit; }
};
```

收益：`0/1-4/5-8/9-12/13-16/17-20` 编码、`index%4`、`(v-1)/4`、`SetShipShang/Xia/Zuo/You`、`DeleteS/X/Z/Y` **全部消失**；删除逻辑从 4 个函数 + 1 个分派函数变成 1 个函数；剩余数从 4 个手工维护的计数器变成一次查询。

### 2.2 对战状态：从 8 组布尔/整数到状态机

```cpp
// core/GameSession.h
enum class Phase { Placing, Aiming, Resolving, Finished };

class GameSession : public QObject {
    Q_OBJECT
public:
    void beginBattle(const Board& p1, const Board& p2);
    Side currentSide() const;
    Phase phase() const;
    int  volleyBudget(Side) const;      // 替 togeShoot1/2
    int  selectedCount(Side) const;     // 替 choic1/2
    int  volleyCost(Side) const;        // 替 time1/2
    bool toggleSelect(Coord);           // 替 Shoot/ShootTime/full 三态
    void fireVolley();                  // 替 shoot1/shoot2 两段 lambda
signals:
    void selectionChanged(Side);
    void cellsRevealed(Side, QVector<Coord>);
    void turnChanged(Side);
    void finished(Side winner);
};
```

`full1/full2` 那个 `label` 信号 hack 自然消失——"满员"只是 `selectedCount == 0` 的派生状态，不该是一个对象。

### 2.3 界面：一个主窗口 + 两个 BoardWidget

```cpp
// ui/widgets/BoardWidget.h
class BoardWidget : public QWidget {
    Q_OBJECT
public:
    void setBoard(const Board* board);          // 只读渲染
    void setMode(BoardMode);                    // Placement / Target / ReadOnly
    void refresh();                             // 全量重绘（100 格，代价可忽略）
signals:
    void cellClicked(Coord);
};
```

`configuration.cpp` 里 100 个 `new label` + `move(30+j*40, 30+i*40)`、`game.cpp` 里 200 个 `new button` + `move(...)`，全部收敛成 `BoardWidget` 内部一个 `QGridLayout` + 100 个 `CellButton`。坐标映射只写一次（`row = y/40, col = x/40` 这类算术不再散落在业务代码里）。

导航改成：

```cpp
MainWindow (QStackedWidget)
  ├── page 0: 主菜单
  ├── page 1: 布置界面（Player1 / Player2 复用同一个实例）
  ├── page 2: 对战界面
  └── page 3: 规则说明
```

`hide()/show()` 与 `setGeometry(other->geometry())` 全部删除；窗口尺寸统一在 `Rules.h` 的 `kWindowSize` 里定义一次。

---

## 三、执行阶段

原则：**每个阶段结束都必须能编译、能跑、能玩**。任何时候不允许出现"改到一半跑不起来"的长期分支。

### P0 · 建立基线（先做，半天内）— 不改一行业务代码

1. 新增 `.gitignore`：`*.pro.user*`、`Makefile*`、`object_script.*`、`ui_*.h`、`debug/`、`release/`、`build*/`、`.qmake.stash`、`*.o`
2. `git rm --cached` 已暂存的构建产物，提交第一个 commit（当前仓库 **0 个 commit**，这是最大的隐患——改坏了无法回退）
3. 打 tag `v0-legacy`，作为行为对照基准
4. 手写 6 条 smoke 用例（纯文本清单）：布置 4 种船各一艘 → 删除一艘 → 返回 → 开始 → 瞄准齐射 → 一方全灭 → 回主菜单
5. 补上 `Image/GameBackground.png` 到 `res.qrc` 或从仓库删除（当前是游离资源）

**验收**：干净 clone → `qmake && mingw32-make` 出 exe；6 条用例全过；`git status` 干净。

### P1 · 抽出 core 层（最大收益的一步）

1. 新建 `src/core/`，实现 `Types.h`、`Rules.h`、`Ship`、`Board`
2. `Board` 只依赖 QtCore，错误一律返回 `PlacementError` 枚举
3. 写 `tests/tst_board.cpp`：越界、重叠、贴边（第 0 行朝上、第 9 列朝右）、沉没判定、删除后续可再放置、剩余数统计
4. `Configuration` 内部改用 `Board`，把 `seaArray` 从类里彻底删除；4 个 `SetShipXxx` + `handleDirection` + 4 个 `DeleteXxx` 替换为 `board.place()` / `board.removeShipOccupying()`；`QMessageBox` 挪到 UI 层，负责把 `PlacementError` 翻译成中文提示
5. 同时修 P0-6、P0-8（删除不清输入框、非法方向静默）

**验收**：单测全绿；`grep -r "seaArray" src/` 无结果；布置界面行为与 `v0-legacy` 逐项一致。

### P2 · 棋盘控件统一

1. `CellButton`（替代 `button`）：状态从 6 个 public bool（`Shoot/full/ShootTime/Turn/Close/...`）收敛为 `enum class CellState { Empty, Hovered, Hidden, Miss, Hit, Selected, Blocked }` + `setState()`；外部不再直接写字段，只调方法
2. `BoardWidget`（替代两处 10×10 手工循环）：`QGridLayout` + 信号 `cellClicked(Coord)`
3. `Configuration` 与 `Game` 里的 `new`/`move`/`setParent` 全部删除
4. 顺手修 P0-7（`shootPix` 未缩放）

**验收**：`grep -c "new button\|new label" src/` 归零；两个界面渲染与旧版像素级一致（截图比对）。

### P3 · 对战逻辑进 GameSession + 修 P0 缺陷

1. `GameSession` 承载回合、预算、选格、齐射、胜负、阶段
2. `BattleView` 只保留：渲染 + 把 `cellClicked` 转成 `session.toggleSelect()` + 响应 session 信号
3. 删除 `label(1)/label(2)`（P0-4）
4. **修 P0-1**：`BattleView` 构造时建一次棋盘，开始新局走 `session.beginBattle()` 重置，不再 `install()`
5. **修 P0-2、P0-3**
6. 写 `tests/tst_gamesession.cpp`：回合轮转、预算耗尽、全灭判胜、重复点同一格

**验收**：连点"开始" 3 次，控件数不变（用 `findChildren<CellButton*>().size()` 断言）；玩家 2 选格计数正确；胜负后可回主菜单再开一局。

### P4 · 导航与主窗口

1. `MainWindow` + `QStackedWidget`，四个 page
2. 删除 `Widget` 里的 4 个窗口指针与 6 处 `setGeometry` 复制
3. `PlacementView` 改为接受 `Board&`，对外只发 `placementFinished()`；主菜单不再需要 public 数组
4. 统一窗口尺寸（P0-5）、给 `new` 出来的对象指定 parent（P0-9）

**验收**：任意跳转窗口不跳变；`grep "Player1Array\|Player2Array"` 无结果；`Widget` 类名消失。

### P5 · 工程化固化

1. `CMakeLists.txt`（`qt_add_executable` + `enable_testing()`），保留 `.pro` 一个版本用于过渡
2. clang-format（Qt 风格）+ 命名规范文档 + 提交前钩子
3. CI：`cmake --build` + `ctest`
4. **（可选，独立分支）** Qt 5.14 → Qt 6.5 LTS：`QMatrix` → `QTransform`、`enterEvent(QEvent*)` → `QEnterEvent*`、`QVector` → `QList`。这一步与上面所有阶段解耦，做不做都不影响结构收益

**验收**：一条命令构建 + 跑测试；新克隆环境 5 分钟内可编译。

---

## 四、命名规范（P1 起逐步套用）

| 现在 | 改为 | 说明 |
|---|---|---|
| `button` | `CellButton` | 类名 PascalCase，禁止与 Qt 类名撞车 |
| `label`（布置界面格子） | 由 `BoardWidget` 内部管理 | 不再单独成类 |
| `label(1)` / `label(2)` | 删除 | 空信号发射器 |
| `Configuration` | `PlacementView` | 后缀表达角色（View/Model/Controller） |
| `Game` | `BattleView` | |
| `Widget` | `MainWindow` | |
| `RuleScene` | `RulesView` | `Scene` 在 Qt 中特指 Graphics View，此处误用 |
| `togeShoot` | `volleyBudget` | 猜测原意 togetherShoot；若为"齐射预算"可再议 |
| `choic` | `selectedCount` | |
| `time1/time2` | `volleyCost` | 单位是"格数"，不是时间 |
| `turn` | `GameSession::currentSide()` | |
| `WinCheck` | `GameSession::checkVictory()` | |
| `tranformNum` / `tranformStr` | `directionFromInput` / `shipTypeName` | 拼写 + 职责单一 |
| `ConfigFinsh` | `placementFinished` | 拼写 |
| `MainWindowBack` | `backRequested` | 信号用过去式/请求式 |
| `SetShipShang/Xia/Zuo/You` | `Board::place(head, Direction)` | 方向是数据，不是函数名 |
| `seaArray` / `PlayerXArray` | `Board` | |

其它约定：
- 成员变量 `m_camelCase`，常量 `kCamelCase`，名字里带单位的加后缀（`kCellSizePx`）
- 所有 `enum` 用 `enum class`，禁止裸 `int` 传状态
- UI 层禁止 `#include` 另一个 View，跨页跳转只通过 `MainWindow`

---

## 五、风险控制

| 风险 | 应对 |
|---|---|
| 逻辑和界面同时改，出问题定位不到 | P1 只动数据层、P2 只动控件、P3 才动逻辑——每阶段独立可验证、可单独提交 |
| 环境不可复现（无 commit、无 .gitignore） | P0 必须先做，`v0-legacy` tag 是唯一的救命绳 |
| Qt 版本迁移牵连大量改动 | 放 P5 且独立分支，与结构重构解耦 |
| 重构把隐藏行为改没了（例：连点开始、重复点击同格） | P3 的 `tst_gamesession` + P0 的 6 条 smoke 用例双保险；每阶段结束对照 `v0-legacy` 手测 |
| 旧功能里有说不清的业务意图 | 遇到语义不明（例：为什么要"点自己棋盘射击"、`index != 0` 当胜利条件）**先记录再问**，不擅自改行为 |

### 无法从代码判定的业务语义（需要你确认）

1. **射击方向**：目前玩家点击的是自己那一侧的棋盘（`seaButton1` / `seaButton2`），命中自己的船才算"击中"。这是"各自攻击自己海域"的玩法设定，还是当年写反了？
2. **预算机制**：翻开一格会让 `togeShoot` **+1**（点击 `flag()` 分支后 `togeShoot1++`），即"打得越多子弹越多"。这是刻意设计还是符号写反？
3. `togeShoot` 到 `volleyBudget` 的替换名，是否准确表达原意？

这三条不影响重构结构，但会影响 P3 的测试断言，建议在 P3 前给个答复。

### 裁剪建议

如果只想快速"变干净"而不想全量重构，最小可行路径是 **P0 + P1 + P2**：先把逻辑从界面里剥出来、把 300 个手工控件收成 2 个组件、删掉魔法数字与重复代码。这三步做完，代码可读性大概能回到正常水平；P3/P4 再解决"能不能加功能"的问题。

---

## 六、进度与验收总表

| 阶段 | 产出 | 不改 UI 行为 | 关键验收 |
|---|---|---|---|
| P0 | `.gitignore`、首个 commit、`v0-legacy` tag、smoke 清单 | ✅ | 可 clean build、可回退 |
| P1 | `core/{Types,Rules,Ship,Board}` + 单测，`seaArray` 消失 | ✅ | `tst_board` 全绿 |
| P2 | `CellButton` + `BoardWidget`，300 个手工控件归零 | ✅ | 像素一致、`new button` 归零 |
| P3 | `GameSession` + 状态机，P0-1~P0-4 修复 | ⚠️ 修 bug（预期） | `tst_gamesession` 全绿、连点开始不增殖 |
| P4 | `MainWindow` + `QStackedWidget` | ✅ | 窗口不跳变、无跨对象裸数组 |
| P5 | CMake + 格式化 + CI（+ Qt6 可选） | ✅ | 一键构建与测试 |

---

## 七、执行记录（2026-11-03）

### 已完成

| 阶段 | 状态 | 结果 |
|---|---|---|
| P0 基线 | ✅ | 新增 `.gitignore`；13 个生成文件停止跟踪；原始代码保留于 tag `v0-legacy`；旧版 qmake 可编译验证通过 |
| P1 core 层 | ✅ | 新增 `src/core/{Types,Rules,Ship,Board,GameSession}`；`seaArray` 与 4 个 `SetShipXxx`、4 个 `DeleteXxx`、`handleDirection`、`tranformNum/Str` 全部消失 |
| P2 控件统一 | ✅ | `button` → `CellButton`（6 个 public bool → `CellVisual` + `setInteractive`）；`BoardWidget` 接管两处 10×10 手工布局；`label` 类删除 |
| P3 对战状态机 | ✅ | `GameSession` 接管回合/预算/选中/胜负；`Game` 的 543 行构造函数改为 `BattleView` 约 200 行纯渲染 |
| P4 导航 | ✅ | `MainWindow` + `QStackedWidget`；`Widget` 的 4 个窗口指针与 6 处 `setGeometry` 复制全部删除；无跨对象裸数组 |
| P5 工程化 | ⚠️ 部分 | CMake（app/ui/core 三层 + 3 个测试目标）完成；`.clang-format` 完成；Qt6 迁移未做（独立分支，见第五节风险表） |

**构建与测试现状**（Qt 5.14.2 / MinGW 7.3 / CMake 3.24）：

```
cmake -G "MinGW Makefiles" -DCMAKE_PREFIX_PATH=<Qt路径> -B build
cmake --build build
ctest --test-dir build          # tst_board 10 项 + tst_gamesession 8 项 + tst_ui 5 项，全绿
```
`.pro` 保留可用（qmake + mingw32-make 已验证）。

### 与计划的偏差（均已在代码中注明）

1. **窗口尺寸统一为 480×900**（原为 460×740 与 480×900 混用）。主菜单/布置/规则页内容居中，对战页铺满。
2. **删除改为"整艘移除"**。旧版点船体格只会清掉那一格，剩余船体变成永久占位且无法再删除；现在按"该格属于哪艘船"整体移除。
3. **布置改为"先整体校验再落子"**。旧版边写边校验，中途失败会留下半截船体。
4. **补齐坐标越界校验**。旧版删除/布置只校验行不校验列，输入 15 会写到数组外。
5. **规则页保留 `.ui`**。内容为 7 张图片 + 大量绝对定位文字，重建收益低于风险，仅把类名改为 `RulesView`。
6. **行动条按回合禁用**。旧版非当前回合的"瞄准/启动"仍可点击（点了也没效果），现在直接置灰。
7. **布置输入框增加了占位提示**（行/列/方向），交互方式未变。

### 修复的缺陷

| 编号 | 说明 |
|---|---|
| P0-1 | 控件只创建一次；开新局走 `GameSession::beginBattle()`，连点"开始"不再堆叠控件 |
| P0-2 | 玩家 2 的选中/取消信号接错到玩家 1 的按钮，已修正（由 `Side` 参数消歧） |
| P0-3 | `times2` 显示玩家 1 预算的复制粘贴残留，随状态收敛自然消失 |
| P0-4 | `label(1)/label(2)` 空信号发射器删除，"满员"改为 `volleyRemaining == 0` 派生状态 |
| P0-5 | 窗口尺寸统一 |
| P0-6 | 删除后输入框统一清空 |
| P0-7 | 贴图统一按 40×40 缩放并缓存 |
| P0-8 | 方向输入非法时按默认方向处理，但已加占位提示说明 |
| P0-9 | 所有子控件都有 parent，随主窗口析构 |

### 曾保留、现已在第二轮对齐的旧行为

第一轮重构时这四条按"不明语义先不动"处理，第二轮已依据设计文档 `Battle Ship.docx` 修正，详见下一节。

---

## 八、第二轮：按设计文档对齐规则（2026-11-03）

### 依据

`Battle Ship.docx` 中的规则原文：

| 原文 | 结论 |
|---|---|
| 「一起扫射：攻击五处」「一同扫射：选五个位置」 | 一次扫射固定覆盖 **5 格**，界面输入框填的是**次数** |
| 「每再击沉一艘舰，得一次一起扫射」 | 只有**击沉**才累积次数，命中不算 |
| 「Player选择位置进攻」「若击中，记录并标记战舰类型」 | 攻击的是**对方**海域，胜利判给攻击方 |
| 「一同扫射：击中后不显示具体船只 除非击沉」 | 齐射命中显示通用标记，只有该舰沉没才暴露船型 |

### 改动

1. **攻击目标**（`GameSession::fireAt` / `fireVolley` / `checkVictory`）
   改为攻击 `board(otherSide(attacker))`；`cellChanged` 携带的是棋盘归属方，
   界面按归属方刷新。胜负改为"打光对方舰队 → 攻击方获胜"。
   界面侧 `BattleView::cellInteractive` 只允许点对方海域，自己的棋盘不可交互。

2. **扫射次数模型**（`GameSession`）
   `budget`（以 -1 为哨兵的"预算"）→ `volleyCredits`（从 0 起的"次数"）。
   击沉 +1；落空清零并换手；发射后按次数扣减。

3. **一次扫射 = 5 格**
   `rules::kCellsPerVolleyUnit` → `kCellsPerVolley`；`TurnBar` 文案与 tooltip 明确
   "输入本次使用几次扫射，每次覆盖 5 格"，`requestedUnits` → `requestedVolleys`。

4. **齐射命中贴图**
   `CellView` 增加 `sunk`；`CellVisual` 增加 `sunk`。齐射命中未击沉时显示通用爆点，
   击沉后才显示船型（原来的"潜水艇特例"删除）。

5. **删除框坐标轴统一**
   删除框改为 X = 行、Y = 列，与放置框一致（原实现相反）。

6. **顺带修复**
   齐射选中标记会同时渲染到两块棋盘上（`visualFor` 未判断棋盘归属），已修正。

### 回退点

若某一条与预期不符，可单独回退，互不牵连：

| 想恢复的旧行为 | 改哪里 |
|---|---|
| 攻击自己棋盘 | `GameSession::fireAt` 与 `fireVolley` 里的 `index(otherSide(attacker))` → `index(attacker)`；`BattleView::cellInteractive` 的 `boardOwner == attacker` 判断 |
| 命中即累积次数 | `GameSession::fireAt` 里 `result == ShotResult::Sunk` → `result != ShotResult::Miss` |
| 齐射命中显示船型 | `CellButton::applyIcon` 的 `VolleyHit` 分支 |
| 删除框 X = 列 | `PlacementView::deleteShip` 的 `readCoord(...)` 实参顺序 |

### 测试

第二轮后共 3 个测试目标、27 项断言：`tst_board` 10 项、`tst_gamesession` 10 项
（新增攻击方向、击沉才计次、5 格换算、胜负归属）、`tst_ui` 7 项（新增删除坐标轴一致性、
自己的海域不可点击）。

### 仍未处理

- 齐射过程中击沉的舰是否也应累积次数：设计文档未明确，当前沿用旧版（不累积）。
- `resources/Image/GameBackground.png` 仍无引用，待决定补进 qrc 或移除。
- Qt6 迁移（独立分支）。



# BattleShip 项目知识库

> 面向两类读者：**接手的人**（5 分钟找到该改哪个文件）与**大模型**（一次读完即可安全改代码）。
> 本文件是项目的"单一事实来源"概览；历史决策与逐轮变更见 `docs/REFACTORING_PLAN.md`。
>
> 生成时间：2026-11-04 ｜ 基线提交：`79a0926` ＋ 未提交的"布置页拖放改造" ｜ 语言：C++17 + Qt 5 Widgets

---

## 目录

1. [五分钟速览](#1-五分钟速览)
2. [技术栈与运行方式](#2-技术栈与运行方式)
3. [目录结构](#3-目录结构)
4. [分层架构与依赖规则](#4-分层架构与依赖规则)
5. [核心数据模型](#5-核心数据模型)
6. [游戏规则（权威清单）](#6-游戏规则权威清单)
7. [状态机与关键流程](#7-状态机与关键流程)
8. [UI 层结构](#8-ui-层结构)
9. [视觉主题系统](#9-视觉主题系统)
10. [构建与测试](#10-构建与测试)
11. [编码约定](#11-编码约定)
12. [常见改动索引](#12-常见改动索引)
13. [已知问题与技术债](#13-已知问题与技术债)
14. [术语表](#14-术语表)

---

## 1. 五分钟速览

**这是什么**：单机、双人同设备、回合制的"海战棋"（Battleship）桌面游戏。10×10 海域、每方 9 艘舰，轮流点击对方海域开火，先打光对方舰队者胜。带一个"一同扫射"的特殊攻击：每击沉一艘舰获得 1 次扫射额度，1 次额度 = 同时打 5 格。

**架构一句话**：`src/core/` 是纯逻辑（只依赖 QtCore，可单测、不弹窗），`src/ui/` 是纯展示（只渲染状态、把点击翻译成意图），`GameSession` 通过对局信号驱动界面刷新。

**三条不可违反的约束**：

1. `core/` 里禁止 `#include <QWidget>`、禁止 `QMessageBox`。错误一律用枚举返回（`PlacementError` / `ShotResult`）。
2. UI 层不持有游戏规则状态。所有"能不能放 / 还剩几次 / 该谁走"都问 `GameSession` 或 `Board`。
3. 跨页跳转只经由 `MainWindow`（`QStackedWidget`），View 之间禁止互相 `#include`。

**代码规模**：约 5.6k 行（core ≈ 890，ui ≈ 3.9k，tests ≈ 810，main 21）。最大的三个文件是 `RulesView.cpp`（约 590 行，几乎全是排版）、`ShipPalette.cpp`（412 行，侧栏自绘）与 `PlacementView.cpp`（273 行）。

---

## 2. 技术栈与运行方式

| 项 | 值 |
|---|---|
| 语言标准 | C++17（`CMAKE_CXX_EXTENSIONS OFF`） |
| 框架 | Qt 5（`Core Gui Widgets`，最低 5.12；实际开发环境 5.14.2 / MinGW 64-bit） |
| 构建 | CMake ≥ 3.16（主）＋ qmake `.pro`（兼容保留，已验证可用） |
| 测试 | QtTest（`tst_board` / `tst_gamesession` / `tst_ui`，`ctest` 驱动） |
| 资源 | Qt 资源系统 `resources/res.qrc`，前缀 `/`，图片在 `:/Image/...` |
| 外部依赖 | 无。无网络、无数据库、无第三方库 |
| 平台 | Windows 为主（`add_executable(BattleShip WIN32 ...)` 隐藏控制台） |

### 构建命令

```bash
# CMake（推荐）
cmake -G "MinGW Makefiles" -DCMAKE_PREFIX_PATH=<Qt路径> -B build
cmake --build build
ctest --test-dir build --output-on-failure

# qmake（兼容路径，产物在 debug/ 或 release/）
qmake BattleShip.pro && mingw32-make
```

### CMake 目标图

```
battleship_core (STATIC, Qt5::Core)      ← 纯逻辑，含 Types/Rules/Ship/Board/GameSession
        ▲ PUBLIC
battleship_ui   (STATIC, Qt5::Widgets)   ← 全部界面类
        ▲ PRIVATE
BattleShip      (WIN32 可执行)            ← 只有 src/main.cpp + resources/res.qrc
```

测试直接链接 `battleship_core` / `battleship_ui`，因此界面类必须能在无主窗口的情况下被构造（`tst_ui` 使用 `-platform offscreen`）。

---

## 3. 目录结构

```
BattleShip/
├── CMakeLists.txt              # 三层目标 + 测试开关
├── BattleShip.pro              # qmake 兼容构建（源文件清单需与 CMake 手工保持同步）
├── .clang-format               # Qt 风格，列宽 100，4 空格
├── Battle Ship.docx            # 原始设计文档（规则出处，勿删）
├── BattleShip.pro.user*        # Qt Creator 用户配置（已 gitignore）
├── Makefile / Makefile.Debug   # qmake 生成物（已 gitignore）
├── ui_rules.h                  # 已废弃的 .ui 生成头（rules.ui 已删除，文件是残留）
├── src/
│   ├── main.cpp                # 入口：设字族 → 显示 MainWindow
│   ├── core/                   # 纯逻辑层
│   │   ├── Types.h/.cpp        # Side/ShipType/Direction/Coord 枚举与几何
│   │   ├── Rules.h/.cpp        # 棋盘尺寸、舰队编制表、每扫射格数
│   │   ├── Ship.h/.cpp         # 单舰：长度/船头/朝向/命中数
│   │   ├── Board.h/.cpp        # 10×10 棋盘：布置/删除/开火/视图
│   │   └── GameSession.h/.cpp  # 对局状态机（QObject，发出信号）
│   └── ui/
│       ├── MainWindow.h/.cpp   # QStackedWidget 导航 + 纸面背景
│       ├── MenuPage.h/.cpp     # 主菜单（封面 + 4 个胶囊按钮，响应式两套版式）
│       ├── MenuButton.h/.cpp   # 菜单胶囊按钮（自绘）
│       ├── PlacementView.h/.cpp# 布置界面（棋盘 + 舰队侧栏，拖放落子）
│       ├── ShipPalette.h/.cpp  # 侧栏：方向选择 + 4 个可拖拽的舰船条目 + 回收区
│       ├── ShipDrag.h/.cpp     # 拖拽载荷（HullPayload / MIME）+ 舰船贴图渲染
│       ├── BattleView.h/.cpp   # 对战界面（两块棋盘 + 两条行动条 + 分割线）
│       ├── RulesView.h/.cpp    # 规则说明（可滚动纸面手册，6 章节）
│       ├── BoardWidget.h/.cpp  # 10×10 棋盘控件（复用 2 次实例；布置页兼作拖放目标）
│       ├── CellButton.h/.cpp   # 单格按钮 + CellVisual 视图结构（含落点虚影）
│       ├── TurnBar.h/.cpp      # 一侧玩家的行动条
│       ├── Theme.h/.cpp        # 颜色/字体/纸面绘制（唯一视觉来源）
│       ├── GameControls.h/.cpp # ThemeButton + 输入控件样式函数
│       └── Pixmaps.h/.cpp      # 贴图缓存 + 旋转 + 40px 缩放
├── resources/
│   ├── res.qrc
│   └── Image/*.png             # 漫画素材、格子贴图、舰型图标
├── tests/
│   ├── CMakeLists.txt
│   ├── tst_board.cpp           # 8 例：棋盘布置/删除/命中/沉没
│   ├── tst_gamesession.cpp     # 8 例：回合/额度/齐射/胜负
│   └── tst_ui.cpp              # 19 例：导航/布局/拖放落子/方向轮换/点击连通性
├── docs/REFACTORING_PLAN.md    # 重构全过程记录（含旧版缺陷与回退点）
└── build/ debug/ release/      # 构建输出（已 gitignore）
```

> 注：`ui_rules.h`、`Makefile*`、`object_script.*`、`*.pro.user*` 均为生成物/残留，已被 `.gitignore` 排除，不要提交。

---

## 4. 分层架构与依赖规则

```
┌──────────────────────────────────────────────────────────────┐
│  ui/                                                          │
│  MainWindow ──┬── MenuPage ── MenuButton                      │
│               ├── PlacementView ── BoardWidget ── CellButton   │
│               ├── BattleView ── TurnBar / BoardWidget          │
│               └── RulesView                                    │
│  横切：Theme / GameControls / Pixmaps                          │
└───────────────────────────┬──────────────────────────────────┘
                            │ 调用方法 + 接收信号（单向依赖）
┌───────────────────────────▼──────────────────────────────────┐
│  core/                                                        │
│  GameSession ── Board[2] ── Ship[*]                           │
│  常量与枚举：Rules / Types                                     │
└──────────────────────────────────────────────────────────────┘
```

**依赖方向**：`ui → core`（单向）。`core` 不知道 `ui` 的存在。

**通信方式**：

- UI → core：直接调方法（`session.fireAt()`、`board.place()`），参数是值类型，返回值是枚举。
- core → UI：`GameSession` 继承 `QObject`，发出 5 个信号（见 §5.4）。`Board` / `Ship` **不是** QObject，不发信号、不持有指针。

**为什么 `GameSession` 是 QObject 而 `Board` 不是**：只有"对局级"的变化（换手、胜负、某格被揭开）需要广播；棋盘本身是纯数据，视图按需轮询 `view()`。

---

## 5. 核心数据模型

### 5.1 `Types.h` — 值类型与几何

```cpp
constexpr int kBoardSize = 10;              // 棋盘边长（格）
constexpr int kCellCount = 100;

enum class Side { Player1 = 0, Player2 = 1 };          // kSideCount = 2
enum class ShipType { Submarine, Destroyer, Cruiser, Battleship };  // 顺序即长度 1..4
enum class Direction { Up, Down, Left, Right };
enum class ShotResult { Miss, Hit, Sunk, AlreadyResolved };
enum class PlacementError { None, OutOfBounds, Overlap, NoShipAtCoord };

struct Coord { int row = 0; int col = 0; };
```

关键辅助：

| 函数 | 语义 |
|---|---|
| `index(Side)` / `sideOf(int)` | 枚举 ↔ 数组下标（`Side` 的值就是数组下标） |
| `otherSide(Side)` | 取对手 |
| `lengthOf(ShipType)` | 潜水艇 1 / 驱逐舰 2 / 巡洋舰 3 / 战舰 4 |
| `shipTypeName(ShipType)` | 中文名（"潜水艇/驱逐舰/巡洋舰/战舰"） |
| `sideTitle(Side)` | "Player 1" / "Player 2" |
| `step(Coord, Direction, n)` | 沿方向走 n 步，**可能越界，调用方须自行 `isValid()`** |
| `Coord::flatIndex()` | `row * 10 + col`，用于一维数组下标 |

坐标系约定：**row 向下递增，col 向右递增**。`Direction::Up` 是 row 减 1。

`directionFromToken()` / `directionToken()` 是旧版 `x / z / y` 字母输入的兼容层，**当前无调用者**（界面已改为中文下拉框）。

### 5.2 `Ship` — 一艘舰

```cpp
class Ship {
    ShipType  m_type;        // 舰种
    Coord     m_head;        // 船头坐标
    Direction m_direction;   // 朝向
    int       m_hitCount;    // 已被命中格数
public:
    int   length() const;                    // = lengthOf(m_type)
    bool  isSunk() const;                    // m_hitCount >= length()
    std::vector<Coord> cells() const;        // 展开的全部格子（恒为 length() 个）
    Coord cellAt(int offset) const;          // 第 offset 格
    bool  occupies(Coord) const;
    bool  registerHit(Coord);                // 该格不属于本舰时返回 false
    void  clearHits();
};
```

船体不存格子数组，而是**用「船头 + 朝向 + 长度」现算**。这是与旧版最大的区别：旧版把一个 `int` 同时编码船型、朝向、船体（`type + 4/8/12/16`），需要 `%4`、`(v-1)/4` 反推。

### 5.3 `Board` — 一块 10×10 海域

```cpp
struct CellView {                 // 给 UI 的只读快照
    bool occupied, resolved, isHead, sunk;
    ShipType type; Direction direction;
};

class Board {
    std::vector<Ship>        m_ships;     // 已布置的舰
    std::array<int, 100>     m_owner;     // 每格归属的舰下标，-1 = 空
    std::array<bool, 100>    m_resolved;  // 每格是否已被揭开（命中或落空）
};
```

| 方法 | 作用 | 失败表现 |
|---|---|---|
| `canPlace(type, head, dir)` | 整体校验（越界 / 重叠） | 返回 `PlacementError` |
| `canPlaceIgnoring(type, head, dir, ignoreShipAt)` | 同上，但把 `ignoreShipAt` 那艘舰当成已经让开 | 拖动已放置的舰时必须用它，否则挪一格就"撞到自己" |
| `place(type, head, dir)` | 先整体校验再落子（**不会留半截船体**） | 返回 `false` |
| `removeShipAt(coord)` | 按"该格属于哪艘船"**整艘移除**，并前移后续下标 | 空格时 `false` |
| `shipAt(coord)` | 该格所属的舰（`const Ship*`，空格 `nullptr`） | 指针在增删舰后失效 |
| `placedCount(type)` / `remainingToPlace(type)` | 已放数 / 编制剩余数（后者 = `rules::initialCount - placedCount`） | — |
| `shoot(coord)` | 标记揭开；有船则 `registerHit` | `Miss/Hit/Sunk/AlreadyResolved` |
| `allSunk()` | 所有舰均沉没 | **空棋盘返回 `true`**（与旧版一致，见 §13） |
| `sunkCount()` | 已沉没舰数 | — |
| `view(coord)` | 渲染用快照 | 越界返回默认空视图 |
| `clearResolutions()` | 清空揭开状态与命中计数，**保留舰船**（开新局用） | — |
| `clear()` | 全部清空 | — |

**不变量**：`m_owner` 中的下标永远与 `m_ships` 的当前顺序一致——`removeShipAt` 在 `erase` 之后会把所有 `> index` 的下标减 1。

### 5.4 `GameSession` — 对局状态机

```cpp
class GameSession : public QObject {
    enum class Phase { Placement, Battle, Finished };

    std::array<Board, 2>            m_boards;
    std::array<int, 2>              m_volleyCredits;  // 已累积的扫射次数
    std::array<bool, 2>             m_armed;          // 是否处于瞄准态
    std::array<int, 2>              m_target;         // 本次需选格数（= 次数 × 5）
    std::array<int, 2>              m_remaining;      // 还差几格
    std::array<std::vector<Coord>,2> m_selected;      // 已选格子
    Phase m_phase = Phase::Placement;
    Side  m_current = Side::Player1;
};
```

**信号（UI 只依赖这 5 个）**：

| 信号 | 参数 | 触发时机 | 上层响应 |
|---|---|---|---|
| `battleStarted()` | — | `beginBattle()` | 目前无接收者（预留） |
| `turnChanged(side)` | 新行动方 | `beginBattle` / 落空换手 | `BattleView` 全量刷新 |
| `cellChanged(boardOwner, coord)` | **棋盘归属方** + 坐标 | 开火 / 选格 / 取消选格 | 只刷新那一格 |
| `sideStateChanged(side)` | 该侧 | 状态量变化（额度、瞄准、选格） | 刷行动条 + 两块棋盘 |
| `finished(winner)` | 胜者 | 对方舰队全灭 | 弹窗 + 退出对战 |

> `cellChanged` 携带的是**棋盘归属方**，不是攻击方。`BattleView` 必须按归属方索引 `m_boards[]`，旧版正是在这里把玩家 2 的信号连到了玩家 1 的控件上。

**关键方法语义**：

| 方法 | 前置条件 | 行为 |
|---|---|---|
| `beginBattle()` | 任意 | 两块棋盘 `clearResolutions()`；清空额度/瞄准/选中；`current = Player1`；`phase = Battle`；发 3 类信号 |
| `leaveBattle()` | 任意 | 只清运行时状态，`phase = Placement`；**舰船与命中状态保留** |
| `fireAt(attacker, coord)` | `Battle` 且 `attacker == current` 且该格未被揭开 | 打 `board(otherSide(attacker))`；落空 → 额度清零 + 换手；击沉 → 额度 +1；非落空则判胜 |
| `armVolley(attacker, n)` | 有额度、`n > 0`、`Battle`、轮到自己 | `target = min(n, credits) × 5`；`remaining = target`；清空选中 |
| `toggleSelection(attacker, coord)` | 已瞄准 | 已选则取消（`remaining++`），未选且可选则加入（`remaining--`） |
| `isSelectable(attacker, coord)` | — | 已瞄准 + 轮到自己 + 该格在对方棋盘未揭开 + （已选 或 未选满） |
| `fireVolley(attacker)` | 已瞄准 | 扣 `cost = target / 5`；逐格 `shoot`；`hits` 计数；清瞄准态；`hits == 0` → 额度清零 + 换手；无论命中与否都判胜 |
| `volleyCost(side)` | — | `m_target / kCellsPerVolley`（派生值，不单独存） |

`VolleyOutcome { int hits; int cost; }` 用于把"打中几格、花了几次额度"回传给 `TurnBar` 显示。

### 5.5 `Rules.h` — 唯一常量来源

```cpp
struct FleetEntry { ShipType type; int count; };
inline constexpr std::array<FleetEntry, 4> kFleet{{
    {ShipType::Battleship, 1},   // 战舰   长 4
    {ShipType::Cruiser,    2},   // 巡洋舰 长 3
    {ShipType::Destroyer,  4},   // 驱逐舰 长 2
    {ShipType::Submarine,  2},   // 潜水艇 长 1
}};                              // 合计 9 艘，占 20 格

constexpr int kCellsPerVolley = 5;   // 一次"一同扫射"覆盖 5 格
```

改玩法只动这张表：布置界面的剩余数、规则页的编制表都从这里派生。

---

## 6. 游戏规则（权威清单）

规则出处是仓库根目录的 `Battle Ship.docx`，逐轮对齐记录见 `REFACTORING_PLAN.md` 第八节。以下是**代码实际实现**的规则，两者若有出入以本节＋§13 的偏差表为准。

**基础**

- 两块 10×10 海域，双方各自布置 9 艘舰，共占 20 格：战舰 4×1 + 巡洋舰 3×2 + 驱逐舰 2×4 + 潜水艇 1×2。
- 同设备双人轮流操作，没有 AI、没有联机。

**布置**

- 交互是拖放：先在右侧舰队面板选方向，把舰从侧栏拖到海域上松手；海域上已放置的舰可以拖走换位置，拖回侧栏底部的虚线框即整艘收回。
- 方向指**船头朝向**：选「↑ 上」时船头朝上、船体从落点格向下延伸（落点格 = 船头格）。侧栏的文案已经按视觉对调过，见 §8.3.1。
- 拖动过程中棋盘给出半透明虚影（合法为墨灰、非法为朱红），**虚影只改视觉，不动棋盘数据**。
- 每艘舰由「船头坐标 + 朝向」确定；船体沿朝向延伸 `lengthOf(type)` 格。
- 越界或与已有舰重叠则拒绝，**不留下半截船体**；拖动中的非法落点在松手时静默取消（不弹窗，因为虚影已经提示过）。
- 拖动已放置的舰时保持"抓住的那一格"贴在鼠标下：落点坐标 - 抓取偏移 = 新船头。
- 删除以「该格属于哪艘舰」为准，**整艘取回**。
- 点"保存"时的分支：9 艘全部放完 → 确认框（保存/取消）；未放完 → 警告框（是否仍返回主菜单）。两条路都可能离开页面，**代码不强制放满**（见 §13.1 第 3 条）。

**对战**

- 攻击的是**对方**海域。点自己的棋盘没有任何效果（`cellInteractive` 直接返回 `false`）。
- 单发结果三种：
  - **落空** → 该格揭开为 Miss，换手，**本回合累积的扫射次数清零**。
  - **命中** → 该格揭开并显示舰型，**继续进攻，不换手**，**不获得扫射次数**。
  - **击沉** → 该舰所有格都被揭开，攻击方 **+1 次扫射额度**，继续进攻。
- **一同扫射**：每 1 次额度 = 一次同时打 5 格（`kCellsPerVolley`）。额度可累积，可一次用完（受额度上限夹取）。
  - 瞄准后依次点选 `5 × 次数` 个格子，点"开火"发射。
  - 齐射落空（0 命中）→ 额度清零 + 换手；有命中则继续进攻。
  - **扫射命中只显示通用爆点，不暴露舰型；只有该舰被击沉才显示船型**。
- **胜利**：一方把对方舰队全部击沉即胜，判给攻击方。结束后 `phase = Finished`，不再接受任何操作。
- 同一格在一次对局里只会被记录一次，重复点击被 `AlreadyResolved` 拦下。

---

## 7. 状态机与关键流程

### 7.1 页面与阶段

```
             ┌──────────── MainWindow(QStackedWidget) ────────────┐
  启动 ────► │ page0 MenuPage   page1 PlacementView               │
             │ page2 BattleView page3 RulesView                   │
             └────────────────────────────────────────────────────┘

GameSession::Phase:  Placement ──beginBattle()──► Battle ──对方全灭──► Finished
                         ▲                          │                     │
                         └──── leaveBattle() ───────┴──── exitRequested ◄─┘
```

注意：**页面切换与 `Phase` 不是同一件事**。`PlacementView` 只改 `Board`，不切 `Phase`；点"开始"进 `BattleView` 时才调 `beginBattle()` 进入 `Battle`。

### 7.2 一次完整对局（时序）

```
MenuPage            MainWindow        PlacementView        GameSession/Board      BattleView
  │ 点 Player1          │                   │                     │                  │
  ├─placementRequested─►│ setSide(Player1)  │                     │                  │
  │                     ├──setCurrentWidget─►│                     │                  │
  │                     │                   │ 点棋盘/填坐标        │                  │
  │                     │                   ├─board.place()──────►│                  │
  │                     │                   │◄──refreshBoard──────┤                  │
  │                     │  点"保存"(全放完)  │                     │                  │
  │                     │◄─placementFinished┤                     │                  │
  │ 点"开始"             │                   │                     │                  │
  ├─startRequested─────►│──────────────────────────────────────────► startBattle()   │
  │                     │                   │   beginBattle()      │                  │
  │                     │                   │   emit turnChanged ──┼──► refreshAll    │
  │                     │                   │ 点对方格子            │                  │
  │                     │                   │   fireAt(attacker)   │                  │
  │                     │                   │   emit cellChanged ──┼──► 刷新该格      │
  │                     │                   │   emit sideStateChanged ─► 刷新条+棋盘  │
  │                     │                   │   落空 → turnChanged ─► refreshAll      │
  │                     │                   │   全灭 → finished ───┼──► QMessageBox   │
  │                     │◄────────────────────────────────────────── exitRequested    │
```

### 7.3 齐射（扫射）流程

```
击沉 x 艘 → volleyCredits += x
                │
   填"次数 n" → 点"瞄准" → armVolley(n): target = min(n, credits) × 5, remaining = target
                │
       逐格 toggleSelection（remaining 递减，可反选）
                │
   点"开火" ──► fireVolley(): credits -= target/5
                │           逐格 shoot()
                ├─ hits == 0 → credits = 0，换手
                └─ hits  > 0 → 保持行动权
                │
           checkVictory() → 对方全灭则 finished
```

**UI 侧的齐射命中标记**：`BattleView::m_volleyHits[boardOwner]` 记录"本次齐射打过的格子"，`visualFor()` 据此把命中渲染成 `Kind::VolleyHit`（通用爆点）。这个集合在下一次 `startBattle()` 时清空，因此整局中齐射过的格子会一直保持"爆点"样式而不是显示舰型——这正是规则要求的效果。

---

## 8. UI 层结构

### 8.1 `MainWindow`

- `QStackedWidget` 装 4 页；`setMinimumSize(620, 660)` + `resize(660, 900)`，窗口可缩放。
- `wrapCentered(page, 620)` 把布置页包进居中的定宽容器（棋盘 424 + 侧栏 156 + 间距）；对战页**不包裹**，直接铺满（两块 424px 棋盘需要完整宽度）。
- `paintEvent` 只画一次纸面（`theme::paintPaper`），各页面自身透明 → 切页时底色不跳。
- 所有跳转都收在这里：`showMenu` / `startPlacement(side)` / `startBattle()` / `startRules()`。

### 8.2 `MenuPage` + `MenuButton`

- 背景是漫画封面 `StartBackground.png`，按 `resizeEvent` 在**两套版式**间切换：
  - `width < 760`：竖排——封面按宽度铺满贴顶，底部 180px 渐隐进墨色，按钮列压在底部（底距 64）。
  - `width >= 760`：分栏——封面铺满左侧并向右渐隐 140px，按钮面板占右侧 44%（夹取 360~520）。
- 4 个 `MenuButton`：开始 / Player1 / Player2 / 规则说明，高 50px 的胶囊，自绘 `paintEvent`（`clicked` 语义仍来自 `QPushButton`）。
- 信号：`startRequested()`、`placementRequested(Side)`、`rulesRequested()`。

### 8.3 `PlacementView`（拖放式布置）

布局（自上而下）：标题 → 操作提示 → `QHBoxLayout[ BoardWidget（固定 424×424，带 1..10 标号）| ShipPalette（固定 156 宽） ]` → 右下角 `保存` / `返回`。

**没有输入框了**：旧版"填行列 → 点放置 / 填行列 → 点删除"的两行表单已整体删除，布置全部走拖放。

```cpp
bool PlacementView::dropHull(const HullPayload& payload, Coord cell); // 新建与挪动共用
bool PlacementView::returnHull(Coord cell);                          // 拖回侧栏 = 整艘收回
```

| 步骤 | 发生什么 |
|---|---|
| 侧栏按住舰体拖动 | `ShipTile::startDrag()` 起 `QDrag`，MIME 里带 `HullPayload{type, direction, fromBoard=false}`，拖动图是整艘舰的贴图 |
| 经过棋盘 | `BoardWidget` 发 `hullHovered(cell, payload)` → 视图算 `hullHeadForDrop()` 与 `checkDrop()` → `BoardWidget::showDropPreview()` 画虚影 |
| 松手 | `BoardWidget` 发 `hullDropped(cell, payload)` → `dropHull()`；不合法就静默取消（虚影已经红过了） |
| 拖动棋盘上的舰 | `CellButton` 按 `startDragDistance` 后发 `dragRequested` → `BoardWidget::hullDragRequested` → 视图用 `shipAt()` 组 `HullPayload{fromBoard=true, sourceHead, grabOffset}` 起拖动 |
| 松手在棋盘上 | `canPlaceIgnoring(type, head, dir, sourceHead)` 校验 → `removeShipAt(sourceHead)` → `place()` |
| 拖回侧栏虚线框 | `HullReturnZone` 发 `hullReturned(sourceHead)` → `returnHull()` → `removeShipAt()` |

关键点：

- **抓取偏移**：拖动已放置的舰时记住"抓住的是船体第几格"，落点 = 该格 - 偏移，位置不会跳。
- **虚影不写数据**：`BoardWidget` 覆盖前先备份被盖住的 `CellVisual`（`m_previewBackup`），`clearDropPreview()` 原样贴回；`refreshBoard()` 开头先清虚影。
- **`checkDrop()` 是唯一的合法性判断**，虚影颜色和松手结果都问它，UI 里不再各自判一遍越界/相撞/余量。
- `保存` 逻辑不变：全放完 → 确认框（`Save/Cancel`），否则警告框（`Yes/No`）；两者都可能 `emit placementFinished()`。

### 8.3.1 `ShipPalette` + `ShipTile` + `HullReturnZone`

- 侧栏（`kPaletteWidth = 156`）自上而下：`舰队` 标题 → `Player 1 · 已放 n/9` → 发丝线 → `方向 · 点一下向右转` → **一个**方向按钮（显示当前朝向，点一次顺时针转 90°：上 → 右 → 下 → 左 → 上，`ShipPalette::rotateDirection()`，初始「↑ 上」）→ 发丝线 → 四个 `ShipTile` → `HullReturnZone`。
- **文案按"看起来船头朝哪"标，枚举名是反的**：`ShipHead.png` 原图朝上而 `Direction::Down` 为 0°，也就是"船体沿 direction 延伸、船头朝反方向"。所以 `directionLabel()/directionArrow()` 里 Up↔Down、Left↔Right 的文案是对调的：`Down` 显示「↑ 上」、`Up` 显示「↓ 下」、`Right` 显示「← 左」、`Left` 显示「→ 右」。改方向相关代码时先看这段注释，不然很容易又绕回去。
- `ShipTile` 自绘一行：舰名 + `剩 n` + 16px 的舰体贴图（船头在左）+ 朝向小箭头；`remaining == 0` 时整行压暗到 35%、光标变箭头、拖不动。
- 侧栏的剩余数由 `PlacementView::refreshPalette()` 从 `Board::remainingToPlace()` 灌进去，**侧栏不自己记账**。
- `HullReturnZone` 是一个独立的小控件（虚线框），只接受 `fromBoard == true` 的拖放，松手即 `hullReturned`；做成独立控件而不是让整个侧栏接收，是为了"拖到这里是删除"这件事有明确边界。
- 分隔线用 `QPalette::Window = paperDeep` 填色，**不用样式表**：给祖先控件设样式表会下发到所有子控件（会把 1px 细线、悬停底色全刷平）。

### 8.4 `BattleView`

垂直布局（stretch 比例）：

```
[ TurnBar(Player1) | 返回主菜单 ]      stretch 1
[ BoardWidget #0 (可伸缩)        ]      stretch 9
[ 5px 间距 ][ 墨色分割线 2px ][ 5px ]   —
[ BoardWidget #1 (可伸缩)        ]      stretch 9
[ TurnBar(Player2)               ]      stretch 1
```

- 两块棋盘 `setScalable(true)`：去掉所有坐标数字，尺寸交给布局均分；`QLayout::SetNoConstraint` 防止布局反向撑大窗口。
- `boardDivider` 是 `objectName`，被 `tst_ui` 用来断言"分割线上下各留 5px"。
- 无 `QScrollArea`（旧版靠滚动条，现按比例铺满）。

渲染逻辑集中在两个函数：

| 函数 | 作用 |
|---|---|
| `visualFor(boardOwner, coord)` | 决定这一格画什么：未揭开（可选中→`Selected`）／落空→`Miss`／命中→`Hit`（齐射过的格子→`VolleyHit`）／布置阶段不涉及 |
| `cellInteractive(boardOwner, coord)` | 决定这一格能不能点：必须 `Battle` 阶段 + 是对方棋盘 + 未揭开 + （瞄准时还要 `isSelectable`） |

交互入口 `onCellActivated()`：瞄准态 → `toggleSelection()`；否则 → `fireAt()`。

> 选中态只在对方棋盘渲染：`boardOwner != currentSide()` 才判 `isSelected`。旧版曾把选中标记同时画到两块棋盘上。

### 8.5 `TurnBar`

单行紧凑条（最小高 26px），一侧一个：

`Player 1` ｜ `扫射 n 次`（瞄准时变 `已选 n / m`）｜ 次数输入框 ｜ `瞄准` ｜ `开火` ｜ `取消` ｜ `击中 n`

- 按钮可用性：`瞄准` 需要"轮到我 + 未瞄准 + 有额度"；`开火`/`取消` 需要"轮到我 + 已瞄准"。
- `setActive()` 只把标题文字染成朱红，不画整条边框。
- 输入框填的是**次数**，不是格数（tooltip 已写明"每次覆盖 5 格"）。

### 8.6 `BoardWidget` + `CellButton`

`BoardWidget` 用 `QGridLayout`：第 0 行放 10 个列号、第 0 列放 10 个行号、其余 100 格是 `CellButton`。两种尺寸模式：

| 模式 | 尺寸 | 坐标标号 |
|---|---|---|
| 默认（布置页） | `setFixedSize(40×10 + 表头)`，正方形 | 显示 |
| `setScalable(true)`（对战页） | 完全由布局分配，`QSizePolicy::Ignored` | 全部隐藏 |

`CellButton` 自绘 `paintEvent`：把缓存的贴图缩放到当前尺寸绘制 + 画 1px 发丝边框。它**不持有游戏状态**，只持有 `CellVisual`（`kind / ship / direction / sunk / valid`）与 `interactive`、`dragSource` 三个外部写入项：

```cpp
enum class Kind { Empty, Selected, Miss, Hit, VolleyHit, ShipHead, ShipBody, GhostHead, GhostBody };
```

`applyIcon()` 里 `VolleyHit` 分支体现规则："`sunk ? shipIcon(type) : volleyCell()`"——未击沉只给爆点。
悬停只对 `Empty` 且可交互的格子生效（显示 `trace.png`）。

布置页的两个额外能力：

- **落点虚影**：`GhostHead / GhostBody` 用 55% 不透明度画，再叠一层 `ink`（合法，alpha 26）或 `sealRed`（非法，alpha 78）+ 2px 朱红边框。`CellVisual::valid` 只对 Ghost 有意义。
- **接收拖放**：`BoardWidget::setDropTargetEnabled(true)` 会给棋盘和 100 个格子都打开 `acceptDrops`，并给格子装事件过滤器；拖动事件无论落在棋盘还是格子上，都由 `handleDragOver` / `handleDrop` 统一换算成**棋盘坐标**（`coordAt()` 用 `geometry().contains()` 反查，不依赖 `childAt`）后再发信号。这样不依赖 Qt "拖放事件向父控件冒泡"的具体实现差异。
- **拖走已放置的舰**：被标记为 `dragSource` 的格子把按下事件 **accept 掉**（不能 ignore，否则按下会转交父控件、后续 mouseMove 也归父控件，拖拽起不来），移动超过 `startDragDistance()` 才发 `dragRequested`。

### 8.7 `RulesView`

可滚动纸面手册：固定页眉（64px 高：`←` 返回 + "规则手册" + `9 SHIPS · 100 CELLS`）+ `QScrollArea` 正文。

正文结构：卷首（48px "玩法说明" + 导语 + 插图）→ 六个编号章节（01 目标 / 02 舰队编制 / 03 布置海域 / 04 轮流进攻 / 05 一同扫射 / 06 版面图例）→ `— END —`。每个章节的头部是 `1px 细线 + 46px 浅灰编号 + 30px 标题 + 英文小标`，正文段落 16~24px，`tst_ui::rulesPageScrollsThroughAllSections` 会校验 01~06 六个编号齐全且内容可滚动。

实现要点：

- 正文栏限宽 700px，左右边距在 `resizeEvent` / `showEvent` 里按 `(视口宽 - 700) / 2` 重算。
- 插图用 `ScaledImage`（实现 `heightForWidth`，宽度跟随文字栏）；与图标一起用 `CompositionMode_Multiply` 叠印到纸面，白底自动消失。
- 编制表直接读 `rules::initialCount()` 与 `lengthOf()`，改编制表页面自动跟着变。
- 图例直接复用 `pixmaps::` 的游戏内贴图，不用示意图。

---

## 9. 视觉主题系统

设计语言：素材是黑白漫画，因此只保留**三色**——墨（近黑）、纸（暖白）、朱红（少量点缀）。层次靠字号、字重、留白与 1px 细线，不用阴影和粗边框。

### 9.1 颜色（`Theme.cpp` 私有常量，只通过函数暴露）

| 函数 | 值 | 用途 |
|---|---|---|
| `ink()` | `#16130F` | 主文字、深色面板 |
| `inkSoft()` | `#3F3932` | 正文 |
| `muted()` | `#8D857A` | 辅助说明 |
| `paper()` | `#FAF8F2` | 纸面底色 |
| `paperDeep()` | `#D8D1C2` | 大号章节数字、禁用态背景 |
| `sealRed()` | `#B0382A` | 朱红，仅点缀（悬停、主按钮、印章） |
| `hairline()` | `#16130F` @ 15% | 发丝线 |

### 9.2 字体

- `sansFamily()`：按 `Microsoft YaHei UI → Microsoft YaHei → PingFang SC → Noto Sans CJK SC → Source Han Sans SC` 顺序探测系统已装字体，`QFontDatabase` 查表；`main.cpp` 里设为全局字族。
- `monoFamily()`：`Consolas → JetBrains Mono → Menlo → DejaVu Sans Mono`。
- 工厂：`displayFont(px)`（加粗 + 字距 1.6）、`titleFont`、`bodyFont(px, weight)`、`capsFont`（全大写、字距 2.4）、`monoFont`。

### 9.3 两个绘图工具

- `paintPaper(painter, rect)`：竖直三段渐变（`#FDFCF8 → #FAF8F2 → #F6F2EA`），无边框。只在 `MainWindow::paintEvent` 调用一次。
- `drawHeavyText(...)`：把文字转成 `QPainterPath` 后**描边 + 填充**。原因写在注释里：中文在 14~17px 下笔画比拉丁字母细，单纯 `setBold` 仍发虚，补一圈 0.45~0.65px 描边才"实"。

### 9.4 控件样式

- `ThemeButton`：圆角 6、高 30；`Accent::Ink`（纸底墨框，悬停转朱红）/ `Accent::Seal`（常态朱红实底）。只重写 `paintEvent`，`clicked`/焦点/无障碍沿用 `QPushButton`。
- `styleLineEdit` / `styleComboBox` / `styleLabel`：**只对控件自身设样式**，不下发到父容器（否则会误伤 200 个棋盘格按钮）。
- 页面里不应出现硬编码色值；新增颜色请加在 `Theme`。

---

## 10. 构建与测试

### 10.1 测试目标

| 目标 | 链接 | 用例数 | 覆盖内容 |
|---|---|---|---|
| `tst_board` | `battleship_core` | 8 | 布置成功/越界/重叠、整艘删除、Miss/Hit/Sunk、全沉判定、编制剩余数、删后重放 |
| `tst_gamesession` | `battleship_core` | 8 | 攻击对方棋盘、落空换手、命中不计次、击沉计次、5 格换算、选满上限、胜负归属、重开清空 |
| `tst_ui` | `battleship_ui` + `res.qrc` | 19 | 菜单构建、窗口可缩放、菜单↔规则往返、六章节齐全且可滚动、侧栏满编、拖放落子、重叠落点被拒、悬停虚影、拖动挪舰（含抓取偏移）、拖回侧栏收回、格子拖动请求、方向按钮轮换（上→右→下→左）、点击落到 session、分割线间距、棋盘随窗缩放、布置页保持定宽、返回信号、按钮文案、重开不复制控件 |

`tst_ui` 通过 `QT_QPA_PLATFORM=offscreen` 运行，不需要真实显示器。它不校验像素，只校验"交互有没有正确落到 core 上"与布局关系。

### 10.2 值得记住的测试断言

- `battleCellClickReachesSession`：自己棋盘 `isInteractive() == false` —— 守住"只能打对方"这条规则。
- `restartingBattleDoesNotDuplicateCells`：`findChildren<CellButton*>().size() == 200` 且连点三次不变 —— 守住旧版 P0-1（重复 `install()` 堆控件）。
- `placementDropPlacesShip` / `placementRejectsOverlappingDrop`：把 `QDragEnterEvent` + `QDropEvent` 直接发给**格子**（和真实拖拽一致），验证落子与"不留半截船体"；侧栏剩余数也要跟着变。
- `placementHoverShowsGhostPreview`：拖动经过时格子是 `GhostHead/GhostBody`，`placedCount` 仍为 0，`QDragLeaveEvent` 后恢复 `Empty` —— 守住"虚影只改视觉"。
- `placementBoardDragMovesShip`：抓第 2 格拖到 (5,5)，船头应落在 (5,4) —— 守住抓取偏移的换算。
- `placementCellDragCanStart`：按下 + 移动超过阈值才发 `hullDragRequested`，且按下事件必须被 accept（否则移动事件归父控件）。
- `battleBoardsSeparatedByDivider`：`boardDivider` 上下间距相等且 ≤ 10px，且两块棋盘里所有 `QLabel` 都 `isHidden()`。

---

## 11. 编码约定

来自 `.clang-format`（Qt 风格，列宽 100，4 空格，不吃 tab）与 `REFACTORING_PLAN.md` 第四节：

| 约定 | 说明 |
|---|---|
| 命名空间 | `bs::` / `bs::ui::` / `bs::rules::` |
| 类名 | PascalCase，禁止与 Qt 类名撞车 |
| 成员 | `m_camelCase` |
| 常量 | `kCamelCase`，带单位的加后缀（`kCellSizePx`、`kCellsPerVolley`） |
| 枚举 | 一律 `enum class`，禁止裸 `int` 传状态 |
| 信号 | 过去式/请求式（`backRequested`、`placementFinished`、`turnChanged`） |
| 头文件保护 | `BS_CORE_BOARD_H` / `BS_UI_BATTLEVIEW_H` |
| 大括号 | 类/函数/枚举体换行，控制语句同行（`AfterControlStatement: false`） |
| 注释语言 | 中文，且倾向解释"为什么"而不是"是什么"（现有代码风格如此，新代码请保持） |

---

## 12. 常见改动索引

| 我想改… | 改这里 |
|---|---|
| 舰队编制（几种船、每种几艘） | `core/Rules.h` 的 `kFleet`（界面与规则页会自动跟随） |
| 舰船长度 | `core/Types.cpp` 的 `lengthOf()` |
| 棋盘尺寸 | `core/Types.h` 的 `kBoardSize`（`kCellCount`、`Coord::flatIndex()`、`BoardWidget` 的格数组与循环都从这里派生；改完请跑 `tst_ui` 的体积/布局断言） |
| 一次扫射覆盖几格 | `core/Rules.h` 的 `kCellsPerVolley` |
| 回合/换手/胜负规则 | `core/GameSession.cpp`：`fireAt` / `fireVolley` / `checkVictory` |
| 落空是否清零额度 | `GameSession::fireAt` 里 `if (result == ShotResult::Miss)` 分支 |
| 击沉才计次 → 命中即计次 | `GameSession::fireAt` 里 `result == ShotResult::Sunk` → `result != ShotResult::Miss` |
| 齐射命中是否显示舰型 | `CellButton::applyIcon()` 的 `VolleyHit` 分支（`sunk ? shipIcon : volleyCell`） |
| 某格贴图 | `ui/Pixmaps.cpp` 对应函数 + `resources/Image/` + `res.qrc` |
| 格子视觉状态种类 | `CellButton.h` 的 `CellVisual::Kind` + `BattleView::visualFor()` |
| 颜色 / 字体 | `ui/Theme.cpp`（唯一来源） |
| 主菜单版式与断点 | `ui/MenuPage.cpp`：`kWideThreshold = 760`、`kPanelShare = 44` |
| 窗口最小尺寸/默认尺寸 | `MainWindow` 构造函数 |
| 规则页文案与章节 | `ui/RulesView.cpp` 的 `buildContent()` |
| 布置页布局（棋盘 + 侧栏） | `ui/PlacementView.cpp` 构造函数 |
| 侧栏的舰队条目 / 方向按钮 / 回收区 | `ui/ShipPalette.cpp`：`ShipTile::paintEvent` / `kPaletteWidth` / `rotateDirection()` / `HullReturnZone` |
| 方向文案（为什么 Up 显示「下」） | `ui/ShipPalette.cpp` 的 `directionLabel()` / `directionArrow()`：船头贴图与枚举相反，文案按视觉对调 |
| 拖动时带什么数据、舰船拖拽图怎么画 | `ui/ShipDrag.cpp`：`HullPayload` / `makeHullMime` / `renderHull` / `hullHeadForDrop` |
| 虚影长什么样 | `CellButton.cpp`：`kGhostOpacity`、`kGhostTintOk/Bad`、`paintEvent` 的 Ghost 分支 |
| 拖动能不能落在这里 | `PlacementView::checkDrop()`（唯一入口，虚影与松手共用） |
| 新增一页 | `MainWindow` 里 `new` 页面 + `m_stack->addWidget` + 一个 `startXxx()` |
| 新增 core 文件 | 同时改 `CMakeLists.txt` 与 `BattleShip.pro`（两处都要加，容易漏） |

---

## 13. 已知问题与技术债

### 13.1 真实缺陷 / 不一致（建议修）

| # | 位置 | 问题 |
|---|---|---|
| 1 | `Board::allSunk()` + `GameSession::fireAt()` | **空棋盘返回 `true`**（沿用旧版语义），但 `fireAt` 只在"非落空"时才 `checkVictory`。于是若一方一艘舰都没放就开战，对方每一发都是 Miss、永远不判胜，对局卡在无限换手里（唯一会无条件判胜的是 `fireVolley`，而齐射额度又必须先击沉才有）。`PlacementView::commit` 允许"没放完就离开"，所以这条路径可达。建议：`beginBattle` 前校验双方舰队非空，或让 `allSunk()` 对空棋盘返回 `false`。 |
| 2 | `GameSession::fireVolley()` | 齐射过程中击沉的舰**不累积**扫射额度（设计文档只写了"每再击沉一艘舰得一次"，未明确齐射场景）。这是文档化的待定项。 |
| 3 | `GameSession::leaveBattle()` | 只重置运行时状态，不 `clearResolutions()`。中途退出再进"开始"会由 `beginBattle()` 清空，所以当前无害，但语义上不对称。 |
| 4 | `PlacementView::dropHull()` | 落点非法时**静默取消**（只有虚影变红作为提示）。若玩家没注意到颜色，会以为"拖了没反应"。若日后要提示，加一个不打断操作的行内文案比弹窗好。 |

### 13.2 死代码 / 预留接口

| 符号 | 状态 |
|---|---|
| `directionFromToken()` / `directionToken()` | 无调用者（旧版 `x/z/y` 输入的兼容层，界面上已无入口） |
| `GameSession::battleStarted()` | 有 `emit`，无 `connect` |
| `BoardWidget::clearCells()` | 无调用者（`refreshBoard` 逐格覆盖即可） |
| `BoardWidget::setColumnLabelsVisible()` | 无调用者（对战页改用 `setScalable(true)` 一次性隐藏全部标号） |
| `BoardWidget::m_columnLabelsVisible` | 在 `setScalable` 路径下不会被 `setGridLabelsVisible(true)` 恢复，当前无影响 |
| `ShipPalette::direction()` | 无外部调用者（方向随拖拽载荷走，侧栏自己用） |

### 13.3 工程卫生

- 根目录残留 `ui_rules.h`（已删除的 `rules.ui` 的产物）、`Makefile*`、`object_script.*`、4 个 `.pro.user*`。均被 `.gitignore` 排除，但可手动清理。
- `resources/Image/GameBackground.png` **未被 `res.qrc` 引用**，是游离资源。
- `CMakeLists.txt` 与 `BattleShip.pro` 两份源文件清单需手工同步，新增文件容易只改一处。
- Qt6 迁移未做。潜在阻塞点：`enterEvent(QEvent*)`（Qt6 为 `QEnterEvent*`）、`QFont::setWeight(int)`（Qt6 需 `QFont::Weight`）、`QFontDatabase()` 构造函数（Qt6 起为静态类）。
- 无 CI 配置；测试需本地手跑 `ctest`。

---

## 14. 术语表

| 术语 | 代码符号 | 含义 |
|---|---|---|
| 海域 / 棋盘 | `Board` | 一块 10×10 网格，每方一块 |
| 舰 / 船 | `Ship` | 由船头 + 朝向 + 舰种确定的一段连续格子 |
| 船头 | `Ship::head()`、`CellView::isHead` | 舰的起始格，贴图带朝向 |
| 揭开 / 已处理 | `resolved` | 该格已被打过（命中或落空），不能重复打 |
| 命中 | `ShotResult::Hit` | 打到船但该舰未沉 |
| 击沉 | `ShotResult::Sunk` | 该舰所有格都被揭开 |
| 一同扫射 / 齐射 | volley | 特殊攻击：一次覆盖 5 格的批量射击 |
| 扫射次数 / 额度 | `volleyCredits` | 每击沉 1 艘得 1 次，落空清零 |
| 瞄准 | `armed` / `armVolley()` | 进入"选格"状态，准备齐射 |
| 单发 | `fireAt()` | 普通的点一格开火 |
| 换手 | `switchTurn()` | 行动方切换（只在落空/齐射 0 命中时发生） |
| 布置 | placement | 开战前摆放 9 艘舰的阶段 |
| 编制 | `rules::kFleet` | "每种舰几艘"的配置表 |
| 落点虚影 | `CellVisual::GhostHead/GhostBody` | 拖动时画在棋盘上的半透明预演，不写数据 |
| 抓取偏移 | `HullPayload::grabOffset` | 拖动已放置的舰时"抓住的是船体第几格" |
| 拖回 | `HullReturnZone` | 把棋盘上的舰拖回侧栏 = 整艘收回 |

---

## 附：给大模型的建议阅读顺序

1. 本文 §1、§4、§6 —— 建立心智模型。
2. `src/core/Types.h` + `src/core/Rules.h`（约 110 行）—— 全部常量与枚举。
3. `src/core/GameSession.h`（91 行）—— 看接口签名就能懂规则，不必先读 `.cpp`。
4. 需要改规则时再读 `GameSession.cpp`（264 行）与 `Board.cpp`（171 行）。
5. 需要改界面时读对应的 `*View.h`，再按需读 `.cpp`；`RulesView.cpp` 只是排版，通常不需要读。
6. 改完务必跑 `ctest --test-dir build`，`tst_gamesession` 会挡住绝大多数规则回归。

**修改时的红线**：不要在 `core/` 里 `#include <QWidget>` 或弹窗；不要在 `ui/` 里复制一份规则判断；不要绕过 `GameSession` 直接改 `Board` 的命中状态。

# GPAChess 开发指南

> 一款基于 Qt6 + C++ 的 RPG 自走棋游戏，融合塔防与 Roguelike 机制。
> 本文档面向完全不了解项目的开发者，逐一说明各模块的职责、位置与关联。

---

## 1. 项目概况

| 项目 | 说明 |
|------|------|
| 语言 | C++17 |
| UI 框架 | Qt 6.10.2 (Widgets + Graphics View) |
| 构建 | CMake + Ninja |
| 编译器 | MinGW 64-bit (g++) |
| 目标平台 | Windows（理论跨平台） |
| UI 设计方式 | Qt Designer 输出 `.ui` 文件 |

### 构建命令
```bash
cmake --build build --config Debug
```
编译产物在 `build/bin/`。运行时会自动将 `assets/` 整目录复制到 `build/bin/assets/`。

---

## 2. 完整文件结构

```
GPAChess/
├── CMakeLists.txt                 # CMake 配置（目标名 gpachess，链接 Qt6）
├── README.md                      # 产品简介
├── MECHANICS.md                   # 游戏机制文档
├── DEVELOP.md                     # 本文档
├── assets/
│   ├── app.rc                     # Windows 资源文件
│   ├── resources.qrc              # Qt 资源集合
│   ├── battleground/
│   │   └── sand.png               # 战场背景贴图
│   └── logo_frames/               # 开屏动画序列帧
├── src/
│   ├── main.cpp                   # 程序入口（创建 QApplication + MainWindow）
│   ├── entry.cpp                  # ★★★ 主窗口逻辑（最核心的 UI 文件，约750行）
│   ├── entry.ui                   # Qt Designer UI 布局（QStackedWidget 场景切换）
│   ├── logo_player.cpp            # 开屏动画播放器
│   ├── print.cpp                  # 全局日志输出（qDebug + 时间戳）
│   ├── database_manager.cpp       # 图鉴数据库（读取 JSON 配置文件）
│   ├── shop_window.cpp            # 商店窗口（购买棋子）
│   ├── unit_graphics_item.cpp     # 可拖拽单位图形项实现
│   ├── core/
│   │   └── game_manager.cpp       # ★★★ 游戏主循环、回合管理、战斗调度
│   ├── entity/
│   │   ├── allies/
│   │   │   ├── ally_factory.cpp   # 我方行为工厂（behaviorId → 具体类）
│   │   │   ├── alpha/
│   │   │   │   ├── alpha_behavior.h
│   │   │   │   └── alpha_behavior.cpp   # AlphaAlly: 近战，3圈旋转武器挥砍
│   │   │   └── beta/
│   │   │       ├── beta_behavior.h
│   │   │       └── beta_behavior.cpp    # BetaAlly: 远程，发射子弹
│   │   └── enemies/
│   │       ├── enemy_factory.cpp  # 敌方行为工厂
│   │       ├── alpha/
│   │       │   ├── alpha_behavior.h
│   │       │   └── alpha_behavior.cpp   # AlphaEnemy: 近战（逻辑与AlphaAlly对称）
│   │       └── beta/
│   │           ├── beta_behavior.h
│   │           └── beta_behavior.cpp    # BetaEnemy: 远程，无我方单位时攻击塔
│   └── include/
│       ├── mainwindow.h           # MainWindow 类声明
│       ├── state.h                # ★★★ 所有数据结构：实例/属性/配置/DrawCmd/PlayerAssets
│       ├── game_manager.h         # GameManager 类声明 + 部分内联实现
│       ├── logo_player.h          # LogoPlayer 声明
│       ├── print.h                # 全局日志函数声明
│       ├── unit_graphics_item.h   # 可拖拽单位图形项（QGraphicsObject子类）
│       └── entity/
│           ├── ally_behavior.h    # AllyBehavior 虚基类 + createAllyBehavior 工厂声明
│           └── enemy_behavior.h   # EnemyBehavior 虚基类 + createEnemyBehavior 工厂声明
└── design/
    └── logo/                      # Logo 设计稿（Premiere 工程文件）
```

---

## 3. 核心架构总览

```
┌──────────────────────────────────────────────────────────────┐
│                   MainWindow (entry.cpp)                     │
│  UI层：场景切换、QGraphicsScene管理、拖拽交互、渲染、跳字动画  │
│  监听 GameManager 信号 → 刷新战场 / 显示结算 / 跳字          │
├──────────────────────────────────────────────────────────────┤
│                 GameManager (game_manager.cpp)                │
│  逻辑层：回合状态机、tick驱动、战斗调度、升星合并、出售、结算  │
│  不持有任何武器/子弹数据，全部委托给 Behavior                 │
├──────────────┬────────────────────┬──────────────────────────┤
│ AllyBehavior │  EnemyBehavior     │  DatabaseManager         │
│ (策略模式)    │  (策略模式)         │  (JSON 图鉴)              │
│ ├AlphaAlly   │  ├AlphaEnemy       │  allChessConfigs()       │
│ └BetaAlly    │  └BetaEnemy        │  allEnemyConfigs()       │
├──────────────┴────────────────────┴──────────────────────────┤
│                   state.h (纯数据结构)                        │
│  ChessInstance / EnemyInstance / DrawCmd / Attribute / Config│
└──────────────────────────────────────────────────────────────┘
```

**信号流方向**：`GameManager` → `emit signal` → `MainWindow slot` → 刷新 UI

---

## 4. `state.h` — 所有数据结构

> **位置**: `src/include/state.h`
> **性质**: 纯头文件，无对应的 .cpp。被几乎所有模块 include。约 310 行。

### 4.1 渲染指令 `DrawCmd`

```cpp
struct DrawCmd {
    enum Kind { Circle, RotatedRect };
    Kind kind;           // Circle=子弹圆点, RotatedRect=武器旋转矩形
    double x, y;         // 场景坐标（像素）
    double param1;       // Circle → 半径; RotatedRect → 半宽
    double param2;       // RotatedRect → 半高
    double angle;        // RotatedRect → 弧度
    QColor color;
    int zValue;          // Z轴层级（决定绘制顺序）
};
```

**用途**：每个 behavior 每帧往 `draws` 向量追加自己的渲染数据。GameManager 收集所有 draws 后存入 `m_pendingDraws`，MainWindow::refreshBattleGround() 读取并绘制。**回合结束后 `m_pendingDraws.clear()` 清空**。

### 4.2 回合阶段枚举 `RoundPhase`

| 阶段 | 含义 | 允许操作 |
|------|------|---------|
| `Prepare` | 准备阶段 | 拖拽部署、购买、出售、升星 |
| `Calculate` | 系统计算（预留，未使用） | — |
| `Combat` | 对战阶段 | tick 循环运行，不可拖拽 |
| `ShowResult` | 结算界面弹出 | 查看结果，点OK进入下一轮 |
| `Finish` | 过渡阶段 | 应用收益、重置单位、进入Prepare |

**流转**：`Prepare → Combat → ShowResult → Finish → Prepare → ...`

### 4.3 属性容器 `Attribute`

```cpp
class Attribute {
    int base;              // 白值
    int flatBonus;         // 固定加成
    float percentBonus;    // 百分比加成
    int getFinal() const;  // (base + flatBonus) × (1 + percentBonus)
};
```

### 4.4 伤害公式

```cpp
void applyDamageToUnit(int rawDamage, const Attribute &def,
                       int &currentHp, bool &isAlive);
```
**公式**：`实际伤害 = max(1, rawDamage × 100 / (100 + 防御力))`
HP ≤ 0 时 `isAlive = false`。

### 4.5 配置类层级（静态图鉴数据）

```
BaseConfig                          # 双方共有的基础数值
├─ configId, name                   # 种类ID、名字
├─ baseHp, baseAtk, baseDef         # 1星三维
├─ hpGrowthMultiplier               # HP 每星成长倍率
├─ atkGrowthMultiplier              # ATK 每星成长倍率
├─ attackRange, baseAttackSpeed     # 攻击距离、基础攻速
├─ maxMp                            # 蓝量上限
├─ speed                            # 移动速度
├─ behaviorId                       # 行为ID（1=Alpha近战, 2=Beta远程）
└─ description                      # 描述文本
    │
    ├── ChessConfig : BaseConfig    # 棋子静态配置
    │   ├─ cost                     # 费用（1~5费）
    │   └─ bonds                    # 羁绊标签列表
    │
    └── EnemyConfig : BaseConfig    # 敌人静态配置
        ├─ baseGoldReward           # 击杀金币奖励
        ├─ baseExpReward            # 击杀经验奖励
        └─ isRequiredByDefault      # 是否必修单位
```

### 4.6 运行时实例

#### `ChessInstance : ChessConfig` — 我方棋子

| 成员 | 类型 | 说明 |
|------|------|------|
| `uuid` | int | 局内唯一ID（自增原子计数器） |
| `starLevel` | int | 当前星级 1~3 |
| `hp/atk/def/attackSpeed` | Attribute | 最终属性（由星级+成长系数算） |
| `currentHp` | int | 当前血量 |
| `currentMp` | int | 当前蓝量 |
| `isAlive` | bool | 是否存活 |
| `deployed` | bool | true=战场, false=备战席 |
| `benchSlot` | int | 备战席槽位 0~10（-1=已部署） |
| `posX/posY` | double | **战场坐标**（仅 deployed 时有效） |
| `prevPosX/prevPosY` | double | **上一 tick 坐标**（渲染插值用） |
| `savedPosX/savedPosY` | double | **回合开始时坐标快照**（回合结束复位用） |
| `behavior` | unique_ptr\<AllyBehavior\> | 行为策略（多态） |

关键方法：
- `calculateBaseStatsByStar()`：根据星级重算三维 `base × multiplier^(star-1)`
- `resetStatus()`：回满 HP/MP，重置存活状态（每轮开始时调用）
- `takeDamage(rawDamage)`：调用 `applyDamageToUnit`

#### `EnemyInstance : EnemyConfig` — 敌方单位

| 成员 | 类型 | 说明 |
|------|------|------|
| `uuid` | int | 局内唯一ID |
| `hp/atk/def/attackSpeed` | Attribute | 最终属性（含回合数加成） |
| `currentHp` | int | 当前血量 |
| `isAlive` | bool | 是否存活 |
| `posX/posY` | double | 战场坐标 |
| `prevPosX/prevPosY` | double | 上一 tick 坐标（插值） |
| `behavior` | unique_ptr\<EnemyBehavior\> | 行为策略 |

构造函数中：`HP × 1.15^(round-1)`, `ATK × 1.10^(round-1)`

### 4.7 `PlayerAssets` — 玩家全局数据

| 成员/常量 | 说明 |
|-----------|------|
| `gold` | 金币 |
| `exp` | 经验 |
| `ownedChesses` | `vector<ChessInstance>` 所有棋子 |
| `maxBattlefield = 5` | 战场部署上限 |
| `maxBench = 11` | 备战席槽位数 |

关键方法：
- `deployedCount()` / `benchCount()` / `battlefieldFull()` / `benchFull()`
- `getUnitByUuid(int uuid)` — O(n) 按 UUID 查找，返回指针（n ≤ 16）
- `compactBenchSlots()` — 压缩备战席空槽
- `firstEmptyBenchSlot()` — 找第一个空槽

---

## 5. `MainWindow` — UI 层

> **文件**: `src/entry.cpp`（约 750 行）+ `src/include/mainwindow.h`

> **特点**: 文件按 `// %` 注释分区，按运行时流程排列

### 5.1 场景常量

```cpp
kBattlefieldWidth      = 1920   // 战场全宽
kBattlefieldHeight     = 800    // 战场可玩区高度
kBenchAreaHeight       = 180    // 底部备战栏高度
kBattlefieldMargin     = 50     // 部署区边界留白
kDeployRightExtension  = 200    // 部署区右侧额外扩展
kBenchSlotWidth        = 150    // 备战槽宽度
kBenchSlotHeight       = 100    // 备战槽高度
kBenchSlotStartX       = 65.0   // 第一个槽 X 坐标
kBenchSlotStartY       = 840.0  // 第一个槽 Y 坐标（800 + 40）
```

### 5.2 核心成员

| 成员 | 类型 | 说明 |
|------|------|------|
| `m_gameManager` | GameManager* | 游戏逻辑总控 |
| `m_battleScene` | QGraphicsScene* | 战场+备战席场景 |
| `m_unitItems` | QMap<int, UnitGraphicsItem*> | uuid → 图形项映射 |
| `m_towerGpaText` | QGraphicsSimpleTextItem* | 塔上显示的学分绩 |
| `m_sellLabel` | QGraphicsSimpleTextItem* | 出售区标签（拖拽时动态显示"出售 ￥n"） |
| `m_shopWindow` | ShopWindow* | 商店弹窗 |
| `m_database` | DatabaseManager* | 图鉴数据库 |

### 5.3 函数清单（按文件分区顺序）

#### % 生命周期

| 函数 | 说明 |
|------|------|
| `MainWindow()` | 构造：`initUi() →` 连接 Logo 播放完毕 / 开始游戏按钮 |
| `initUi()` | 固定窗口 1920×1080，切到 Logo 场景，调用 `setupGameScene()` |
| `initGame()` | **首次点击"开始游戏"触发**：创建 GameManager + DatabaseManager，**绑定全部信号槽**（见 §5.4），调用 `m_gameManager->initialize()` |
| `~MainWindow()` | 析构：delete ui |

#### % 场景搭建

| 函数 | 说明 |
|------|------|
| `switchScene(Scene)` | 切换 QStackedWidget 页面（Logo / 菜单 / 主游戏） |
| `assetPath(relPath)` | 拼接 `可执行文件目录/assets/xxx`，文件不存在时打印警告 |
| `setupGameScene()` | **一次性**：创建 QGraphicsScene(1920×980)，绘制背景贴图、塔占位符(70×70)、备战席背景、部署区边框、11个备战槽、出售区(100×100 红色方块) |

#### % 战场刷新（每 tick 调用）

| 函数 | 说明 |
|------|------|
| `refreshBattleGround()` | ①清除 tag=-1 的临时项 ②遍历 `getEnemies()` 绘制敌方圆+血条+文字 ③遍历 `getPendingDraws()` 绘制 DrawCmd（Circle 画圆、RotatedRect 画旋转矩形）④更新塔学分绩 |
| `refreshAllUnits()` | 遍历 `ownedChesses`，创建/更新 `UnitGraphicsItem`：部署的用 `posX/Y`（加插值 lerp），备战的用槽位坐标；清理已删除单位 |
| `showSplashText(text,x,y,color)` | 战斗跳字：两层文字(前景+阴影) → 缩放动画 4×→1×(300ms) → 800ms 后自动删除 |

#### % 拖拽交互

| 函数 | 说明 |
|------|------|
| `onUnitDragStarted(uuid, pos)` | 查找单位 → 计算出售价值 `getTotalWorth()` → 标签显示"出售 ￥n" → 重新居中 |
| `onUnitDragFinished(uuid, pos)` | 根据目标区域走不同分支：**S**=出售 / **A**=部署 / **B**=撤回 / **C**=战场移动 / **D**=备战席交换 / **E**=弹回 |
| `isInBattlefieldLegalZone(pos)` | x∈[50, 1010] 且 y∈[50, 750] |
| `isInBenchZone(pos)` | y∈(800, 980) |
| `isInSellZone(pos)` | 在出售区红色方块内（最右侧 100×100） |
| `nearestBenchSlot(pos)` | 找离触摸点最近的空备战槽（跳过已占用） |
| `recenterSellLabel()` | 根据 `boundingRect().width()` 重新居中出售标签 |

#### % UI标签

| 函数 | 说明 |
|------|------|
| `refreshSceneLabels()` | 更新 5 个 QLabel：回合数、已修学分、GPA、上场数、金币数 |
| `onShopOpenClicked()` | 打开商店（仅 Prepare 阶段），关闭时触发升星合并 |

#### % 结算

| 函数 | 说明 |
|------|------|
| `showRoundResult(victory)` | 弹出 QMessageBox 显示回合结算，点 OK 后 300ms 调用 `nextRound()` |
| `showGameOver(finalGpa)` | 弹出 QMessageBox 显示整局结算，切回菜单场景 |

### 5.4 信号槽布线

**在 `initGame()` 中一次性连接，之后不再变动**：

```
GameManager::splashText        → MainWindow::showSplashText          (跳字动画)
GameManager::phaseChanged      → Lambda: 启/禁用按钮 + refresh       (阶段切换)
GameManager::tick              → Lambda: refreshBattleGround()       (战斗帧)
                                      + refreshAllUnits()
GameManager::roundEnded        → MainWindow::showRoundResult         (回合结算)
GameManager::gameOver          → MainWindow::showGameOver            (游戏结束)

startRoundButton::clicked      → Lambda: m_gameManager->startRound()
openShopButton::clicked        → MainWindow::onShopOpenClicked

UnitGraphicsItem::dragStarted  → MainWindow::onUnitDragStarted       (出售价显示)
UnitGraphicsItem::dragFinished → MainWindow::onUnitDragFinished      (拖拽处理)

ShopWindow::shopClosed         → Lambda: checkAndMergeStars()        (商店关闭)
                                      + refreshAllUnits()
                                      + refreshSceneLabels()
```

### 5.5 位置插值渲染

在 `refreshBattleGround()` 和 `refreshAllUnits()` 中：
```cpp
double lerpT = m_gameManager->getTickLerp();  // 0.0 ~ 1.0
displayX = prevPosX + (posX - prevPosX) * lerpT;
displayY = prevPosY + (posY - prevPosY) * lerpT;
```
50ms tick 间隔被插值因子平滑填充，视觉上无跳跃感。拖拽部署时同步设置 `prevPos = pos` 防止首次渲染从原点起跳。

---

## 6. `GameManager` — 逻辑层

> **文件**: `src/core/game_manager.cpp`（约 360 行）+ `src/include/game_manager.h`
> **核心常量**: `GAME_TICK_INTERVAL_MS = 50` (20Hz), `BASE_TOWER_HP = 10000`

### 6.1 成员变量

| 成员 | 说明 |
|------|------|
| `m_player` | `PlayerAssets` 玩家数据（金币、经验、棋子） |
| `m_enemies` | `vector<EnemyInstance>` 当前敌方单位 |
| `m_phase` | `RoundPhase` 回合阶段 |
| `m_roundNumber` | 当前回合数（1-based） |
| `m_towerHp` / `m_maxTowerHp` | 塔血量 / 塔满血值 |
| `m_towerAttackCooldown` | 塔攻击冷却（1.5s） |
| `m_tickTimer` | `QTimer` 每 50ms 触发一次 `onTick()` |
| `m_timeAccumulator` | 累计模拟时间（秒） |
| `m_tickProgress` | 插值进度 0→1（`fmod(acc, 0.05) / 0.05`） |
| `m_pendingDraws` | 每帧 behavior 产生的渲染指令（**回合结束清空**） |
| `m_pendingGold` / `m_pendingExp` | 战斗中累计的临时收益 |
| `m_roundStartGold` / `m_roundStartExp` | 回合开始时快照（计算本回合净收入） |
| `m_weightedGpaSum` / `m_totalCredits` | 加权学分绩累计 |
| `m_gameSeed` / `m_rng` | 随机种子与 QRandomGenerator |

### 6.2 函数清单

| 函数 | 说明 |
|------|------|
| `initialize()` | 整局初始化：随机种子、10金币、赠送2个随机棋子（放备战席） |
| `startRound(n)` | 开始回合：快照金币+位置，塔满血，生成3~4个敌人，启动 tickTimer |
| `stopRound()` | 停止 tickTimer，**清空 `m_pendingDraws`**（子弹/武器消失） |
| `nextRound()` | 结算收益（金币+经验+学分绩），调用 `resetUnitsForNextRound()`，进入 Prepare |
| `resetUnitsForNextRound()` | 重置棋子 HP/状态，**恢复 posX/Y 到回合开始快照**，清除敌人 |
| `transitionPhase(p)` | 切换阶段 + emit `phaseChanged` |
| `spawnEnemies(n)` | 生成 3+(n>5?1:0) 个敌人，分配行为策略，设置初始位置 |
| `onTick()` | **tick 入口**：计算插值进度 → `executeAttackCycle()` → emit `tick()` → 检查战斗结束 |
| `executeAttackCycle(dt)` | **核心战斗循环**（见 §6.3） |
| `checkCombatEndConditions()` | 胜利=敌方全灭 / 失败=塔HP≤0 |
| `checkAndMergeStars()` | 3同种同星→1高星（优先保留战场上的） |
| `sellUnit(uuid)` | 出售棋子：`返还金币 = cost × 3^(star-1)` |
| `getTotalWorth(star,cost)` | 计算总价值（贴现金币） |
| `getTickLerp()` | 返回插值因子 0~1 |
| `getRoundGpa()` | `塔血% × 4.0` |
| `getRoundCredit(n)` | 第1门2分、第2门3分、第3门5分 |

### 6.3 `executeAttackCycle(dt)` — 核心战斗循环

```
1. 塔攻击冷却递减
2. 准备 splashFn = [this](...) { emit splashText(...); }
3. 创建 draws = {}
4. ★ 保存 prevPos = pos (所有盟友 + 敌人)
5. ★ for each deployed ally:   behavior->tick(dt, ally, enemies, draws, ...)
6. ★ for each enemy alive:     behavior->tick(dt, enemy, allies, draws, towerHp, ...)
7. 塔攻击（如果冷却完毕 且 有存活敌人）
   └→ 伤害 = 30 × (1 + (1-HP%) × 3)
   └→ 目标 = 第一个存活敌人
   └→ 击杀: 10%概率掉金币 + 必掉经验
8. m_pendingDraws = std::move(draws)
```

### 6.4 战斗全时序

```
startRound()
  ├─ 快照: m_roundStartGold / m_roundStartExp
  ├─ 快照: 每个 deployed 单位的 savedPosX/Y = posX/Y
  ├─ spawnEnemies()
  └─ m_tickTimer->start()

onTick()  [每 50ms]
  ├─ m_tickProgress = fmod(timeAccumulator, 0.05) / 0.05
  ├─ executeAttackCycle(dt)
  │   ├─ 保存 prevPos (allies + enemies)
  │   ├─ for ally:   behavior->tick()     → 移动 + 攻击 + 生成 draws
  │   ├─ for enemy:  behavior->tick()     → 移动 + 攻击 + 生成 draws
  │   ├─ 塔攻击 (if cooldown ready)
  │   └─ m_pendingDraws = draws
  ├─ emit tick()  →  MainWindow::refreshBattleGround() + refreshAllUnits()
  └─ checkCombatEndConditions()
      └─ [true] → stopRound()  [停止tick, 清空draws]
                → transitionPhase(ShowResult)
                → emit roundEnded(victory)

showRoundResult()  [玩家看到结算弹窗，点OK]
  └─ QTimer(300ms) → nextRound()
      ├─ 结算: 金币 += pendingGold + 10, 经验 += pendingExp
      ├─ resetUnitsForNextRound()
      │   ├─ 每个单位: resetStatus() (HP回满)
      │   └─ deployed 单位: posX/Y = savedPosX/Y (回到初始位置)
      └─ transitionPhase(Prepare)
```

---

## 7. 行为系统 — 策略模式

> **核心设计原则**：每个角色的逻辑**完全自包含**在 behavior 类中。
> 武器、子弹、冷却等全部私有数据归 behavior 自己管理。
> GameManager 不持有任何弹药/武器容器，只负责调用 `tick()` 并收集 DrawCmd。

### 7.1 基类接口

#### `AllyBehavior` (`src/include/entity/ally_behavior.h`)

```cpp
virtual void tick(
    double dt,                              // 时间增量（秒）
    ChessInstance &self,                    // 自身实例（可修改 posX/Y 等）
    std::vector<EnemyInstance> &enemies,    // 敌方列表（可查位置、可伤害）
    std::vector<DrawCmd> &draws,            // ★ 追加渲染指令到此向量
    int &pendingGold,                       // 战斗中临时金币收益
    int &pendingExp,                        // 战斗中临时经验收益
    const SplashFn &splash                  // 跳字回调
) = 0;
```

#### `EnemyBehavior` (`src/include/entity/enemy_behavior.h`)

```cpp
virtual void tick(
    double dt,
    EnemyInstance &self,
    std::vector<ChessInstance> &allies,     // 我方列表（可查位置、可伤害）
    std::vector<DrawCmd> &draws,            // ★ 追加渲染指令
    int &towerHp,                           // 塔血量（可扣减）
    int &pendingGold,
    int &pendingExp,
    const SplashFn &splash
) = 0;
```

### 7.2 具体实现对比

| 属性 | AlphaAlly (behaviorId=1) | BetaAlly (behaviorId=2) | AlphaEnemy | BetaEnemy |
|------|--------------------------|--------------------------|------------|-----------|
| 类型 | 近战 | 远程 | 近战 | 远程 |
| 文件 | `allies/alpha/` | `allies/beta/` | `enemies/alpha/` | `enemies/beta/` |
| 索敌 | 最近 | 最远 | 最近我方 | 最远我方 |
| 移动 | 向目标移动到 70px | 不移动 | 向目标移动到 70px | 不移动 |
| 攻击方式 | 3圈旋转武器（0.2s/圈） | 发射子弹（400px/s） | 同 AlphaAlly | 发射子弹 |
| 范围 | 120px | 子弹碰撞 40px | 120px | 子弹碰撞 40px |
| 颜色 | 橙色 #FF8C32 | 绿色 #64DC50 | 红色 | 红色 |
| 特殊 | — | — | — | **无我方时攻击塔** |

### 7.3 私有数据结构

**Alpha 系列**（近战）：
```cpp
struct Weapon {
    bool active = false;         // 是否正在挥砍
    double angle = 0.0;         // 当前旋转角度（弧度）
    double elapsed = 0.0;       // 已过时间
    int rotationsDone = 0;      // 已完成圈数
    int targetUuid = -1;        // 目标 UUID
};
Weapon m_weapon;
double m_cooldown;              // 冷却倒计时
```
- 每圈（0.2s）对 120px 内敌人造成一次 `atk` 伤害
- 3 圈后进入冷却 `cooldown = 1.0 / attackSpeed`
- 渲染: `DrawCmd::RotatedRect` (55×8 旋转矩形)
- 冷却中不移动不攻击

**Beta 系列**（远程）：
```cpp
struct Bullet {
    double x, y;     // 当前位置
    double vx, vy;   // 速度分量
    double damage;   // 伤害值
};
std::vector<Bullet> m_bullets;
double m_cooldown;
```
- 每 tick：推进所有子弹，碰撞检测 40px
- 边界清除：`x∈(-100,2020)` `y∈(-100,900)` 外自动销毁
- 冷却完毕时发射一发子弹（伤害 = atk）
- 渲染: `DrawCmd::Circle` (半径 6px)

### 7.4 工厂函数

```cpp
// src/entity/allies/ally_factory.cpp
AllyBehavior *createAllyBehavior(int behaviorId) {
    switch (behaviorId) {
        case 1: return new AlphaAlly();
        case 2: return new BetaAlly();
        default: return nullptr;
    }
}

// src/entity/enemies/enemy_factory.cpp
EnemyBehavior *createEnemyBehavior(int behaviorId) {
    // 同上逻辑
}
```

在 `initialize()` / `spawnEnemies()` 中通过 `inst.behavior.reset(createXxxBehavior(id))` 注入。

---

## 8. 辅助模块

### 8.1 `UnitGraphicsItem` — 可拖拽单位图形项

> **文件**: `src/include/unit_graphics_item.h` + `src/unit_graphics_item.cpp`
> 继承 `QGraphicsObject`，支持拖拽。

| 信号 | 触发时机 |
|------|---------|
| `dragStarted(int uuid, QPointF scenePos)` | 鼠标按下 |
| `dragFinished(int uuid, QPointF scenePos)` | 鼠标释放 |

| 方法 | 说明 |
|------|------|
| `updateVisual(name, hp, maxHp, color, radius, star, deployed)` | 刷新绘制外观 |
| `setDraggable(bool)` | 控制可拖拽性（仅 Prepare 阶段 true） |
| `setPos(QPointF)` | 设置场景坐标（由 refreshAllUnits 驱动） |

绘制内容：圆形身体 + 血条(背景/前景) + 名字/HP文字 + 星级星星。

### 8.2 `DatabaseManager` — 图鉴数据库

读取 `assets/entities/` 下 JSON 配置，提供：
```cpp
const std::vector<ChessConfig> &allChessConfigs() const;
const std::vector<EnemyConfig> &allEnemyConfigs() const;
```

### 8.3 `ShopWindow` — 商店窗口

准备阶段可打开。随机展示可买棋子。购买后关闭时 emit `shopClosed` → 触发 `checkAndMergeStars()`。

### 8.4 `LogoPlayer` — 开屏动画

播放 `assets/logo_frames/` 序列帧。播放完毕 emit `logoPlayFinished` → `switchScene(EntryMenu)`。

### 8.5 `print()` — 全局日志

```cpp
// src/include/print.h
void print(const QString &msg);  // qDebug() + 时间戳前缀
```

---

## 9. 准备阶段拖拽交互

### 9.1 区域划分

```
┌─────────────────┬──────────────────────────────────┐
│  部署区（左半场） │  敌方区域（右半场，不可拖入）       │
│  x: [50, 1010]  │  x: (1010, 1920)                │
│  y: [50, 750]   │  y: [50, 750]                   │
├─────────────────┴──────────────────────────────────┤
│  备战席   y: [800, 980]                             │
│  槽位1~11 (间距 150px)       出售区 (最右侧 100×100)  │
└────────────────────────────────────────────────────┘
```

### 9.2 拖拽分支（`onUnitDragFinished` 内）

| 分支 | 情形 | 处理 |
|------|------|------|
| **S** | 拖到出售区 | `sellUnit(uuid)` → 标签恢复"出售" → 刷新 |
| **A** | 备战席 → 部署区（且未满） | `deployed=true, benchSlot=-1`，记录 `posX/Y`，同步 `prevPos` |
| **B** | 部署区 → 备战席（有空槽） | `deployed=false, benchSlot=targetSlot, posX/Y=0` |
| **C** | 部署区内移动 | 仅更新 `posX/Y`（战斗中 behavior 会从此出发） |
| **D** | 备战席内移动/交换 | 交换或移动到空槽 |
| **E** | 非法区域 | 弹回原位，不做数据修改 |

所有分支结束都恢复出售标签为"出售"并重新居中。

---

## 10. 关键参数速查

| 参数 | 值 | 位置 |
|------|-----|------|
| Tick 间隔 | 50ms (20Hz) | `game_manager.h:8` |
| 塔基础血量 | 10000 | `game_manager.h:9` |
| 近战攻击范围 | 120px | Alpha 系列 `tick()` 内 |
| 近战停步距离 | 70px | Alpha 系列 `tick()` 内 |
| 武器旋转速度 | 0.2s/圈 × 3圈 | Alpha 系列 `tick()` 内 |
| 子弹速度 | 400px/s | Beta 系列 `tick()` 内 |
| 子弹碰撞半径 | 40px | Beta 系列 `tick()` 内 |
| 子弹边界清除 | x(-100,2020) y(-100,900) | Beta 系列 `tick()` 内 |
| 塔攻击冷却 | 1.5s | `game_manager.cpp:executeAttackCycle` |
| 塔攻击伤害 | 30×(1+(1-HP%)×3) | `game_manager.cpp:executeAttackCycle` |
| 出售返还 | `cost × 3^(star-1)` | `game_manager.h:getTotalWorth` |
| 升星条件 | 3个同种同星 | `game_manager.cpp:checkAndMergeStars` |
| 开局免费棋子 | 2个 | `game_manager.cpp:initialize` |
| 敌方数量 | 3 + (round>5 ? 1 : 0) | `game_manager.cpp:spawnEnemies` |
| 敌方 HP 成长 | `×1.15^(round-1)` | `state.h:EnemyInstance 构造` |
| 敌方 ATK 成长 | `×1.10^(round-1)` | `state.h:EnemyInstance 构造` |
| 最大回合数 | 3 | `game_manager.h` |
| 战场部署上限 | 5 | `state.h:PlayerAssets` |
| 备战席槽位数 | 11 | `state.h:PlayerAssets` |
| 学分绩公式 | `(塔血% × 4.0)` | `game_manager.h:getRoundGpa` |
| 回合学分 | 2 / 3 / 5 | `game_manager.h:getRoundCredit` |

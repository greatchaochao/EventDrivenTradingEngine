# Week 5 — Order Management System (OMS) & Execution

## Overview

**Theme:** Order Management System (OMS) & Execution

By the end of this week, your engine will be fully closed-loop: it reads market data, generates signals, **automatically executes trades**, and tracks your account equity in real time. Every trade is logged to `data/trades.csv`, which becomes part of the Week 7 performance report.

**Environment:** GitHub Codespaces (same environment as previous weeks)

**Time estimate:** 1.5 – 2 hours

---

## What You Will Build This Week

Two new files, one updated file:

1. **`src/portfolio.h`** — declares the `Trade` struct, the `Portfolio` struct, and three helper functions
2. **`src/portfolio.cpp`** — implements `executeBuy`, `executeSell`, and `getEquity`
3. **Updated `src/engine.cpp`** — creates a `Portfolio` each cycle, calls `executeBuy`/`executeSell` on signals, prints live equity, and writes `data/trades.csv`

`CMakeLists.txt` gets a one-word update. `main.cpp`, `data_feed.*`, and `strategy.*` are untouched.

New C++ concepts introduced:
- `struct` with member functions used as a data container
- Passing a non-`const` reference (`Portfolio&`) to let a function modify a variable
- `std::floor` — round down to the nearest integer
- `static constexpr` — compile-time constant, safer than `#define`
- Writing multiple CSV files from one run
- Range-based `for` loop with `const` reference

---

## Background: What Is a Portfolio Module?

In a real OMS, the portfolio module answers three questions at every tick:

| Question                            | Answer                                  |
| ----------------------------------- | --------------------------------------- |
| Can I afford to buy?                | Check `cash` against `price × quantity` |
| What do I currently hold?           | Read `position`                         |
| What is my account worth right now? | `cash + position × currentPrice`        |

This week we use the simplest possible model:

- **Starting capital:** 10,000
- **On BUY signal:** buy as many whole shares as the available cash allows (all-in)
- **On SELL signal:** sell the entire position (all-out)
- **Commission:** 0.1 % of trade value, deducted on both buy and sell
- **No short-selling, no leverage, no fractional shares**

The math for a buy:

$$\text{shares} = \left\lfloor \frac{\text{cash}}{\text{price} \times (1 + \text{commissionRate})} \right\rfloor$$

$$\text{total cost} = \text{shares} \times \text{price} \times (1 + \text{commissionRate})$$

$$\text{equity} = \text{cash} + \text{position} \times \text{currentPrice}$$

---

## Step-by-Step Plan

### Phase A — Update CMakeLists.txt

**Step 1 — Add portfolio.cpp to the build**

1. Open `CMakeLists.txt`.
2. Find the `add_executable` line and add `src/portfolio.cpp`:

```cmake
add_executable(engine
    src/main.cpp
    src/data_feed.cpp
    src/engine.cpp
    src/strategy.cpp
    src/portfolio.cpp
)
```

3. Press **Ctrl+S** to save.

> Only `src/portfolio.cpp` is new. Every new module you add follows the same pattern: append its `.cpp` file here.

---

### Phase B — Create the header file

**Step 2 — Create `src/portfolio.h`**

1. Right-click the `src/` folder → **"New File"** → name it `portfolio.h`.
2. Paste the following:

```cpp
#pragma once
// portfolio.h — 投资组合模块的声明文件
// 本文件定义账户所需的数据类型，并声明三个操作函数
// 任何需要管理账户的文件，只需 #include "portfolio.h" 即可

#include <string>
#include <vector>

// ─────────────────────────────────────────────
// Trade：记录一次完整的成交记录
//
// 每当 BUY 或 SELL 信号触发真实成交时，就创建一个 Trade 对象并保存下来
// 这些记录最终写入 data/trades.csv，供 Week 7 的 Python 脚本绘图使用
// ─────────────────────────────────────────────
struct Trade {
    std::string date;       // 成交日期，例如 "2010-02-17"
    std::string action;     // 操作方向："BUY" 或 "SELL"
    double      price;      // 成交价格（单位：美元/股）
    int         quantity;   // 成交数量（股数）
    double      commission; // 手续费 = 成交金额 × 0.1%
    double      cashAfter;  // 成交后账户剩余现金
};

// ─────────────────────────────────────────────
// Portfolio：实时记录账户的资产状态
//
//   cash      — 账户中当前持有的现金
//   position  — 当前持有的股票数量（本系统只持有一种资产：AAPL）
//   tradeLog  — 历史成交记录，用 vector 存储所有 Trade 对象
//
// 设计原则：Portfolio 是一个纯数据容器（Plain Data Struct）
//   它自己不做任何计算，只存储状态
//   所有修改都通过下方的三个函数完成
// ─────────────────────────────────────────────
struct Portfolio {
    double           cash;       // 账户现金
    int              position;   // 持仓数量（股数）
    std::vector<Trade> tradeLog; // 成交日志
};

// ─────────────────────────────────────────────
// createPortfolio：初始化一个空账户
//
// 参数：
//   initialCash — 初始资金（例如 10000.0）
//
// 返回：一个现金充足、持仓为零的 Portfolio 对象
// ─────────────────────────────────────────────
Portfolio createPortfolio(double initialCash);

// ─────────────────────────────────────────────
// executeBuy：以当前价格买入尽可能多的股票
//
// 参数：
//   p     — Portfolio 的引用（& 代表引用，函数内修改会影响外部变量）
//   date  — 本次成交的日期字符串
//   price — 本次成交的价格
//
// 返回：
//   true  — 成交成功（已买入股票）
//   false — 成交失败（原因：已有持仓，或现金不足）
//
// 注意：& 与 const & 的区别
//   const Portfolio& p  — 只读引用，函数不能修改 p（Week 3/4 中 runEngine 的参数）
//         Portfolio& p  — 可写引用，函数可以修改 p（本函数需要扣现金、增持仓）
// ─────────────────────────────────────────────
bool executeBuy(Portfolio& p, const std::string& date, double price);

// ─────────────────────────────────────────────
// executeSell：卖出全部持仓
//
// 参数：同 executeBuy
//
// 返回：
//   true  — 成交成功（已卖出全部股票）
//   false — 成交失败（原因：当前没有持仓）
// ─────────────────────────────────────────────
bool executeSell(Portfolio& p, const std::string& date, double price);

// ─────────────────────────────────────────────
// getEquity：计算账户当前总净值
//
// 公式：总净值 = 现金 + 持仓股数 × 当前价格
//
// 参数：
//   p            — 只读引用（不修改账户，仅读取数据）
//   currentPrice — 本 tick 的最新市场价格
//
// 返回：以浮点数表示的账户总价值（美元）
// ─────────────────────────────────────────────
double getEquity(const Portfolio& p, double currentPrice);
```

3. Press **Ctrl+S** to save.

---

### Phase C — Create the implementation file

**Step 3 — Create `src/portfolio.cpp`**

1. Right-click the `src/` folder → **"New File"** → name it `portfolio.cpp`.
2. Paste the following:

```cpp
#include "portfolio.h"
#include <cmath>    // std::floor — 向下取整，用于计算可买股数
#include <iostream> // std::cout — 打印成交提示

// ─────────────────────────────────────────────
// COMMISSION_RATE：手续费率常量
//
// constexpr — "compile-time constant"（编译期常量）
//   程序编译时编译器直接把 0.001 替换进去，运行时不占用任何内存
//   比 #define COMMISSION_RATE 0.001 更安全，因为它有类型（double）
//
// static — 限制此常量只在本文件内可见
//   其他文件无法访问 COMMISSION_RATE，防止命名冲突
// ─────────────────────────────────────────────
static constexpr double COMMISSION_RATE = 0.001; // 0.1% 手续费率

// ─────────────────────────────────────────────
// createPortfolio 实现
// ─────────────────────────────────────────────
Portfolio createPortfolio(double initialCash) {
    Portfolio p;
    p.cash     = initialCash; // 初始现金
    p.position = 0;           // 初始持仓为零
    // p.tradeLog 是 std::vector，默认构造为空列表，无需手动初始化
    return p;
}

// ─────────────────────────────────────────────
// executeBuy 实现
// ─────────────────────────────────────────────
bool executeBuy(Portfolio& p, const std::string& date, double price) {

    // ── 检查 1：是否已有持仓 ──────────────────────────────
    // 本系统采用"满仓/空仓"模式（all-in / all-out）：
    //   有持仓时不重复买入，避免资金管理复杂化
    if (p.position > 0) {
        return false; // 已有持仓，忽略本次 BUY 信号
    }

    // ── 计算可买股数 ──────────────────────────────────────
    // 设可买股数为 n，需满足：n × price × (1 + rate) ≤ cash
    // 解方程得：n ≤ cash / (price × (1 + rate))
    // 因为股数必须是整数，对结果向下取整（不能买半股）
    //
    // std::floor(x)：返回不超过 x 的最大整数
    //   std::floor(3.9) == 3.0
    //   std::floor(3.1) == 3.0
    //
    // static_cast<int>(...)：将 double 转换为 int
    //   必须显式转换，否则编译器会报"精度损失"警告
    int shares = static_cast<int>(
        std::floor(p.cash / (price * (1.0 + COMMISSION_RATE)))
    );

    // ── 检查 2：现金是否足够买入至少 1 股 ───────────────────
    if (shares <= 0) {
        return false; // 现金不足，无法成交
    }

    // ── 计算本次成交的各项金额 ────────────────────────────
    double commission = shares * price * COMMISSION_RATE; // 手续费
    double totalCost  = shares * price + commission;       // 总花费

    // ── 更新账户状态 ──────────────────────────────────────
    p.cash     -= totalCost; // 扣除现金
    p.position  = shares;    // 增加持仓

    // ── 记录本次成交 ────────────────────────────────────
    // 使用列表初始化语法（C++11 起支持）直接构造 Trade 对象
    // 按照 struct 中字段声明的顺序依次赋值
    Trade t;
    t.date       = date;
    t.action     = "BUY";
    t.price      = price;
    t.quantity   = shares;
    t.commission = commission;
    t.cashAfter  = p.cash;

    // push_back：把 t 追加到 tradeLog 列表末尾
    p.tradeLog.push_back(t);

    return true; // 成交成功
}

// ─────────────────────────────────────────────
// executeSell 实现
// ─────────────────────────────────────────────
bool executeSell(Portfolio& p, const std::string& date, double price) {

    // ── 检查：是否有持仓可以卖出 ─────────────────────────
    if (p.position == 0) {
        return false; // 空仓，忽略本次 SELL 信号
    }

    int shares = p.position; // 卖出全部持仓

    // ── 计算本次成交的各项金额 ────────────────────────────
    double commission = shares * price * COMMISSION_RATE; // 手续费
    double proceeds   = shares * price - commission;       // 卖出净收入

    // ── 更新账户状态 ──────────────────────────────────────
    p.cash    += proceeds; // 收回现金
    p.position = 0;        // 清空持仓

    // ── 记录本次成交 ────────────────────────────────────
    Trade t;
    t.date       = date;
    t.action     = "SELL";
    t.price      = price;
    t.quantity   = shares;
    t.commission = commission;
    t.cashAfter  = p.cash;

    p.tradeLog.push_back(t);

    return true; // 成交成功
}

// ─────────────────────────────────────────────
// getEquity 实现
// ─────────────────────────────────────────────
double getEquity(const Portfolio& p, double currentPrice) {
    // 账户总净值 = 可用现金 + 持仓股票的实时市值
    // 如果 position == 0（空仓），持仓市值为 0，净值即现金
    return p.cash + p.position * currentPrice;
}
```

3. Press **Ctrl+S** to save.

---

### Phase D — Update engine.cpp

**Step 4 — Integrate the portfolio into the engine loop**

The existing `while (true)` loop, `prevPrice` / direction logic, and `computeSignal` call all stay exactly as they are. You only need to add four things:

1. `#include "portfolio.h"` at the top.
2. A `Portfolio portfolio = createPortfolio(10000.0)` at the start of each cycle.
3. A second `std::ofstream tradeCsv(...)` opened once per cycle.
4. After `computeSignal`, call `executeBuy` or `executeSell` based on the signal; print equity; write to both CSVs; and print a trade summary at the end of each cycle.

Replace the entire contents of `src/engine.cpp` with:

```cpp
#include "engine.h"
#include "portfolio.h" // 新增：引入投资组合模块（Week 5）
#include "strategy.h"  // 引入信号计算函数 computeSignal 和 Signal 枚举
#include <chrono>
#include <fstream>     // std::ofstream — 用于写入 CSV 日志文件
#include <iostream>
#include <thread>

void runEngine(const std::vector<Tick>& ticks) {
    std::cout << "引擎启动，共加载 " << ticks.size() << " 条行情数据。" << std::endl;
    std::cout << "按 Ctrl+C 停止。" << std::endl;
    std::cout << std::endl;

    int cycle = 1; // 记录当前是第几轮循环（仅用于终端显示）

    // 外层循环：数据播完后自动重头开始，模拟持续运行的实时行情
    while (true) {
        std::cout << "--- 第 " << cycle << " 轮 ---" << std::endl;

        // ── 每轮开始时初始化账户 ──────────────────────────
        // 初始资金设为 10000，每轮重置，便于重复回测
        Portfolio portfolio = createPortfolio(10000.0);

        // ── 每轮开始时创建（或覆盖）日志文件 ──────────────
        // std::ofstream 以写入模式打开文件；若文件已存在则从头覆盖
        // 两个变量（sigCsv, tradeCsv）在本轮 while 循环体结束时
        // 自动关闭（RAII：资源随对象生命周期释放，无需手动 close）
        std::ofstream sigCsv("data/signals.csv");
        std::ofstream tradeCsv("data/trades.csv");

        // 写入 CSV 表头（第一行）
        sigCsv   << "Date,Price,Signal,Equity\n";
        tradeCsv << "Date,Action,Price,Quantity,Commission,CashAfter\n";

        // ── 价格历史缓冲区，每轮循环重置 ────────────────────
        // priceHistory 存储截至当前 tick 的所有收盘价（按时间顺序）
        // computeSignal 需要读取这段历史才能计算短期和长期均线
        std::vector<double> priceHistory;

        double prevPrice = ticks[0].price; // 记录上一个 Tick 的价格，用于比较涨跌

        // 内层循环：逐条处理每一笔行情数据
        for (int i = 0; i < (int)ticks.size(); i++) {
            double price = ticks[i].price;

            // ── 方向判断（沿用 Week 3 的逻辑）──────────────
            std::string direction;
            if      (price > prevPrice) direction = "UP  ";
            else if (price < prevPrice) direction = "DOWN";
            else                        direction = "FLAT";

            // ── 将本 tick 的价格追加进历史缓冲区 ───────────
            priceHistory.push_back(price);

            // ── 计算当前 tick 的交易信号（Week 4 逻辑）──────
            Signal sig = computeSignal(priceHistory);

            std::string sigStr;
            if      (sig == Signal::BUY)  sigStr = "BUY ";
            else if (sig == Signal::SELL) sigStr = "SELL";
            else                          sigStr = "----";

            // ── 订单执行（Week 5 新增）──────────────────────
            // executeBuy / executeSell 会修改 portfolio 的内部状态
            // 它们返回 true 表示本次成交成功，false 表示忽略（无持仓可卖 / 已有持仓）
            if (sig == Signal::BUY) {
                bool traded = executeBuy(portfolio, ticks[i].date, price);
                if (traded) {
                    // 打印成交通知，便于跟踪系统行为
                    std::cout << "  [买入成交] "
                              << portfolio.tradeLog.back().quantity << " 股 @ "
                              << price
                              << "  手续费: " << portfolio.tradeLog.back().commission
                              << "  剩余现金: " << portfolio.cash
                              << std::endl;
                }
            } else if (sig == Signal::SELL) {
                bool traded = executeSell(portfolio, ticks[i].date, price);
                if (traded) {
                    std::cout << "  [卖出成交] "
                              << portfolio.tradeLog.back().quantity << " 股 @ "
                              << price
                              << "  手续费: " << portfolio.tradeLog.back().commission
                              << "  剩余现金: " << portfolio.cash
                              << std::endl;
                }
            }

            // ── 计算并打印当前账户净值 ───────────────────────
            // getEquity = 现金 + 持仓 × 当前价格
            double equity = getEquity(portfolio, price);

            std::cout << ticks[i].date
                      << "  " << direction
                      << "  " << sigStr
                      << "  price: "  << price
                      << "  equity: " << equity
                      << std::endl;

            // ── 将本行数据写入信号日志 ────────────────────────
            // signals.csv 新增了 Equity 列，供 Week 7 绘制净值曲线
            sigCsv << ticks[i].date << ","
                   << price         << ","
                   << sigStr        << ","
                   << equity        << "\n";

            prevPrice = price; // 更新上一条价格

            // 每条行情之间暂停 50 毫秒，模拟真实行情的播放节奏
            std::this_thread::sleep_for(std::chrono::milliseconds(50));
        }

        // ── 周期结束：将成交记录写入 trades.csv ─────────────
        // 范围 for 循环（Range-based for loop，C++11 起支持）：
        //
        //   for (const Trade& t : portfolio.tradeLog) { ... }
        //
        //   等价于：
        //     for (int i = 0; i < portfolio.tradeLog.size(); i++) {
        //         const Trade& t = portfolio.tradeLog[i];
        //         ...
        //     }
        //
        //   const Trade& t — 只读引用，不复制 Trade 对象，不能修改 t
        //   portfolio.tradeLog — 被遍历的 vector；循环自动从头到尾依次取出每个元素
        for (const Trade& t : portfolio.tradeLog) {
            tradeCsv << t.date       << ","
                     << t.action     << ","
                     << t.price      << ","
                     << t.quantity   << ","
                     << t.commission << ","
                     << t.cashAfter  << "\n";
        }

        // ── 打印本轮交易摘要 ─────────────────────────────────
        std::cout << std::endl;
        std::cout << "本轮共成交 " << portfolio.tradeLog.size() << " 笔：" << std::endl;
        for (const Trade& t : portfolio.tradeLog) {
            std::cout << "  " << t.date
                      << "  " << t.action
                      << "  " << t.quantity << " 股"
                      << "  @ " << t.price
                      << "  手续费: " << t.commission
                      << std::endl;
        }

        // 打印最终净值和盈亏
        double finalEquity = getEquity(portfolio, ticks.back().price);
        std::cout << "最终净值: " << finalEquity
                  << "  盈亏: "   << (finalEquity - 10000.0)
                  << std::endl;

        std::cout << std::endl;
        std::cout << "数据播放完毕。日志已写入 data/signals.csv 和 data/trades.csv。重新开始..."
                  << std::endl;
        std::cout << std::endl;

        cycle++; // 进入下一轮
    }
}
```

3. Press **Ctrl+S** to save.

---

### Phase E — Code Walkthrough

**Passing by non-`const` reference**

```cpp
bool executeBuy(Portfolio& p, ...);   // 可写引用：函数可以修改 p
double getEquity(const Portfolio& p, ...); // 只读引用：函数不能修改 p
```

This week you see both kinds of reference in the same file.

| Signature            | Meaning                                                         |
| -------------------- | --------------------------------------------------------------- |
| `const Portfolio& p` | "I only need to read from `p`" — used in `getEquity`            |
| `Portfolio& p`       | "I need to write to `p`" — used in `executeBuy` / `executeSell` |

The `&` avoids copying the whole struct every time. `const` is a safety promise to the reader of the code.

---

**`static constexpr` vs `#define`**

```cpp
static constexpr double COMMISSION_RATE = 0.001;
```

Both `constexpr` and `#define` produce a hardcoded value that the compiler replaces at compile time. `constexpr` is preferred in modern C++ because:
- It has a **type** (`double`) — the compiler catches unit errors
- It obeys **scope** — `static` keeps it invisible outside `portfolio.cpp`
- It shows up properly in **debuggers**

---

**`std::floor` and the shares calculation**

```cpp
int shares = static_cast<int>(
    std::floor(p.cash / (price * (1.0 + COMMISSION_RATE)))
);
```

`std::floor(3.9)` returns `3.0`. Without `floor`, we might try to buy 3.9 shares and accidentally truncate towards zero in unexpected ways. `static_cast<int>` then converts the rounded `double` to an `int` explicitly — the compiler would warn about an implicit conversion.

---

**Range-based `for` loop**

```cpp
for (const Trade& t : portfolio.tradeLog) {
    tradeCsv << t.date << "," << t.action << ...
}
```

The `:` reads as "in". This loop visits every element in `tradeLog` from first to last, binding each to the reference `t`. It is shorter and less error-prone than a manual index loop because there is no `i` to manage.

---

**Why `executeBuy` returns `bool`**

```cpp
bool traded = executeBuy(portfolio, ticks[i].date, price);
if (traded) { ... }
```

The engine does not need to know *why* a trade was rejected (already have position vs. no cash). It only needs to know *whether* it happened. Returning `bool` is the simplest contract that covers both use cases.

---

### Phase F — Build and Run

**Step 5 — Build and run**

1. Click **⚙ Build** in the blue status bar.
2. The output panel should end with:
   ```
   [100%] Built target engine
   ```
3. Open the terminal (**Ctrl+`**) and run:
   ```sh
   cd build && ./engine
   ```
4. Expected output (excerpt):

```
引擎启动，共加载 2158 条行情数据。
按 Ctrl+C 停止。

--- 第 1 轮 ---
2010-01-04  FLAT  ----  price: 30.5728  equity: 10000
2010-01-05  UP    ----  price: 30.6257  equity: 10000
...
  [买入成交] 339 股 @ 28.9357  手续费: 0.981211  剩余现金: 8.00556
2010-02-17  DOWN  BUY   price: 28.9357  equity: 9812.21
...
  [卖出成交] 339 股 @ 33.4159  手续费: 1.133  剩余现金: 11320.7
2010-05-14  UP    SELL  price: 33.4159  equity: 11320.7
...

本轮共成交 N 笔：
  2010-02-17  BUY   339 股  @ 28.9357  手续费: 0.981211
  2010-05-14  SELL  339 股  @ 33.4159  手续费: 1.133
  ...
最终净值: XXXXX  盈亏: XXXXX

数据播放完毕。日志已写入 data/signals.csv 和 data/trades.csv。重新开始...
```

5. Press **Ctrl+C** to stop.

---

**Step 6 — Verify the CSVs**

After at least one full cycle, open `data/trades.csv`. It should start with:

```
Date,Action,Price,Quantity,Commission,CashAfter
2010-02-17,BUY,28.9357,339,0.981211,8.00556
2010-05-14,SELL,33.4159,339,1.133,11320.7
...
```

Open `data/signals.csv` and confirm the `Equity` column is now present:

```
Date,Price,Signal,Equity
2010-01-04,30.5728,----,10000
2010-01-05,30.6257,----,10000
...
2010-02-17,28.9357,BUY ,9812.21
```

---

### Phase G — Commit and Push

**Step 7 — Commit your work**

1. Click the **Source Control** icon in the left sidebar.
2. Hover over **"Changes"** → click **"+"** to stage all files.
3. Type the commit message:
   ```
   Week 5: portfolio module, executeBuy/Sell, equity tracking, trades.csv
   ```
4. Click **"Commit"**.
5. Click **"Sync Changes"**.

---

## Milestone Checklist

- [ ] `src/portfolio.h` declares `Trade`, `Portfolio`, `createPortfolio`, `executeBuy`, `executeSell`, and `getEquity`
- [ ] `src/portfolio.cpp` implements all three functions with commission logic
- [ ] `src/engine.cpp` creates a `Portfolio` per cycle, calls `executeBuy`/`executeSell` on signals, prints `equity` each tick, writes `data/trades.csv`
- [ ] `CMakeLists.txt` includes `src/portfolio.cpp` in `add_executable`
- [ ] Program builds with no errors (`[100%] Built target engine`)
- [ ] Terminal shows `[买入成交]` / `[卖出成交]` messages at each signal
- [ ] `data/trades.csv` exists after one full cycle with at least one trade row
- [ ] `data/signals.csv` now includes an `Equity` column
- [ ] End-of-cycle summary prints total trades and final equity
- [ ] All changes are committed and visible on GitHub

---

## Reference: Files Changed This Week

| File                | Change                                                                          |
| ------------------- | ------------------------------------------------------------------------------- |
| `src/portfolio.h`   | New — declares `Trade`, `Portfolio`, `createPortfolio`, `executeBuy`, etc.      |
| `src/portfolio.cpp` | New — implements buy/sell execution with commission and equity calculation      |
| `src/engine.cpp`    | Updated — integrates portfolio; calls executeBuy/Sell; writes `data/trades.csv` |
| `CMakeLists.txt`    | Updated — adds `src/portfolio.cpp` to `add_executable`                          |

---

## Quick Reference: New C++ Features Used

| Feature                             | What it does                                                               |
| ----------------------------------- | -------------------------------------------------------------------------- |
| `Portfolio& p`                      | Non-`const` reference — lets the function modify the caller's variable     |
| `static constexpr double X = 0.001` | Compile-time constant with type; stays private to the `.cpp` file          |
| `std::floor(x)`                     | Returns the largest integer ≤ x (rounds down); requires `<cmath>`          |
| `static_cast<int>(x)`               | Explicit conversion from `double` to `int`; silences compiler warning      |
| `for (const Trade& t : v)`          | Range-based for loop — iterates every element of a vector without an index |
| `tradeLog.back()`                   | Returns a reference to the last element of the vector                      |
| `bool` return type                  | Lets a function signal success or failure to the caller with one bit       |
| Multiple `std::ofstream`            | Two CSV files can be written in parallel from the same function            |

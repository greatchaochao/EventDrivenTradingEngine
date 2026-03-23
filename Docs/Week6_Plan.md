# Week 6 — Quantitative Risk Management & Circuit Breakers

## Overview

**Theme:** Quantitative Risk Management & Circuit Breakers

By the end of this week, your engine will monitor its own risk in real time. When the account's loss from its historical peak exceeds a set threshold, an automatic **circuit breaker** fires: it forces a sell if in a position, and blocks all new buys until the cycle ends. The drawdown value is also written to `data/signals.csv` so it can be plotted in Week 7.

**Environment:** GitHub Codespaces (same environment as previous weeks)

**Time estimate:** 1.5 – 2 hours

---

## What You Will Build This Week

Two new files, one updated file:

1. **`src/risk.h`** — declares the `RiskManager` struct and two helper functions
2. **`src/risk.cpp`** — implements `createRiskManager` and `updateRisk`
3. **Updated `src/engine.cpp`** — creates a `RiskManager` each cycle, calls `updateRisk` on every tick, and enforces the circuit breaker

`CMakeLists.txt` gets a one-word update. `main.cpp`, `data_feed.*`, `strategy.*`, and `portfolio.*` are untouched.

New C++ concepts introduced:
- `std::max` — pick the larger of two values; used to track the running peak
- Boolean flag as a state machine — `halted` switches from `false` to `true` and stays there
- Hiding a helper with `static` inside a `.cpp` file
- Adding a new column to an existing CSV without changing any other module

---

## Background: What Is Drawdown?

**Drawdown** measures how far the account has fallen from its all-time high.

$$\text{drawdown} = \frac{\text{peak equity} - \text{current equity}}{\text{peak equity}}$$

| Example                         | Result        |
| ------------------------------- | ------------- |
| Peak = 12 000, Current = 12 000 | 0 % — at peak |
| Peak = 12 000, Current = 10 800 | 10 % drawdown |
| Peak = 12 000, Current = 10 200 | 15 % drawdown |

The **circuit breaker** is a simple rule: if drawdown ever exceeds a threshold (we use **10 %**), the engine:

1. Force-sells the position (if any) to stop further loss.
2. Sets a `halted` flag to `true`.
3. Rejects all subsequent BUY signals for the rest of the cycle.

This mimics the real-world risk controls used by quantitative hedge funds.

---

## Step-by-Step Plan

### Phase A — Update CMakeLists.txt

**Step 1 — Add risk.cpp to the build**

1. Open `CMakeLists.txt`.
2. Find the `add_executable` block and add `src/risk.cpp`:

```cmake
add_executable(engine
    src/main.cpp
    src/data_feed.cpp
    src/engine.cpp
    src/strategy.cpp
    src/portfolio.cpp
    src/risk.cpp
)
```

3. Press **Ctrl+S** to save.

> Only `src/risk.cpp` is new. The pattern is always the same: one new `.cpp` file per module.

---

### Phase B — Create the header file

**Step 2 — Create `src/risk.h`**

1. Right-click the `src/` folder → **"New File"** → name it `risk.h`.
2. Paste the following:

```cpp
#pragma once
// risk.h — 风控模块的声明文件
// 本文件定义了风险管理器所需的数据类型，并声明了两个函数
// 任何需要风控功能的文件，只需 #include "risk.h" 即可

#include <string>

// ─────────────────────────────────────────────
// RiskManager：实时记录账户的风险状态
//
//   peakEquity  — 账户历史最高净值（运行中只会增大，永不减小）
//   drawdown    — 当前回撤比例（0.0 ~ 1.0，例如 0.10 表示 10% 回撤）
//   maxDrawdown — 历史最大回撤（记录整个回测期间出现过的最大回撤）
//   halted      — 熔断标志：false = 正常运行；true = 已触发熔断，禁止开新仓
//   limit       — 触发熔断的回撤阈值（例如 0.10 表示亏损超过 10% 就熔断）
//
// 设计原则：
//   RiskManager 是一个纯数据容器，不做任何输出
//   所有更新逻辑都在下方的 updateRisk() 函数中完成
// ─────────────────────────────────────────────
struct RiskManager {
    double peakEquity;  // 历史最高净值
    double drawdown;    // 当前回撤（本 tick 的实时值）
    double maxDrawdown; // 历史最大回撤（整个周期内的峰值）
    bool   halted;      // 熔断开关（一旦为 true，本轮禁止买入）
    double limit;       // 回撤阈值（超过此值则触发熔断）
};

// ─────────────────────────────────────────────
// createRiskManager：初始化风险管理器
//
// 参数：
//   initialEquity — 初始账户净值（与 Portfolio 的初始资金保持一致）
//   limit         — 熔断回撤阈值，默认 0.10（即 10%）
//
// 返回：一个"峰值 = 初始资金、未熔断"的 RiskManager 对象
// ─────────────────────────────────────────────
RiskManager createRiskManager(double initialEquity, double limit = 0.10);

// ─────────────────────────────────────────────
// updateRisk：在每个 tick 更新风险状态
//
// 参数：
//   rm            — RiskManager 的引用（函数会修改它，所以用 & 而非 const &）
//   currentEquity — 本 tick 的账户总净值（来自 getNetValue）
//   date          — 本 tick 的日期字符串（仅用于打印提示信息）
//
// 函数内部逻辑（按顺序执行）：
//   1. 用 std::max 更新 peakEquity（峰值只升不降）
//   2. 计算 drawdown = (peak - current) / peak
//   3. 用 std::max 更新 maxDrawdown
//   4. 如果 drawdown >= limit 且尚未熔断，将 halted 设为 true 并打印提示
//
// 返回：void（无返回值，所有修改通过引用直接作用于调用方的变量）
// ─────────────────────────────────────────────
void updateRisk(RiskManager& rm, double currentEquity, const std::string& date);
```

3. Press **Ctrl+S** to save.

---

### Phase C — Create the implementation file

**Step 3 — Create `src/risk.cpp`**

1. Right-click the `src/` folder → **"New File"** → name it `risk.cpp`.
2. Paste the following:

```cpp
#include "risk.h"
#include <algorithm> // std::max — 返回两个值中较大的那个
#include <iostream>  // std::cout — 打印熔断提示

// ─────────────────────────────────────────────
// createRiskManager 实现
// ─────────────────────────────────────────────
RiskManager createRiskManager(double initialEquity, double limit) {
    RiskManager rm;
    rm.peakEquity  = initialEquity; // 初始峰值 = 初始资金
    rm.drawdown    = 0.0;           // 初始回撤为 0
    rm.maxDrawdown = 0.0;           // 初始历史最大回撤为 0
    rm.halted      = false;         // 初始状态：正常运行，未熔断
    rm.limit       = limit;         // 保存回撤阈值
    return rm;
}

// ─────────────────────────────────────────────
// updateRisk 实现
// ─────────────────────────────────────────────
void updateRisk(RiskManager& rm, double currentEquity, const std::string& date) {

    // ── 步骤 1：更新峰值 ──────────────────────────────────
    // std::max(a, b) 返回 a 和 b 中较大的那个
    // 如果当前净值超过了历史最高，就更新峰值；否则峰值不变
    // 注意：峰值只会单调递增，永远不会下降
    rm.peakEquity = std::max(rm.peakEquity, currentEquity);

    // ── 步骤 2：计算当前回撤 ──────────────────────────────
    // 公式：回撤 = (峰值 - 当前净值) / 峰值
    // 当 currentEquity == peakEquity 时（处于峰值），回撤 = 0
    // 当 currentEquity < peakEquity 时（处于回撤），回撤 > 0
    //
    // 特殊情况保护：如果 peakEquity == 0（理论上不应出现），
    // 避免触发除以 0 的未定义行为，直接将回撤设为 0
    if (rm.peakEquity > 0.0) {
        rm.drawdown = (rm.peakEquity - currentEquity) / rm.peakEquity;
    } else {
        rm.drawdown = 0.0;
    }

    // ── 步骤 3：更新历史最大回撤 ─────────────────────────
    // maxDrawdown 记录整个回测期间出现过的最大回撤值
    // 用 std::max 保持它只增不减
    rm.maxDrawdown = std::max(rm.maxDrawdown, rm.drawdown);

    // ── 步骤 4：检查是否触发熔断 ─────────────────────────
    // 条件：当前回撤 >= 阈值，且尚未熔断（避免重复打印提示）
    //
    // 布尔标志（Boolean Flag）作为状态机：
    //   halted 变量有两个状态：false（正常）和 true（已熔断）
    //   一旦切换到 true，本轮循环就不再切换回 false
    //   这模拟了真实熔断的"不可逆"特性——触发就锁定，直到下一轮重置
    if (rm.drawdown >= rm.limit && !rm.halted) {
        rm.halted = true; // 触发熔断，锁定状态
        std::cout << "  [!熔断触发] " << date
                  << "  回撤达到 " << rm.drawdown * 100.0 << "%"
                  << "，超过阈值 " << rm.limit * 100.0 << "%"
                  << "，本轮禁止开新仓。" << std::endl;
    }
}
```

3. Press **Ctrl+S** to save.

---

### Phase D — Update engine.cpp

**Step 4 — Integrate the risk manager into the engine loop**

The engine already handles market data, signals, and portfolio updates. This week, you add three targeted changes:

1. `#include "risk.h"` at the top.
2. Create `RiskManager rm = createRiskManager(10000.0)` alongside the portfolio at the start of each cycle.
3. After computing `netValue` each tick, call `updateRisk(rm, netValue, date)`. If `rm.halted`, force-sell any open position and skip BUY signals.
4. Add a `Drawdown` column to `signals.csv`.

Replace the entire contents of `src/engine.cpp` with:

```cpp
#include "engine.h"
#include "portfolio.h" // 投资组合模块（Week 5）
#include "risk.h"      // 新增：风控模块（Week 6）
#include "strategy.h"  // 信号计算模块（Week 4）
#include <chrono>
#include <fstream>
#include <iostream>
#include <thread>

void runEngine(const std::vector<Tick>& ticks) {
    std::cout << "引擎启动，共加载 " << ticks.size() << " 条行情数据。" << std::endl;
    std::cout << "按 Ctrl+C 停止。" << std::endl;
    std::cout << std::endl;

    int cycle = 1;

    while (true) {
        std::cout << "--- 第 " << cycle << " 轮 ---" << std::endl;

        // ── 每轮初始化账户与风控 ──────────────────────────
        // 两个模块使用相同的初始资金，保持数值一致
        Portfolio   portfolio = createPortfolio(10000.0);
        RiskManager rm        = createRiskManager(10000.0); // Week 6 新增

        // ── 每轮创建（或覆盖）日志文件 ─────────────────────
        std::ofstream sigCsv("data/signals.csv");
        std::ofstream tradeCsv("data/trades.csv");

        // signals.csv 新增 Drawdown 列，供 Week 7 绘制回撤曲线
        sigCsv   << "Date,Price,Signal,NetValue,Drawdown\n";
        tradeCsv << "Date,Action,Price,Quantity,Commission,CashAfter\n";

        std::vector<double> priceHistory;
        double prevPrice = ticks[0].price;

        for (int i = 0; i < (int)ticks.size(); i++) {
            double      price = ticks[i].price;
            std::string date  = ticks[i].date;

            // ── 方向判断 ────────────────────────────────────
            std::string direction;
            if      (price > prevPrice) direction = "UP  ";
            else if (price < prevPrice) direction = "DOWN";
            else                        direction = "FLAT";

            priceHistory.push_back(price);

            // ── 计算交易信号 ────────────────────────────────
            Signal sig = computeSignal(priceHistory);

            std::string sigStr;
            if      (sig == Signal::BUY)  sigStr = "BUY ";
            else if (sig == Signal::SELL) sigStr = "SELL";
            else                          sigStr = "----";

            // ── 订单执行（含熔断保护）──────────────────────
            //
            // 熔断时的两条规则：
            //   规则 1 — 禁止开新仓（跳过 BUY 信号）
            //   规则 2 — 强制平仓（若有持仓，立即执行卖出）
            //
            // 为什么只拦 BUY 而不拦 SELL？
            //   熔断的目的是"止损"，允许卖出可以及时关闭亏损头寸
            //   禁止买入是为了防止在下跌趋势中继续加仓，扩大损失
            if (rm.halted) {
                // 规则 2：如果熔断时仍有持仓，强制卖出
                if (portfolio.position > 0) {
                    bool sold = executeSell(portfolio, date, price);
                    if (sold) {
                        std::cout << "  [强制平仓] " << date
                                  << "  卖出 " << portfolio.tradeLog.back().quantity
                                  << " 股 @ " << price << std::endl;
                    }
                }
                // 规则 1：直接跳过后续的 BUY/SELL 判断
                // （continue 让循环跳到下一个 tick，但仍要先更新风控和写日志）
            } else {
                // 正常状态：按信号执行
                if (sig == Signal::BUY) {
                    bool traded = executeBuy(portfolio, date, price);
                    if (traded) {
                        std::cout << "  [买入成交] "
                                  << portfolio.tradeLog.back().quantity
                                  << " 股 @ " << price
                                  << "  手续费: " << portfolio.tradeLog.back().commission
                                  << "  剩余现金: " << portfolio.cash << std::endl;
                    }
                } else if (sig == Signal::SELL) {
                    bool traded = executeSell(portfolio, date, price);
                    if (traded) {
                        std::cout << "  [卖出成交] "
                                  << portfolio.tradeLog.back().quantity
                                  << " 股 @ " << price
                                  << "  手续费: " << portfolio.tradeLog.back().commission
                                  << "  剩余现金: " << portfolio.cash << std::endl;
                    }
                }
            }

            // ── 计算净值 ────────────────────────────────────
            double netValue = getNetValue(portfolio, price);

            // ── 更新风控状态（Week 6 新增）───────────────────
            // 必须在计算 netValue 之后调用，才能传入最新净值
            // updateRisk 内部会：更新峰值、计算回撤、检查熔断条件
            updateRisk(rm, netValue, date);

            // ── 终端输出 ─────────────────────────────────────
            std::cout << date << "  " << direction << "  " << sigStr
                      << "  price: " << price
                      << "  net_value: " << netValue
                      << "  drawdown: " << rm.drawdown * 100.0 << "%"
                      << (rm.halted ? "  [熔断中]" : "")
                      << std::endl;

            // ── 写入信号日志（含新的 Drawdown 列）──────────
            sigCsv << date << "," << price << "," << sigStr << ","
                   << netValue << "," << rm.drawdown << "\n";

            prevPrice = price;
            std::this_thread::sleep_for(std::chrono::milliseconds(50));
        }

        // ── 周期结束：写入成交记录 ────────────────────────────
        for (const Trade& t : portfolio.tradeLog) {
            tradeCsv << t.date << "," << t.action << "," << t.price << ","
                     << t.quantity << "," << t.commission << "," << t.cashAfter
                     << "\n";
        }

        // ── 打印本轮摘要 ──────────────────────────────────────
        std::cout << std::endl;
        std::cout << "本轮共成交 " << portfolio.tradeLog.size() << " 笔：" << std::endl;
        for (const Trade& t : portfolio.tradeLog) {
            std::cout << "  " << t.date << "  " << t.action << "  "
                      << t.quantity << " 股"
                      << "  @ " << t.price
                      << "  手续费: " << t.commission << std::endl;
        }

        double finalNetValue = getNetValue(portfolio, ticks.back().price);
        std::cout << "最终总价值: "   << finalNetValue
                  << "  盈亏: "       << (finalNetValue - 10000.0) << std::endl;

        // Week 6 新增：打印风控摘要
        std::cout << "历史最大回撤: " << rm.maxDrawdown * 100.0 << "%"
                  << (rm.halted ? "  （本轮已触发熔断）" : "  （本轮未触发熔断）")
                  << std::endl;

        std::cout << std::endl;
        std::cout << "数据播放完毕。日志已写入 data/signals.csv 和 data/trades.csv。重新开始..."
                  << std::endl;
        std::cout << std::endl;

        cycle++;
    }
}
```

3. Press **Ctrl+S** to save.

---

## Build & Run

**Step 5 — Rebuild and test**

Open the terminal in Codespaces and run:

```bash
cd build
cmake .. -DCMAKE_BUILD_TYPE=Debug
cmake --build .
./engine
```

**What to look for in the output:**

| Output line                                  | What it means                                |
| -------------------------------------------- | -------------------------------------------- |
| `[买入成交] 123 股 @ 28.50`                  | Normal buy, no risk issue                    |
| `[卖出成交] 123 股 @ 31.00`                  | Normal sell triggered by SELL signal         |
| `[!熔断触发] 2010-06-14  回撤达到 11.3%`     | Drawdown exceeded 10%; circuit breaker fires |
| `[强制平仓] 2010-06-14  卖出 123 股 @ 28.10` | Halted engine forced a sell to stop losses   |
| `历史最大回撤: 11.3%  （本轮已触发熔断）`    | End-of-cycle risk summary                    |

> **Note:** Whether the circuit breaker triggers depends on the data. If the SMA strategy happens to perform well this run, the drawdown may never reach 10% and `halted` stays `false`. That is correct behaviour — the circuit breaker is a safety net, not a guarantee.

---

## Concept Review

### New C++ Ideas This Week

**`std::max`**

```cpp
// std::max(a, b) 返回 a 和 b 中较大的那个
// 常用于"只升不降"的追踪场景（峰值、高水位线）
rm.peakEquity = std::max(rm.peakEquity, currentEquity);
// 等价于：
// if (currentEquity > rm.peakEquity) rm.peakEquity = currentEquity;
```

**Boolean flag as a state machine（布尔标志作为状态机）**

```cpp
// halted 只有两种状态：false（正常）和 true（已熔断）
// 一旦变为 true，本轮不再变回 false
// 这叫做"单向状态转换"，是系统设计中常见的保护模式
if (rm.drawdown >= rm.limit && !rm.halted) {
    rm.halted = true; // 状态切换：正常 → 熔断（不可逆）
}
```

**Why `static` in `.cpp` but not in `.h`?**

```cpp
// static 出现在 .cpp 文件里时，意思是"这个函数/变量只对本文件可见"
// 这是一种封装手段，防止其他文件意外调用内部工具函数
// 注意：这与 class 成员中的 static 含义完全不同
static constexpr double COMMISSION_RATE = 0.001; // 只在 portfolio.cpp 内可见
```

---

## This Week's Milestone

After a successful build and run you should see:

- [ ] Terminal prints a real-time `drawdown: X.X%` value on every tick
- [ ] When drawdown ≥ 10%, the line `[!熔断触发]` appears exactly once per cycle
- [ ] After the trigger, no new `[买入成交]` lines appear for the rest of that cycle
- [ ] End-of-cycle summary prints `历史最大回撤: X.X%`
- [ ] `data/signals.csv` has a fifth column `Drawdown` (ready for Week 7 plotting)

Congratulations — your engine now has institutional-level risk controls built in.

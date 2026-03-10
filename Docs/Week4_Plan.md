# Week 4 — Strategy Architecture & Alpha Signal Generation

## Overview

**Theme:** Strategy Architecture & Alpha Signal Generation

By the end of this week, your engine will produce real trading signals — **BUY**, **SELL**, or **HOLD** — on every tick, based on a classic SMA crossover strategy. Every signal is printed in the terminal and saved to `data/signals.csv`, which becomes the direct input to the Week 7 Python visualisation.

**Environment:** GitHub Codespaces (same environment as previous weeks)

**Time estimate:** 1.5 – 2 hours

---

## What You Will Build This Week

Two new files, one updated file:

1. **`src/strategy.h`** — declares `Signal` (enum) and `computeSignal` (function)
2. **`src/strategy.cpp`** — implements the SMA crossover logic
3. **Updated `src/engine.cpp`** — calls `computeSignal` each tick; writes `data/signals.csv`

`CMakeLists.txt` gets a one-word update. `main.cpp` and `data_feed.*` are untouched.

New C++ concepts introduced:
- `enum class` — a type-safe list of named constants
- `std::accumulate` — STL algorithm to sum a range of values
- `std::ofstream` — write data to a file
- Default function parameters — `int shortLen = 5`
- Static helper functions — `static` limits a function's visibility to one file

---

## Background: What Is an SMA Crossover?

A **Simple Moving Average (SMA)** is the arithmetic mean of the last *N* closing prices.

$$\text{SMA}(N) = \frac{P_{t} + P_{t-1} + \cdots + P_{t-N+1}}{N}$$

This strategy uses **two** moving averages:

| Line      | Window  | Role                                    |
| --------- | ------- | --------------------------------------- |
| Short SMA | 5 days  | Reacts quickly to recent price moves    |
| Long SMA  | 20 days | Smooths out noise; represents the trend |

**Signal rules:**

| Situation                                           | Signal   |
| --------------------------------------------------- | -------- |
| Short SMA crosses **above** Long SMA (Golden Cross) | **BUY**  |
| Short SMA crosses **below** Long SMA (Death Cross)  | **SELL** |
| No crossover                                        | **HOLD** |

The first 19 ticks always output `----` because there are not yet enough data points to compute the 20-day average.

---

## Step-by-Step Plan

### Phase A — Update CMakeLists.txt

**Step 1 — Add strategy.cpp to the build**

1. Open `CMakeLists.txt`.
2. Find the `add_executable` line and add `src/strategy.cpp`:

```cmake
add_executable(engine src/main.cpp src/data_feed.cpp src/engine.cpp src/strategy.cpp)
```

3. Press **Ctrl+S** to save.

> This is the only change to `CMakeLists.txt`. Every new module you add in future weeks follows the same pattern: add its `.cpp` file to this line.

---

### Phase B — Create the header file

**Step 2 — Create `src/strategy.h`**

1. Right-click the `src/` folder → **"New File"** → name it `strategy.h`.
2. Paste the following:

```cpp
#pragma once
// strategy.h — 策略模块的声明文件
// 本文件定义了策略所需的数据类型，并声明了信号计算函数
// 任何想要使用策略功能的文件，只需 #include "strategy.h" 即可

#include <vector>

// ─────────────────────────────────────────────
// Signal：枚举类，表示策略产生的三种信号
//   BUY  — 买入信号（短期均线从下方穿越长期均线）
//   SELL — 卖出信号（短期均线从上方穿越长期均线）
//   HOLD — 无操作（均线未发生交叉，持仓不变）
//
// 使用 "enum class" 而非普通 "enum" 的原因：
//   enum class 会把枚举值限定在自己的命名空间里
//   例如必须写 Signal::BUY，而不能直接写 BUY
//   这样可以避免与其他模块的同名变量发生冲突
// ─────────────────────────────────────────────
enum class Signal {
    BUY,   // 买入
    SELL,  // 卖出
    HOLD   // 观望
};

// ─────────────────────────────────────────────
// computeSignal：根据历史价格序列，计算当前时刻的交易信号
//
// 参数：
//   prices   — 截至当前时刻的全部收盘价（最新价格在末尾）
//   shortLen — 短期均线的窗口长度，默认 5 天
//   longLen  — 长期均线的窗口长度，默认 20 天
//
// 返回值：Signal::BUY / Signal::SELL / Signal::HOLD
//
// 策略逻辑（SMA 均线交叉）：
//   当 prices 数量不足 longLen 时，返回 HOLD（数据不够，无法判断）
//   当 shortSMA 从上一刻的"低于 longSMA"变为"高于 longSMA" → BUY
//   当 shortSMA 从上一刻的"高于 longSMA"变为"低于 longSMA" → SELL
//   其余情况 → HOLD
// ─────────────────────────────────────────────
Signal computeSignal(const std::vector<double>& prices,
                     int shortLen = 5,
                     int longLen  = 20);
```

3. Press **Ctrl+S** to save.

> **Default parameters** (`= 5`, `= 20`): when a caller writes `computeSignal(prices)` without specifying lengths, the compiler automatically uses 5 and 20. This makes the common case short to write while still allowing custom windows.

---

### Phase C — Create the implementation file

**Step 3 — Create `src/strategy.cpp`**

1. Right-click the `src/` folder → **"New File"** → name it `strategy.cpp`.
2. Paste the following:

```cpp
#include "strategy.h"
#include <numeric>  // std::accumulate — 用于对区间求和

// ─────────────────────────────────────────────
// sma：计算简单移动平均线（Simple Moving Average）
//
// 参数：
//   prices — 完整的历史价格序列
//   len    — 窗口长度（取序列末尾 len 个价格来计算均值）
//
// 返回值：过去 len 天收盘价的算术平均值
//
// 例如：prices = {30.0, 31.0, 29.0, 32.0, 30.5}，len = 3
//   取末尾 3 个 → {29.0, 32.0, 30.5}
//   总和 = 91.5，平均 = 30.5
//
// std::accumulate 的用法：
//   std::accumulate(起始迭代器, 结束迭代器, 初始值)
//   从起始位置到结束位置依次相加，最后加上初始值，返回总和
//   这里用 prices.end() - len 定位到倒数第 len 个元素
//
// static 关键字：
//   将这个函数标记为"仅在本文件内可见"
//   其他文件无法调用 sma()，它是 strategy.cpp 的私有工具函数
// ─────────────────────────────────────────────
static double sma(const std::vector<double>& prices, int len) {
    // prices.end() - len  指向序列倒数第 len 个元素
    // prices.end()        指向序列末尾（最后一个元素的下一位）
    double sum = std::accumulate(prices.end() - len, prices.end(), 0.0);
    return sum / len;  // 除以窗口长度得到平均值
}

// ─────────────────────────────────────────────
// computeSignal 实现
// ─────────────────────────────────────────────
Signal computeSignal(const std::vector<double>& prices,
                     int shortLen,
                     int longLen) {

    // ── 条件 1：数据不足 ──────────────────────
    // 如果当前拿到的价格条数少于长期窗口，
    // 根本无法计算 20 日均线，直接返回 HOLD
    if ((int)prices.size() < longLen) {
        return Signal::HOLD;
    }

    // ── 计算"此时此刻"的短期和长期均线 ────────
    double shortNow = sma(prices, shortLen);  // 当前 5 日均线
    double longNow  = sma(prices, longLen);   // 当前 20 日均线

    // ── 条件 2：数据只够计算一个截面 ──────────
    // 需要对比"上一刻"和"这一刻"，至少要有 longLen+1 条数据
    // 否则上一刻的均线无法计算，同样返回 HOLD
    if ((int)prices.size() < longLen + 1) {
        return Signal::HOLD;
    }

    // ── 计算"上一时刻"的短期和长期均线 ────────
    // 方法：从 prices 中去掉最后一个元素，得到"昨天"的价格序列
    // 用 std::vector 的范围构造：取 prices 的 [0, size-1) 区间
    std::vector<double> prevPrices(prices.begin(), prices.end() - 1);
    double shortPrev = sma(prevPrices, shortLen);  // 上一日 5 日均线
    double longPrev  = sma(prevPrices, longLen);   // 上一日 20 日均线

    // ── 判断均线交叉 ──────────────────────────
    // 黄金交叉（Golden Cross）：短期均线从下方穿越长期均线 → BUY
    //   上一刻：shortPrev <= longPrev（短线在长线下方或相等）
    //   这一刻：shortNow  >  longNow（短线升到长线上方）
    if (shortPrev <= longPrev && shortNow > longNow) {
        return Signal::BUY;
    }

    // 死亡交叉（Death Cross）：短期均线从上方穿越长期均线 → SELL
    //   上一刻：shortPrev >= longPrev（短线在长线上方或相等）
    //   这一刻：shortNow  <  longNow（短线跌到长线下方）
    if (shortPrev >= longPrev && shortNow < longNow) {
        return Signal::SELL;
    }

    // 没有发生交叉 → HOLD（观望，不做任何操作）
    return Signal::HOLD;
}
```

3. Press **Ctrl+S** to save.

---

### Phase D — Update engine.cpp

**Step 4 — Integrate the strategy into the engine loop**

The existing `while (true)` loop and `prevPrice` / direction logic stay exactly as they are. You only need to add four things:

1. Two new `#include` lines at the top (`"strategy.h"` and `<fstream>`).
2. A `std::ofstream csv(...)` opened once per cycle to write `data/signals.csv`.
3. A `std::vector<double> priceHistory` that accumulates prices tick by tick.
4. A `computeSignal` call on each tick, followed by a `csv <<` write.

The updated `engine.cpp` in full:

```cpp
#include "engine.h"
#include "strategy.h"   // 引入信号计算函数 computeSignal 和 Signal 枚举
#include <chrono>
#include <fstream>      // std::ofstream — 用于写入 CSV 日志文件
#include <iostream>
#include <thread>

void runEngine(const std::vector<Tick> &ticks) {
    std::cout << "引擎启动，共加载 " << ticks.size() << " 条行情数据。" << std::endl;
    std::cout << "按 Ctrl+C 停止。" << std::endl;
    std::cout << std::endl;

    int cycle = 1;

    while (true) {
        std::cout << "--- 第 " << cycle << " 轮 ---" << std::endl;

        // ── 每轮开始时创建（或覆盖）信号日志文件 ──────────────
        // std::ofstream 以写入模式打开文件；若文件已存在则从头覆盖
        // 变量 csv 在本轮 while 循环体结束时自动关闭（RAII）
        std::ofstream csv("data/signals.csv");
        csv << "Date,Price,Signal\n";  // 写入 CSV 表头（第一行）

        // ── 价格历史缓冲区，每轮循环重置 ────────────────────
        // priceHistory 存储截至当前 tick 的所有收盘价（按时间顺序）
        // computeSignal 需要读取这段历史才能计算短期和长期均线
        std::vector<double> priceHistory;

        double prevPrice = ticks[0].price;

        for (int i = 0; i < (int)ticks.size(); i++) {
            double price = ticks[i].price;

            // 涨跌方向（沿用第三周逻辑）
            std::string direction;
            if      (price > prevPrice) direction = "UP  ";
            else if (price < prevPrice) direction = "DOWN";
            else                        direction = "FLAT";

            // ── 将本 tick 的价格追加进历史缓冲区 ──────────────
            // 追加后 priceHistory 包含从第 1 天到当前日的所有价格
            priceHistory.push_back(price);

            // ── 计算当前 tick 的交易信号 ───────────────────────
            // 不足 20 条 → HOLD；20 条以上 → 判断均线是否交叉
            Signal sig = computeSignal(priceHistory);

            // ── 将 Signal 枚举转换为可打印的字符串 ─────────────
            // "BUY " 刻意加一个空格，与 "SELL" / "----" 对齐
            std::string sigStr;
            if      (sig == Signal::BUY)  sigStr = "BUY ";
            else if (sig == Signal::SELL) sigStr = "SELL";
            else                          sigStr = "----";

            std::cout << ticks[i].date << "  " << direction
                      << "  " << sigStr << "  price: " << price << std::endl;

            // ── 将本行数据写入 CSV 日志 ───────────────────────
            csv << ticks[i].date << "," << price << "," << sigStr << "\n";

            prevPrice = price;

            std::this_thread::sleep_for(std::chrono::milliseconds(50));
        }

        // csv 在此离开作用域，ofstream 析构函数自动 flush 并关闭文件
        std::cout << std::endl;
        std::cout << "数据播放完毕。信号日志已写入 data/signals.csv。重新开始..." << std::endl;
        std::cout << std::endl;
        cycle++;
    }
}
```

> **RAII（Resource Acquisition Is Initialization）**  
> `std::ofstream csv(...)` 在构造时打开文件，在析构时（变量离开作用域时）自动关闭。你不需要手动调用 `csv.close()`——C++ 替你处理。这是 C++ 管理资源的核心惯用法。

---

## Code Walkthrough

### `enum class Signal` vs plain `enum`

```cpp
enum class Signal { BUY, SELL, HOLD };
```

A plain `enum` leaks its names into the surrounding scope — `BUY` could clash with a variable named `BUY` in another part of your code. `enum class` requires `Signal::BUY`, making it impossible to confuse with anything else. Use `enum class` by default.

---

### `std::accumulate` — summing a range

```cpp
#include <numeric>
double sum = std::accumulate(prices.end() - len, prices.end(), 0.0);
```

`std::accumulate(first, last, initial_value)` adds every element from `first` to `last` onto `initial_value` and returns the total. It replaces a manual for-loop:

```cpp
// 等价的手动写法（勿用，accumulate 更清晰）
double sum = 0.0;
for (int i = prices.size() - len; i < (int)prices.size(); i++)
    sum += prices[i];
```

---

### Detecting a crossover

```cpp
// 上一刻
std::vector<double> prevPrices(prices.begin(), prices.end() - 1);
double shortPrev = sma(prevPrices, shortLen);
double longPrev  = sma(prevPrices, longLen);

// 这一刻
double shortNow = sma(prices, shortLen);
double longNow  = sma(prices, longLen);

// 黄金交叉：短线从下方越过长线
if (shortPrev <= longPrev && shortNow > longNow) return Signal::BUY;

// 死亡交叉：短线从上方跌破长线
if (shortPrev >= longPrev && shortNow < longNow) return Signal::SELL;
```

A crossover requires two snapshots in time. By comparing `[prev state]` and `[current state]`, the function detects the exact moment when the two lines swap positions — exactly one BUY or SELL fires per crossing, no matter how many ticks the lines stay on the same side.

---

### Writing a CSV file

```cpp
std::ofstream csv("data/signals.csv");
csv << "Date,Price,Signal\n";          // 表头
csv << ticks[i].date << "," << price << "," << sigStr << "\n";  // 数据行
```

`std::ofstream` works exactly like `std::cout` but sends output to a file instead of the terminal. The `<<` operator is identical. When `csv` goes out of scope at the end of the loop body, C++ automatically flushes and closes the file.

---

## Build and Run

**Step 5 — Build and run**

1. Click **⚙ Build** in the blue status bar.
   ```
   [100%] Built target engine
   ```
2. Open the terminal (**Ctrl+`**) and run:
   ```sh
   cd build && ./engine
   ```
3. Expected output (first 25 rows):
   ```
   事件驱动交易引擎
   ----------------
   引擎启动，共加载 2138 条行情数据。
   按 Ctrl+C 停止。

   --- 第 1 轮 ---
   2010-01-04  FLAT  ----  price: 30.5728
   2010-01-05  UP    ----  price: 30.6257
   ...（前 20 行全部显示 ----，数据不足以计算长期均线）
   2010-02-17  DOWN  BUY   price: 28.9357   ← 首个 BUY 信号
   ...
   数据播放完毕。信号日志已写入 data/signals.csv。重新开始...
   ```
4. Press **Ctrl+C** to stop.

**Step 6 — Verify the CSV**

After at least one full cycle, open `data/signals.csv`. It should start with:

```
Date,Price,Signal
2010-01-04,30.5728,----
2010-01-05,30.6257,----
...
2010-02-17,28.9357,BUY
```

---

## Commit and Push

**Step 7 — Commit your work**

1. Click the **Source Control** icon in the left sidebar.
2. Hover over **"Changes"** → click **"+"** to stage all files.
3. Type the commit message:
   ```
   Week 4: SMA crossover strategy, BUY/SELL/HOLD signals, signals.csv output
   ```
4. Click **"Commit"**.
5. Click **"Sync Changes"**.

---

## Milestone Checklist

- [ ] `src/strategy.h` declares `Signal` enum and `computeSignal` function
- [ ] `src/strategy.cpp` implements `sma` helper and `computeSignal` with crossover logic
- [ ] `src/engine.cpp` includes `strategy.h`, accumulates `priceHistory`, calls `computeSignal`, writes `data/signals.csv`
- [ ] `CMakeLists.txt` includes `src/strategy.cpp` in `add_executable`
- [ ] Program builds with no errors (`[100%] Built target engine`)
- [ ] First 19 rows print `----`; first BUY or SELL signal appears by row 21
- [ ] `data/signals.csv` exists after one full cycle with 2138+ data rows and a header
- [ ] All changes are committed and visible on GitHub

---

## Reference: Files Changed This Week

| File               | Change                                                                      |
| ------------------ | --------------------------------------------------------------------------- |
| `src/strategy.h`   | New — declares `Signal` enum and `computeSignal` function                   |
| `src/strategy.cpp` | New — implements SMA helper and crossover signal logic                      |
| `src/engine.cpp`   | Updated — integrates `computeSignal`; accumulates price history; writes CSV |
| `CMakeLists.txt`   | Updated — adds `src/strategy.cpp` to `add_executable`                       |

---

## Quick Reference: New C++ Features Used

| Feature                                  | What it does                                                            |
| ---------------------------------------- | ----------------------------------------------------------------------- |
| `enum class Signal { BUY, SELL, HOLD };` | Type-safe named constants; accessed as `Signal::BUY`                    |
| `#include <numeric>`                     | Brings in `std::accumulate` and other numeric algorithms                |
| `std::accumulate(first, last, init)`     | Sums all elements in a range; replaces a manual for-loop sum            |
| `static double sma(...)`                 | Limits function visibility to the current `.cpp` file                   |
| `int shortLen = 5`                       | Default parameter; caller can omit the argument to use the default      |
| `std::ofstream csv("file.csv")`          | Opens a file for writing; `<<` sends text to it just like `std::cout`   |
| `csv << value << "\n"`                   | Writes a value followed by a newline to the CSV file                    |
| RAII (automatic file close)              | `ofstream` closes and flushes the file when it goes out of scope        |
| `std::vector<double> priceHistory`       | Accumulates all prices seen so far; passed to `computeSignal` each tick |
| `priceHistory.push_back(price)`          | Appends the current price to the history buffer                         |
| `sig == Signal::BUY`                     | Compares an `enum class` value using `==`                               |

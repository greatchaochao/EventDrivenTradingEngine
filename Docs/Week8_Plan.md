# Week 8 — Final Deployment & Digital Portfolio Packaging

## Overview

**Theme:** Final Deployment & Digital Portfolio Packaging  
**Session Length:** 60 minutes  
**Format:** Tutor-guided, student executes all steps

By the end of this session the student will walk away with two concrete, showcaseable deliverables:

1. **A professional `README.md`** — the front page of the GitHub portfolio, presenting the full system architecture, quick-start guide, and module reference.
2. **A finalized `data/report.png`** — the performance white-paper chart generated automatically by the system itself, proving the engine runs end-to-end.

Everything built across Weeks 1–7 feeds into these two outputs. No new C++ code is needed.

---

## Session Roadmap

| #   | Phase                         | Goal                                            | Time   |
| --- | ----------------------------- | ----------------------------------------------- | ------ |
| A   | System Sanity Check           | Confirm the full pipeline compiles and runs     | 10 min |
| B   | Write the README              | Build the portfolio front page                  | 25 min |
| C   | Enhance the Report            | Add a summary stats annotation to the chart     | 15 min |
| D   | Final Generation & Git Commit | Produce the deliverable and seal the repository | 10 min |

---

## Phase A — System Sanity Check (10 min)

> **Goal:** Start the session by verifying the full pipeline works. This grounds the student in the complete system before packaging it.

### Task A1 — Build and run the engine

Open a terminal in the project root directory and run:

```bash
cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
cmake --build .
./engine
```

Watch the output. Confirm you can see:
- Tick-by-tick price data flowing (`UP` / `DOWN` / `FLAT`)
- `BUY` / `SELL` signals appearing in the log
- Drawdown percentage updating each tick
- A circuit-breaker `[熔断中]` line if drawdown exceeds 10%

Once the terminal prints `数据播放完毕。日志已写入 data/signals.csv 和 data/trades.csv。重新开始...`, press **Ctrl+C** to stop.

### Task A2 — Run the Python report

Back in the project root:

```bash
cd ..
python3 scripts/report.py
```

Expected output:
```
已读取 756 条行情记录，N 条成交记录。
报告已生成：data/report.png
```

Open `data/report.png` in VS Code (click the file). Confirm all three sub-charts render correctly:
- ① Blue equity curve
- ② Red drawdown fill with the 10% threshold line
- ③ Gray price line with green `▲` BUY and red `▼` SELL markers

**Checkpoint:** If both commands succeed and the chart looks correct, the pipeline is healthy — move to Phase B.

---

## Phase B — Write the README (25 min)

> **Goal:** Replace the placeholder README with a professional portfolio document. This is the most important deliverable of the entire project.

### Task B1 — Open `README.md`

Open `EventDrivenTradingEngine/README.md`. It currently contains only:
```
# EventDrivenTradingEngine
```

You will replace its entire content with the document below.

### Task B2 — Replace the README content

Select all and replace with the following (read through each section as you paste — the tutor will explain each part):

```markdown
# Event-Driven Trading Engine

A fully self-contained, event-driven backtesting engine written in **C++**, with a **Python** visualization pipeline that automatically generates a professional performance report.

Built across 8 weeks as a structured engineering project simulating the workflow of a quantitative hedge fund development team.

---

## System Architecture

```
┌─────────────────────────────────────────────────────────────┐
│                        main.cpp                             │
│              Entry point — wires all modules                │
└───────────────────────┬─────────────────────────────────────┘
                        │
        ┌───────────────▼────────────────┐
        │          data_feed             │
        │  Reads tr_eikon_eod_data.csv   │
        │  Produces: vector<Tick>        │
        └───────────────┬────────────────┘
                        │
        ┌───────────────▼────────────────┐
        │            engine              │
        │  Tick-by-tick simulation loop  │
        │  Orchestrates all modules      │
        └──┬──────────┬──────────┬───────┘
           │          │          │
    ┌──────▼──┐  ┌────▼────┐  ┌─▼────────┐
    │strategy │  │portfolio│  │   risk   │
    │SMA cross│  │Cash +   │  │Drawdown +│
    │→ Signal │  │Holdings │  │Circuit   │
    └─────────┘  └────┬────┘  │Breaker   │
                      │       └──────────┘
              ┌───────▼────────────────┐
              │    data/signals.csv    │
              │    data/trades.csv     │
              └───────┬────────────────┘
                      │
              ┌───────▼────────────────┐
              │  scripts/report.py     │
              │  Python + Matplotlib   │
              │  → data/report.png     │
              └────────────────────────┘
```

---

## Modules

| File                | Language | Responsibility                                       |
| ------------------- | -------- | ---------------------------------------------------- |
| `src/data_feed.cpp` | C++      | Parses CSV into `Tick` structs                       |
| `src/strategy.cpp`  | C++      | SMA crossover → BUY / SELL / HOLD signal             |
| `src/portfolio.cpp` | C++      | Cash & holdings management, order execution          |
| `src/risk.cpp`      | C++      | Max drawdown tracking, circuit-breaker halting       |
| `src/engine.cpp`    | C++      | Main event loop, writes `signals.csv` & `trades.csv` |
| `scripts/report.py` | Python   | Reads CSVs, renders 3-panel performance chart        |

---

## Strategy

The engine implements a classic **Simple Moving Average (SMA) Crossover** strategy:

- **Short window:** 5 days
- **Long window:** 20 days
- **BUY signal:** short SMA crosses above long SMA (upward momentum)
- **SELL signal:** short SMA crosses below long SMA (downward momentum)

A **circuit breaker** halts all new BUY orders if the portfolio drawdown exceeds **10%**, and forces a market SELL of any open position.

---

## Quick Start

### Prerequisites

- CMake ≥ 3.10
- A C++23-compatible compiler (clang++ or g++)
- Python 3 with `pandas` and `matplotlib`

```bash
pip install pandas matplotlib
```

### 1 — Build

```bash
cd build
cmake ..
cmake --build .
```

### 2 — Run the Engine

```bash
./build/engine
```

The engine will loop continuously through the dataset. Press **Ctrl+C** after one complete pass (when the terminal prints `数据播放完毕`).

Output files written to `data/`:
- `signals.csv` — per-tick price, signal, net value, and drawdown
- `trades.csv` — every executed order with price, quantity, and commission

### 3 — Generate the Performance Report

```bash
python3 scripts/report.py
```

Opens → `data/report.png`

---

## Performance Report

The report is a three-panel chart automatically generated from the engine's output:

| Panel | Chart                 | What it shows                                             |
| ----- | --------------------- | --------------------------------------------------------- |
| ①     | Equity Curve          | Portfolio net value vs. initial capital of $10,000        |
| ②     | Drawdown              | Current drawdown % with the 10% circuit-breaker threshold |
| ③     | Price + Trade Signals | Raw price with ▲ BUY and ▼ SELL markers                   |

---

## Project Structure

```
EventDrivenTradingEngine/
├── src/
│   ├── main.cpp          # Entry point
│   ├── data_feed.cpp/h   # Market data ingestion
│   ├── engine.cpp/h      # Event-driven simulation loop
│   ├── strategy.cpp/h    # Alpha signal generation
│   ├── portfolio.cpp/h   # Order management system
│   └── risk.cpp/h        # Quantitative risk management
├── scripts/
│   └── report.py         # Python visualization pipeline
├── data/
│   ├── tr_eikon_eod_data.csv  # Historical price data (Apple Inc.)
│   ├── signals.csv            # Engine output: per-tick log
│   ├── trades.csv             # Engine output: trade audit log
│   └── report.png             # Auto-generated performance chart
├── Docs/
│   └── Project_Development_Plan.md
├── build/                # CMake build directory
└── CMakeLists.txt
```

---

## Key Concepts Demonstrated

- **Event-driven architecture** — the engine does not look ahead; each tick is processed in strict time order
- **Strategy/execution separation** — `strategy.cpp` only generates signals; `engine.cpp` decides whether to act on them
- **Institutional risk controls** — drawdown-based circuit breaker mirrors real hedge fund risk limits
- **Reproducible results** — all outputs are deterministic and written to auditable CSV files
- **Cross-language pipeline** — C++ handles performance-critical simulation; Python handles visualization
```

Save the file with **Ctrl+S**.

### Task B3 — Review the README together

With the tutor, walk through each section of the README and answer these questions:

1. **Architecture diagram:** Can you trace the flow of a single price tick from `data_feed` all the way to `report.png`? Point to each box in the diagram and name the corresponding file.

2. **Modules table:** For each row, open the corresponding `.h` file and find the function declaration that matches the "Responsibility" column.

3. **Strategy section:** What would happen to the BUY signal if you changed `shortLen` from 5 to 3? Would the strategy trade more or less frequently?

4. **Quick Start:** Close the README, open a fresh terminal, and follow only the README instructions to rebuild and re-run the engine. If you can do it without help, the documentation is complete.

---

## Phase C — Enhance the Report with Summary Stats (15 min)

> **Goal:** Add a printed summary block at the end of `report.py` that quantifies strategy performance in plain numbers. This turns the visual chart into a complete white-paper.

### Task C1 — Understand the existing data

Before writing any code, answer these questions by inspecting `data/signals.csv` and `data/trades.csv`:

- What is the first and last date in the dataset?
- What is the starting net value? What is the final net value?
- How many rows are in `trades.csv`?

### Task C2 — Add a summary stats block to `report.py`

Open `scripts/report.py`. Add the following block **at the very end of the file**, after the final `print(...)` line:

```python
# ── 第六步：打印绩效摘要 ──────────────────────────────────────────────────
#
# 从 signals DataFrame 中直接计算关键绩效指标
# 这些数字与图表图像构成完整的"绩效白皮书"

initial_capital = 10000.0

# 最终净值：取 signals 最后一行的 NetValue
final_net_value = signals["NetValue"].iloc[-1]

# 总收益率（百分比）
total_return_pct = (final_net_value - initial_capital) / initial_capital * 100

# 历史最大回撤（来自 Drawdown 列的最大值）
max_drawdown_pct = signals["Drawdown"].max() * 100

# 总成交笔数
total_trades = len(trades)

# 数据起止日期
start_date = signals["Date"].iloc[0].strftime("%Y-%m-%d")
end_date   = signals["Date"].iloc[-1].strftime("%Y-%m-%d")

print("\n" + "=" * 50)
print("  PERFORMANCE SUMMARY")
print("=" * 50)
print(f"  Period          : {start_date}  →  {end_date}")
print(f"  Initial Capital : ${initial_capital:,.2f}")
print(f"  Final Net Value : ${final_net_value:,.2f}")
print(f"  Total Return    : {total_return_pct:+.2f}%")
print(f"  Max Drawdown    : {max_drawdown_pct:.2f}%")
print(f"  Total Trades    : {total_trades}")
print("=" * 50 + "\n")
```

Save the file.

### Task C3 — Run the updated script and read the summary

```bash
python3 scripts/report.py
```

You should now see a block like:

```
==================================================
  PERFORMANCE SUMMARY
==================================================
  Period          : 2010-01-04  →  2012-12-31
  Initial Capital : $10,000.00
  Final Net Value : $10,xxx.xx
  Total Return    : +x.xx%
  Max Drawdown    : x.xx%
  Total Trades    : N
==================================================
```

With the tutor, interpret the numbers:

- Is the total return positive? What does that mean about the SMA strategy on this dataset?
- How does the max drawdown compare to the 10% circuit-breaker threshold? Did the breaker fire?
- Is the number of trades reasonable for a 3-year dataset using a 5/20 SMA crossover?

---

## Phase D — Final Generation & Git Commit (10 min)

> **Goal:** Seal the repository with a clean, final commit that represents the completed project.

### Task D1 — Run the full pipeline one final time

This produces the definitive output files:

```bash
# Step 1: Run the engine for one complete pass
cd build && ./engine
# Press Ctrl+C after seeing: 数据播放完毕。日志已写入...

# Step 2: Generate the final report
cd ..
python3 scripts/report.py
```

Verify `data/report.png` is up to date (check the file's modification time).

### Task D2 — Review what has been built

Take 2 minutes to open the file tree and, for each file, say out loud what it does:

```
src/data_feed.cpp    → "reads the CSV and returns Tick objects"
src/strategy.cpp     → "computes SMA crossover signals"
src/portfolio.cpp    → "manages cash, holdings, and order execution"
src/risk.cpp         → "tracks drawdown and triggers the circuit breaker"
src/engine.cpp       → "the main loop that drives everything"
scripts/report.py    → "reads CSVs and generates the performance chart"
README.md            → "explains the whole system to a new reader"
```

If you can say this without hesitation, you understand the architecture.

### Task D3 — Commit to Git

```bash
git add README.md scripts/report.py data/report.png
git commit -m "Week 8: finalize README, add summary stats, generate final report"
```

> **Note:** `data/signals.csv` and `data/trades.csv` are engine outputs that change on every run — they are intentionally not committed (add them to `.gitignore` if not already there).

### Task D4 — (Optional) Tag the release

```bash
git tag -a v1.0 -m "Completed Event-Driven Trading Engine — 8-week project"
```

---

## Milestone

**Project officially complete.** The student now holds:

| Deliverable                   | Location            | What it proves                                |
| ----------------------------- | ------------------- | --------------------------------------------- |
| Full C++ trading engine       | `src/`              | Systems programming, OOP, event-driven design |
| Python visualization pipeline | `scripts/report.py` | Cross-language integration, data analysis     |
| Professional README           | `README.md`         | Technical communication and documentation     |
| Performance report chart      | `data/report.png`   | End-to-end reproducible research output       |
| Git history                   | repository          | Engineering discipline and version control    |

---

## Tutor Notes

**Pacing guide:**
- Phase A should feel fast — the student built this; let them lead.
- Phase B is the longest phase. Do not rush it. The README is the portfolio centerpiece.
- In Task B3, the "close the README and follow only the instructions" test is the most important check of the whole session — it validates that the documentation is genuinely self-sufficient.
- Phase C introduces one new Python concept (`.iloc[-1]`, f-string formatting). Walk through it slowly.
- Phase D should feel like a celebration — the student is sealing something they built from scratch.

**If ahead of schedule:**  
Add a `.gitignore` file that excludes `build/`, `data/signals.csv`, and `data/trades.csv`. Discuss *why* generated outputs are typically excluded from version control.

**Discussion questions for the session wrap-up:**
1. What would you change about the SMA strategy to make it more profitable?
2. What would break first if you fed in real-time data instead of historical CSV?
3. If a recruiter or admissions officer opens your GitHub — what is the first thing they will see, and what does it tell them about you?

# Week 3 — Event-Driven Core & Time Simulation

## Overview

**Theme:** Event-Driven Core & Time Simulation

By the end of this week, your engine will have a proper simulation loop: it steps through every tick one by one, tracks the current price, and flags whether the market went up or down at each step. This is the "heartbeat" that every future module — signals, orders, risk — will plug into.

**Environment:** GitHub Codespaces (same environment as previous weeks)

**Time estimate:** 1.5 – 2 hours

---

## What You Will Build This Week

Three things:

1. **`src/engine.h`** — declares the `runEngine` function
2. **`src/engine.cpp`** — implements the tick-by-tick simulation loop
3. **Updated `src/main.cpp`** — now just loads data and hands it to the engine

`CMakeLists.txt` also gets one small update to include `engine.cpp`.

New C++ concepts introduced:
- Passing a `const` reference — share data without copying it
- Comparing values across loop iterations — track previous price
- `if / else if / else` — decide whether price rose, fell, or stayed flat
- `std::this_thread::sleep_for` — pause between ticks to simulate a live feed
- `while (true)` — keep the simulation running until Ctrl+C

---

## Background: What Is an Event-Driven Loop?

In a real trading system, the engine does not process all data at once. It processes **one event at a time** — in our case, one tick at a time — in strict time order. At each step it asks: *"given the current market state, what should happen?"*

This week the answer is simply: *print the price and note the direction*. In Week 4 the answer will be: *generate a BUY or SELL signal*. In Week 5: *execute an order*. The loop itself never changes — only what happens inside it.

---

## Step-by-Step Plan

### Phase A — Update CMakeLists.txt

**Step 1 — Add engine.cpp to the build**

1. Open `CMakeLists.txt`.
2. Replace the entire contents with:

```cmake
cmake_minimum_required(VERSION 3.10)
project(EventDrivenTradingEngine)
set(CMAKE_CXX_STANDARD 23)

add_executable(engine src/main.cpp src/data_feed.cpp src/engine.cpp)
target_include_directories(engine PRIVATE src)

# Symlink build/data -> project data/ so the binary finds the CSV via relative path
add_custom_command(TARGET engine POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E create_symlink
        ${CMAKE_SOURCE_DIR}/data
        ${CMAKE_BINARY_DIR}/data
    COMMENT "Symlinking build/data -> project data/")
```

3. Press **Ctrl+S** to save.

> The only change from Week 2 is `src/engine.cpp` added to the `add_executable` line.

---

### Phase B — Create the header file

**Step 2 — Create `src/engine.h`**

1. Right-click the `src/` folder → **"New File"** → name it `engine.h`.
2. Paste the following:

```cpp
#pragma once
#include "data_feed.h"
#include <vector>

// Run the tick-by-tick simulation loop.
// Prints each tick with a direction indicator (UP / DOWN / FLAT).
// Loops continuously until Ctrl+C.
void runEngine(const std::vector<Tick>& ticks);
```

3. Press **Ctrl+S** to save.

> **`const std::vector<Tick>&`** — the `&` means we pass a reference (no copy of 2,000+ ticks), and `const` means the engine promises not to modify the data. This is the standard way to pass large containers in C++.

---

### Phase C — Create the implementation file

**Step 3 — Create `src/engine.cpp`**

1. Right-click the `src/` folder → **"New File"** → name it `engine.cpp`.
2. Paste the following:

```cpp
#include "engine.h"
#include <chrono>
#include <iostream>
#include <thread>

void runEngine(const std::vector<Tick>& ticks) {
    std::cout << "Engine started. Processing " << ticks.size() << " ticks." << std::endl;
    std::cout << "Press Ctrl+C to stop." << std::endl;
    std::cout << std::endl;

    int cycle = 1;

    while (true) {
        std::cout << "--- Cycle " << cycle << " ---" << std::endl;

        double prevPrice = ticks[0].price;  // remember the price from the previous tick

        for (int i = 0; i < (int)ticks.size(); i++) {
            double price = ticks[i].price;

            // decide direction compared to the previous tick
            std::string direction;
            if      (price > prevPrice) direction = "UP  ";
            else if (price < prevPrice) direction = "DOWN";
            else                        direction = "FLAT";

            std::cout << ticks[i].date
                      << "  " << direction
                      << "  price: " << price
                      << std::endl;

            prevPrice = price;  // update for the next iteration

            // pause 50ms between ticks to simulate a live feed
            std::this_thread::sleep_for(std::chrono::milliseconds(50));
        }

        std::cout << std::endl;
        std::cout << "End of data. Restarting..." << std::endl;
        std::cout << std::endl;
        cycle++;
    }
}
```

3. Press **Ctrl+S** to save.

---

### Phase D — Update main.cpp

**Step 4 — Hand off to the engine**

1. Open `src/main.cpp`.
2. **Select all** (Ctrl+A) and delete everything.
3. Paste the following:

```cpp
#include <iostream>
#include "data_feed.h"
#include "engine.h"

int main() {
    std::cout << "Event-Driven Trading Engine" << std::endl;
    std::cout << "---------------------------" << std::endl;

    std::vector<Tick> ticks = loadCSV("data/tr_eikon_eod_data.csv");

    if (ticks.empty()) {
        std::cout << "No data loaded. Check that data/tr_eikon_eod_data.csv exists." << std::endl;
        return 1;
    }

    runEngine(ticks);
    return 0;
}
```

4. Press **Ctrl+S** to save.

---

### Phase E — Code Walkthrough

**The function signature**

```cpp
void runEngine(const std::vector<Tick>& ticks);
```

`const std::vector<Tick>&` is the standard way to pass large data in C++. Without `&`, the program would copy the entire vector of 2,158 ticks on every call. The `&` passes a reference instead — like handing someone a map rather than photocopying the whole book. `const` guarantees the function cannot accidentally modify any tick.

---

**The outer `while (true)` loop**

```cpp
while (true) {
    // process all ticks once, then cycle++ and restart
}
```

This keeps the engine running indefinitely. When it reaches the last tick it increments `cycle` and starts again from the beginning. Press **Ctrl+C** to kill the process — the OS handles this cleanly with no extra code required.

---

**Tracking state across iterations**

```cpp
double prevPrice = ticks[0].price;

for (int i = 0; i < (int)ticks.size(); i++) {
    double price = ticks[i].price;
    ...
    prevPrice = price;  // update for the next iteration
}
```

`prevPrice` is the simplest form of **state** — a value that persists from one tick to the next. This is the core idea of the event-driven pattern: after processing each event, update the system state so the next event can act on it. In Week 4 this becomes a moving average; in Week 5 it drives buy/sell decisions.

---

**Direction logic**

```cpp
if      (price > prevPrice) direction = "UP  ";
else if (price < prevPrice) direction = "DOWN";
else                        direction = "FLAT";
```

Three mutually exclusive outcomes. The extra space in `"UP  "` keeps terminal output aligned with `"DOWN"` and `"FLAT"`.

---

**The delay**

```cpp
std::this_thread::sleep_for(std::chrono::milliseconds(50));
```

Pauses 50 ms between each tick, making the output feel like a live market feed rather than a wall of text. Change `50` to any value you like — `10` for fast, `500` for slow.

---

### Phase F — Build and Run

**Step 5 — Build and run**

> **Important:** run the binary from inside the `build/` folder so the `data/` symlink resolves correctly.

1. Click **⚙ Build** in the blue status bar.
2. Open the terminal in Codespaces (**Ctrl+`**) and run:
   ```sh
   cd build && ./engine
   ```
3. Expected output:
   ```
   Event-Driven Trading Engine
   ---------------------------
   Engine started. Processing 2158 ticks.
   Press Ctrl+C to stop.

   --- Cycle 1 ---
   2010-01-04  FLAT  price: 30.5728
   2010-01-05  UP    price: 30.6257
   2010-01-06  DOWN  price: 30.1385
   ...
   End of data. Restarting...

   --- Cycle 2 ---
   2010-01-04  FLAT  price: 30.5728
   ...
   ```
4. Press **Ctrl+C** to stop.

---

### Phase G — Commit and Push

**Step 6 — Commit your work**

1. Click the **Source Control** icon in the left sidebar.
2. Hover over **"Changes"** → click **"+"** to stage all files.
3. Type the commit message:
   ```
   Week 3: engine loop, tick-by-tick simulation, price direction tracking
   ```
4. Click **"Commit"**.
5. Click **"Sync Changes"**.

---

## Milestone Checklist

- [ ] `src/engine.h` declares `runEngine`
- [ ] `src/engine.cpp` implements the tick-by-tick loop with direction tracking and 50 ms delay
- [ ] `src/main.cpp` contains only data loading and the `runEngine` call
- [ ] `CMakeLists.txt` includes `src/engine.cpp` in the `add_executable` line
- [ ] Program builds with no errors (`[100%] Built target engine`)
- [ ] Running `./engine` from inside `build/` streams ticks continuously with UP / DOWN / FLAT labels
- [ ] Ctrl+C stops the engine cleanly
- [ ] All changes are committed and visible on GitHub

---

## Reference: Files Changed This Week

| File             | Change                                                                     |
| ---------------- | -------------------------------------------------------------------------- |
| `src/engine.h`   | New — declares `runEngine`                                                 |
| `src/engine.cpp` | New — continuous tick-by-tick loop with direction tracking and 50 ms delay |
| `src/main.cpp`   | Updated — now just loads data and calls `runEngine`                        |
| `CMakeLists.txt` | Updated — adds `src/engine.cpp` to the build                               |

---

## Quick Reference: New C++ Features Used

| Feature                          | What it does                                             |
| -------------------------------- | -------------------------------------------------------- |
| `const std::vector<T>&`          | Passes a large container by reference without copying it |
| `while (true)`                   | Runs indefinitely until the process is killed (Ctrl+C)   |
| `prevPrice = price`              | Persists state from one loop iteration to the next       |
| `if / else if / else`            | Chooses one of three mutually exclusive outcomes         |
| `ticks.front()` / `ticks.back()` | Returns the first / last element of a vector             |
| `void` return type               | A function that performs an action but returns no value  |
| `std::this_thread::sleep_for`    | Pauses execution for a given duration                    |
| `std::chrono::milliseconds(50)`  | Represents a 50 ms duration for use with `sleep_for`     |

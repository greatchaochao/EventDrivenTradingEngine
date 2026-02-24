# Week 2 — Market Data Ingestion & Streaming Architecture

## Overview

**Theme:** Market Data Ingestion & Streaming Architecture

By the end of this week, your engine will load **real historical market data** and print Apple's prices to the terminal. This is the data pipeline that every future module depends on.

**Environment:** GitHub Codespaces (same environment as Week 1)

**Time estimate:** 1.5 – 2 hours

---

## What You Will Build This Week

Four things:

1. **`data/tr_eikon_eod_data.csv`** — a real dataset you download from the web
2. **`src/data_feed.h`** — declares the `Tick` struct and `loadCSV` function
3. **`src/data_feed.cpp`** — implements `loadCSV` (all the file-reading logic)
4. **Updated `src/main.cpp`** — entry point only; calls `loadCSV` and prints results

New C++ concepts introduced:
- `struct` — group related fields together
- header files (`.h`) — declare what a module offers
- `std::ifstream` — open and read a file
- `std::stringstream` — split a line by commas
- `std::vector` — a resizable list

Splitting code across files is standard practice: `main.cpp` stays clean and readable while the data logic lives in its own dedicated module.

---

## Background: What Is a Tick?

In financial systems, a **tick** is one snapshot of a market at a single point in time. The dataset you are downloading records the **daily closing price** of 12 instruments from 2010 to 2018.

Each row looks like this:

```
Date,       AAPL.O,  MSFT.O,  INTC.O, ...
2010-01-04, 30.57,   30.95,   20.88,  ...
```

Your program will read only two columns — **Date** and **AAPL.O** — and ignore the rest.

---

## Step-by-Step Plan

### Phase A — Download the Real Market Data

**Step 1 — Download the CSV file in your browser**

1. In a **new browser tab**, go to:
   ```
   http://hilpisch.com/tr_eikon_eod_data.csv
   ```
2. The browser will either display the raw file or download it automatically.
   - If it **downloads automatically**, find it in your `Downloads` folder.
   - If it **displays in the browser**, press **Ctrl+S** (Windows) or **Cmd+S** (Mac) to save it. Make sure the filename is `tr_eikon_eod_data.csv`.

**Step 2 — Upload the file to your Codespace**

1. Go back to the browser tab with your **Codespace** (the VSCode editor).
2. In the **Explorer** panel on the left, right-click the `data/` folder → **"Upload..."**.
3. Select `tr_eikon_eod_data.csv` from your Downloads folder.
4. The file now appears inside `data/` in the Explorer panel.

> **What is this file?** It contains end-of-day closing prices for Apple (`AAPL.O`), Microsoft, Amazon, and several other instruments, spanning 2010–2018. It was published by Yves Hilpisch as a sample financial dataset for educational use.

---

### Phase B — Update the CMake Build File

**Step 3 — Add the new source files and a data symlink**

CMake needs to know about `data_feed.cpp`. We also add a one-time command that creates a symlink `build/data → project/data` after every build, so the binary can find the CSV via a relative path without moving the binary out of `build/`.

1. Open `CMakeLists.txt`.
2. Replace the entire contents with:

```cmake
cmake_minimum_required(VERSION 3.10)
project(EventDrivenTradingEngine)
set(CMAKE_CXX_STANDARD 23)

add_executable(engine src/main.cpp src/data_feed.cpp)
target_include_directories(engine PRIVATE src)

# Symlink build/data -> project data/ so the binary finds the CSV via relative path
add_custom_command(TARGET engine POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E create_symlink
        ${CMAKE_SOURCE_DIR}/data
        ${CMAKE_BINARY_DIR}/data
    COMMENT "Symlinking build/data -> project data/")
```

3. Press **Ctrl+S** to save.

> **What is `POST_BUILD`?** After compiling, CMake runs the `create_symlink` command. It creates `build/data` as a shortcut pointing to the real `data/` folder. When `engine` opens `"data/tr_eikon_eod_data.csv"`, the OS follows that shortcut transparently.

> **What is `target_include_directories`?** It tells the compiler to look inside `src/` when resolving `#include` statements — this is how `main.cpp` can write `#include "data_feed.h"` without specifying a path.

---

### Phase C — Create the header file

**Step 4 — Create `src/data_feed.h`**

A header file (`.h`) is a declaration file. It tells other files what a module *offers* — the names and types — without showing the implementation. `main.cpp` will include this to know about `Tick` and `loadCSV`.

1. Right-click the `src/` folder → **"New File"** → name it `data_feed.h`.
2. Paste the following:

```cpp
#pragma once
#include <string>
#include <vector>

// One row of Apple (AAPL.O) market data
struct Tick {
    std::string date;
    double price;
};

// Load AAPL.O prices from tr_eikon_eod_data.csv.
// Rows with a missing price are silently skipped.
std::vector<Tick> loadCSV(const std::string& filename);
```

3. Press **Ctrl+S** to save.

> **What is `#pragma once`?** It tells the compiler to include this file only once, even if multiple files include it. Without it you could get duplicate-definition errors.

---

### Phase D — Create the implementation file

**Step 5 — Create `src/data_feed.cpp`**

This file contains the actual code for `loadCSV`. Separating the *what* (header) from the *how* (`.cpp`) keeps each file focused and easy to read.

1. Right-click the `src/` folder → **"New File"** → name it `data_feed.cpp`.
2. Paste the following:

```cpp
#include "data_feed.h"
#include <fstream>
#include <sstream>
#include <iostream>

std::vector<Tick> loadCSV(const std::string& filename) {
    std::vector<Tick> ticks;
    std::ifstream file(filename);

    if (!file.is_open()) {
        std::cerr << "Error: could not open " << filename << std::endl;
        return ticks;
    }

    std::string line;
    std::getline(file, line);  // skip the header row

    while (std::getline(file, line)) {
        std::stringstream ss(line);
        std::string date, priceStr;

        std::getline(ss, date,     ',');  // column 0: Date
        std::getline(ss, priceStr, ',');  // column 1: AAPL.O

        if (priceStr.empty()) continue;  // row has no Apple price

        Tick t;
        t.date  = date;
        t.price = std::stod(priceStr);
        ticks.push_back(t);
    }

    return ticks;
}
```

3. Press **Ctrl+S** to save.

---

### Phase E — Update main.cpp

**Step 6 — Slim down main.cpp to the entry point only**

1. Open `src/main.cpp`.
2. **Select all** (Ctrl+A) and delete everything.
3. Paste the following:

```cpp
#include <iostream>
#include "data_feed.h"

int main() {
    std::cout << "Event-Driven Trading Engine -- Market Data Feed" << std::endl;
    std::cout << "------------------------------------------------" << std::endl;

    std::vector<Tick> ticks = loadCSV("data/tr_eikon_eod_data.csv");

    if (ticks.empty()) {
        std::cout << "No data loaded. Check that data/tr_eikon_eod_data.csv exists." << std::endl;
        return 1;
    }

    std::cout << "Loaded " << ticks.size() << " AAPL.O ticks." << std::endl << std::endl;

    // Print the first 10 rows only -- the dataset has ~2200 rows
    int limit = 10;
    for (int i = 0; i < limit && i < (int)ticks.size(); i++) {
        std::cout << ticks[i].date << "  AAPL.O  price: " << ticks[i].price << std::endl;
    }

    std::cout << "...\n(showing first " << limit << " of " << ticks.size() << " rows)" << std::endl;
    return 0;
}
```

4. Press **Ctrl+S** to save.

> `main.cpp` no longer contains any data logic. It simply calls `loadCSV` from `data_feed.cpp` and prints the result. This separation becomes increasingly valuable as the project grows — each new module gets its own `.h`/`.cpp` pair.

---

### Phase F — Code Walkthrough

**The `Tick` struct (in `data_feed.h`)**

```cpp
struct Tick {
    std::string date;
    double price;
};
```

A `struct` groups related variables under one name. It lives in the header so both `data_feed.cpp` and `main.cpp` can use it.

---

**Header vs implementation**

| File            | Role                                            |
| --------------- | ----------------------------------------------- |
| `data_feed.h`   | Declares `Tick` and `loadCSV` — the *interface* |
| `data_feed.cpp` | Implements `loadCSV` — the *logic*              |
| `main.cpp`      | Uses them via `#include "data_feed.h"`          |

This pattern repeats for every module you add. `main.cpp` never needs to know *how* something works — only *what* it can call.

---

**Opening the file**

```cpp
std::ifstream file(filename);

if (!file.is_open()) {
    std::cerr << "Error: could not open " << filename << std::endl;
    return ticks;
}
```

`std::ifstream` opens a file for reading. The `if (!file.is_open())` check handles the case where the file is missing or the path is wrong.

---

**Skipping the header and looping line by line**

```cpp
std::getline(file, line);  // skip the header row

while (std::getline(file, line)) {
    // process one row
}
```

The first `std::getline` discards the header (`Date,AAPL.O,MSFT.O,...`). The `while` loop reads every remaining line one at a time.

---

**Splitting a CSV line**

```cpp
std::stringstream ss(line);
std::getline(ss, date,     ',');  // column 0
std::getline(ss, priceStr, ',');  // column 1
```

`std::stringstream` lets you read from a string like a file. Passing `','` splits on commas. After two calls you have the date and the Apple price string — you do not need to read the remaining 10 columns.

---

**Skipping empty rows**

```cpp
if (priceStr.empty()) continue;
```

The first row of the dataset (`2010-01-01`) has no Apple price. This skips any row where the price field is blank, so `std::stod` never receives an empty string.

---

**Converting a string to a number**

```cpp
t.price = std::stod(priceStr);
```

CSV files store everything as text. `std::stod` (string-to-double) converts the price text into a number you can do arithmetic with later.

---

**The symlink in `CMakeLists.txt`**

```cmake
add_custom_command(TARGET engine POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E create_symlink
        ${CMAKE_SOURCE_DIR}/data
        ${CMAKE_BINARY_DIR}/data
    COMMENT "Symlinking build/data -> project data/")
```

After every build, CMake creates `build/data` as a pointer to the real `data/` folder. The binary runs from `build/` and opens `"data/tr_eikon_eod_data.csv"` — the OS follows the shortcut transparently.

---

### Phase G — Build and Run

**Step 7 — Build and run**

1. Click **⚙ Build** in the blue status bar at the bottom.
2. The output panel should end with:
   ```
   [100%] Built target engine
   ```
3. Click **▷** (run) in the same status bar.
4. Expected output:
   ```
   Event-Driven Trading Engine -- Market Data Feed
   ------------------------------------------------
   Loaded 2158 AAPL.O ticks.

   2010-01-04  AAPL.O  price: 30.5728
   2010-01-05  AAPL.O  price: 30.6257
   2010-01-06  AAPL.O  price: 30.1385
   2010-01-07  AAPL.O  price: 30.0835
   2010-01-08  AAPL.O  price: 30.2885
   2010-01-11  AAPL.O  price: 30.0656
   2010-01-12  AAPL.O  price: 29.6721
   2010-01-13  AAPL.O  price: 30.0835
   2010-01-14  AAPL.O  price: 30.2172
   2010-01-15  AAPL.O  price: 29.4857
   ...
   (showing first 10 of 2158 rows)
   ```

> **If you see `Error: could not open data/tr_eikon_eod_data.csv`**, check two things:
> 1. The file is inside the `data/` folder in the Explorer panel.
> 2. The `add_custom_command` block is in `CMakeLists.txt`. Save it and click **⚙ Build** again to re-run the symlink step.

---

### Phase H — Commit and Push

**Step 8 — Commit your work**

1. Click the **Source Control** icon in the left sidebar.
2. Hover over **"Changes"** → click **"+"** to stage all files.
3. Type the commit message:
   ```
   Week 2: Tick struct, CSV reader, real AAPL.O market data
   ```
4. Click **"Commit"**.
5. Click **"Sync Changes"**.

---

## Milestone Checklist

- [ ] `data/tr_eikon_eod_data.csv` is uploaded and visible in the Explorer panel
- [ ] `src/data_feed.h` declares `Tick` and `loadCSV`
- [ ] `src/data_feed.cpp` implements `loadCSV`
- [ ] `src/main.cpp` contains only the entry point — no data logic
- [ ] `CMakeLists.txt` lists both `src/main.cpp` and `src/data_feed.cpp` and has the `POST_BUILD` symlink command
- [ ] Program builds with no errors (`[100%] Built target engine`)
- [ ] Running the program prints `Loaded 2158 AAPL.O ticks.` and shows the first 10 rows
- [ ] All changes are committed and visible on GitHub

---

## Reference: Files Changed This Week

| File                         | Change                                                                   |
| ---------------------------- | ------------------------------------------------------------------------ |
| `data/tr_eikon_eod_data.csv` | New — real EOD market data for 12 instruments, 2010–2018                 |
| `src/data_feed.h`            | New — declares `Tick` struct and `loadCSV` function                      |
| `src/data_feed.cpp`          | New — implements `loadCSV` (all file-reading and parsing logic)          |
| `src/main.cpp`               | Rewritten — entry point only; calls `loadCSV` and prints first 10 rows   |
| `CMakeLists.txt`             | Updated — adds `data_feed.cpp` to build and creates `build/data` symlink |

---

## Quick Reference: New C++ Features Used

| Feature                        | What it does                                                        |
| ------------------------------ | ------------------------------------------------------------------- |
| `struct Tick { ... };`         | Groups related fields into one named type                           |
| `#pragma once`                 | Prevents a header file from being included more than once           |
| `#include "data_feed.h"`       | Brings in declarations from another file in the same folder         |
| `std::ifstream file(name)`     | Opens a file for reading                                            |
| `std::getline(file, line)`     | Reads one line from a file                                          |
| `std::stringstream ss(line)`   | Wraps a string so you can read from it like a file                  |
| `std::getline(ss, field, ',')` | Reads one comma-separated field                                     |
| `std::stod(str)`               | Converts a string to `double`                                       |
| `POST_BUILD` symlink           | CMake hook that runs after compilation to link `build/data → data/` |
| `std::vector<Tick> ticks`      | A resizable list of Tick objects                                    |
| `ticks.push_back(t)`           | Appends one item to the end of a vector                             |
| `if (priceStr.empty())`        | Skips rows where a field has no value                               |

# Week 1 — Project Initialization & Industry-Standard Workflow

## Overview

**Theme:** Project Initialization & Industry-Standard Workflow

By the end of this week, your GitHub repository will be live, your development environment will be running in the cloud, and your C++ project will compile and run at the click of a button — with zero software installed on your personal computer.

**Environment:** GitHub Codespaces (runs entirely in the browser — no local installs required)

**Time estimate:** 1.5 – 2 hours

---

## What You Will Build This Week

```
EventDrivenTradingEngine/
├── .devcontainer/
│   └── devcontainer.json   ← auto-configures the Codespace environment
├── src/
│   └── main.cpp            ← your first C++ file
├── include/                ← header files (used from Week 3 onwards)
├── data/                   ← market data CSV files (used from Week 2 onwards)
├── docs/                   ← architecture notes and documentation
├── scripts/                ← Python visualisation scripts (used in Week 7)
├── CMakeLists.txt          ← enables one-click GUI build via CMake Tools
├── .gitignore              ← excludes build artefacts from version control
└── README.md               ← already created by GitHub
```

---

## Step-by-Step Plan

### Phase A — Create Your GitHub Account

**Step 1 — Sign up for GitHub**

1. Open a browser and go to `https://github.com`.
2. Click **"Sign up"** in the top-right corner.
3. Follow the prompts: enter your email → create a password → choose a username.
4. Check your email and click the verification link GitHub sends you.
5. When asked to choose a plan, select **Free**. No credit card is required.

---

### Phase B — Create the Project Repository

**Step 2 — Create a new repository**

1. Once logged in, click the **"+"** icon in the top-right corner → **"New repository"**.
2. Fill in the following fields:
   - **Repository name:** `EventDrivenTradingEngine`
   - **Visibility:** Public
   - **Initialise this repository with:** tick **"Add a README file"**
3. Click **"Create repository"**.

> Your repository is now live at `https://github.com/YOUR_USERNAME/EventDrivenTradingEngine`.

---

### Phase C — Launch Your Codespace

**Step 3 — Open the project in GitHub Codespaces**

1. On your repository page, click the green **"Code"** button.
2. Click the **"Codespaces"** tab.
3. Click **"Create codespace on main"**.
4. Wait approximately 30–60 seconds. A full VSCode editor opens **inside the browser**.

> **What just happened?** GitHub has provisioned a Linux computer in the cloud. It already has `g++` (C++ compiler), `cmake`, and `git` installed — you do not need to install anything.

---

### Phase D — Create the Project Skeleton

**Step 4 — Create the folder structure**

In the **Explorer** panel on the left sidebar (the file icon at the top of the left toolbar):

1. Right-click the empty file area below `README.md` → **"New Folder"**.
2. Create each of the following folders one by one: `src`, `include`, `data`, `docs`, `scripts`.

Your Explorer panel should now show all five folders alongside `README.md`.

> **Tip:** GitHub does not store empty folders. To make sure each folder appears in your repository after pushing, you can add a blank placeholder file inside each one. Right-click a folder → **"New File"** → name it `.gitkeep`. Repeat for `include`, `data`, `docs`, and `scripts`.

**Step 5 — Create the devcontainer configuration**

This file tells Codespaces which tools and extensions to install automatically every time the environment is created. It means anyone who opens your repository in a Codespace gets the exact same setup.

1. Right-click the file area → **"New Folder"** → name it `.devcontainer`.
2. Right-click `.devcontainer` → **"New File"** → name it `devcontainer.json`.
3. Paste the following content exactly:

```json
{
  "image": "mcr.microsoft.com/devcontainers/cpp:latest",
  "customizations": {
    "vscode": {
      "extensions": [
        "ms-vscode.cpptools",
        "ms-vscode.cmake-tools"
      ]
    }
  }
}
```

4. Press **Ctrl+S** to save.

**Step 6 — Create the CMake build file**

This file tells the compiler what to build and how. It is what powers the one-click build button in the toolbar.

1. Right-click the root of the file tree (not inside any folder) → **"New File"** → name it `CMakeLists.txt`.
2. Paste the following content exactly:

```cmake
cmake_minimum_required(VERSION 3.10)
project(EventDrivenTradingEngine)
set(CMAKE_CXX_STANDARD 23)

add_executable(engine src/main.cpp)
```

3. Press **Ctrl+S** to save.

> **What does this mean?**
> - `project(...)` → names the project
> - `set(CMAKE_CXX_STANDARD 23)` → uses C++23, the latest standard — fully supported by GCC 13.3.0 which comes pre-installed in Codespaces
> - `add_executable(engine src/main.cpp)` → compile `main.cpp` into a program called `engine`
>
> In future weeks, when you add new `.cpp` files, you will simply add them to this last line.

> **Popup: "Install Recommended Extensions?"**  
> After saving `CMakeLists.txt`, VSCode will show a notification in the bottom-right corner asking to install **C/C++ Extension Pack** and **CMake Tools**. Click **"Install"** — this installs both extensions immediately and is the fastest way to get the build toolbar working. You can dismiss the popup and install them manually via the Extensions sidebar if it disappears.

**Step 7 — Create the first C++ source file**

1. Right-click the `src/` folder → **"New File"** → name it `main.cpp`.
2. Paste the following starter code:

```cpp
#include <iostream>

int main() {
    std::cout << "Event-Driven Trading Engine -- Initialised." << std::endl;
    return 0;
}
```

3. Press **Ctrl+S** to save.

**Step 8 — Create the .gitignore file**

This file tells git which files to ignore — specifically the compiled output and temporary build folders that CMake generates. These should never be committed to GitHub.

1. Right-click the root of the file tree (not inside any folder) → **"New File"** → name it `.gitignore`.
2. Paste the following content exactly:

```
# CMake build output
build/
```

3. Press **Ctrl+S** to save.

> **Why?** Every time you click Build, CMake creates a `build/` folder containing hundreds of temporary files and the compiled `engine` binary. These are automatically regenerated and do not belong in version control — committing them would clutter your repository history with machine-generated files.

---

### Phase E — Build and Run (No Terminal Required)

**Step 9 — Build and run using the CMake toolbar**

CMake Tools detects `CMakeLists.txt` automatically and adds buttons to the **blue status bar** at the very bottom of the VSCode window.

1. A popup notification may appear in the bottom-right corner asking **"Select a Kit"** — click it and choose **"GCC"** from the list. If no popup appears, click the **"No Kit Selected"** text in the status bar and select **"GCC"** from the dropdown.
2. Look at the **blue status bar** at the bottom of the screen. You will see a **⚙ Build** button (it may also show the project name next to it).
3. Click **⚙ Build**. The output panel at the bottom shows compilation progress. A successful build ends with:
   ```
   [100%] Built target engine
   ```
4. Click the **▷** (play/run) button in the same status bar.
5. The output panel prints:
   ```
   Event-Driven Trading Engine -- Initialised.
   ```

> **If the status bar buttons are not visible:** Press **Ctrl+Shift+P** to open the Command Palette, type `CMake: Configure`, and press Enter. The buttons should appear afterwards.

---

### Phase F — Save Your Work to GitHub

**Step 10 — Commit and push using the Source Control panel**

All changes live only in the cloud Codespace until you push them to your repository.

1. Click the **Source Control** icon in the left sidebar (the branch/fork icon, third icon from the top).
2. All new files appear under **"Changes"**. Hover over the **"Changes"** section header and click the **"+"** icon to stage all files at once. They will move to **"Staged Changes"**.
3. Click inside the message box at the very top of the Source Control panel (it says *"Message"* as placeholder text).
4. Type the following commit message:
   ```
   Week 1: project scaffold, devcontainer, CMake build, initial main.cpp
   ```
5. Click the blue **"Commit"** button.
6. Click **"Sync Changes"** (or **"Publish Branch"** if prompted). VSCode pushes all your files to GitHub.
7. Switch to the browser tab with your GitHub repository and refresh the page. All folders and files should now be visible.

---

### Phase G — End of Session Housekeeping

**Step 11 — Stop your Codespace to save your free usage quota**

GitHub Free accounts include 120 core-hours of Codespace usage per month (~60 real hours on a standard machine). The Codespace auto-stops after 30 minutes of inactivity, but it is good practice to stop it manually.

1. Go back to your repository page on GitHub.
2. Click the green **"Code"** button → **"Codespaces"** tab.
3. Click the **"..."** menu next to your active Codespace → **"Stop codespace"**.

To resume in your next session:
- Click **"Code"** → **"Codespaces"** → **"Open in browser"** — your editor reloads exactly where you left off, all files intact.

---

## Milestone Checklist

Before finishing Week 1, verify all of the following:

- [ ] GitHub account created and email verified
- [ ] Repository `EventDrivenTradingEngine` is visible at `https://github.com/YOUR_USERNAME/EventDrivenTradingEngine`
- [ ] Codespace opens successfully in the browser
- [ ] All five folders (`src`, `include`, `data`, `docs`, `scripts`) exist in the repository
- [ ] `.devcontainer/devcontainer.json` is committed to the repository
- [ ] `CMakeLists.txt` is committed to the repository
- [ ] `.gitignore` is committed to the repository
- [ ] `src/main.cpp` compiles successfully (status bar shows `[100%] Built target engine`)
- [ ] Running the program prints `Event-Driven Trading Engine -- Initialised.`
- [ ] All files are pushed — visible on GitHub in the browser

---

## Reference: Files Created This Week

| File                              | Purpose                                                                           |
| --------------------------------- | --------------------------------------------------------------------------------- |
| `.devcontainer/devcontainer.json` | Defines the cloud development environment; auto-installs C++ and CMake extensions |
| `CMakeLists.txt`                  | Build configuration; enables the one-click Build button in the toolbar            |
| `src/main.cpp`                    | First C++ source file; the entry point of the trading engine                      |
| `.gitignore`                      | Excludes `build/` folder and compiled `engine` binary from version control        |
| `include/.gitkeep`                | Placeholder so the `include/` folder is tracked by git                            |
| `data/.gitkeep`                   | Placeholder so the `data/` folder is tracked by git                               |
| `docs/.gitkeep`                   | Placeholder so the `docs/` folder is tracked by git                               |
| `scripts/.gitkeep`                | Placeholder so the `scripts/` folder is tracked by git                            |

---

## Quick Reference: Key Actions

| Action              | How                                                     |
| ------------------- | ------------------------------------------------------- |
| Build the project   | Click **⚙ Build** in the blue status bar                |
| Run the program     | Click **▷** in the blue status bar                      |
| Save a file         | **Ctrl+S**                                              |
| Open Source Control | Click the branch icon in the left sidebar               |
| Stage all changes   | Hover over "Changes" → click **"+"**                    |
| Commit              | Type a message → click blue **"Commit"** button         |
| Push to GitHub      | Click **"Sync Changes"**                                |
| Stop Codespace      | GitHub repo page → Code → Codespaces → **"..."** → Stop |

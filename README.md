# C++ Console RPG Engine

A single-file console RPG written in C++ for Windows. It features double-buffered terminal rendering on a 100x100 grid, real-time combat timing, dynamic class evolution at Level 10, procedural enemy generation, and file-based save persistence.

---

## Current Game Architecture

### 1. Viewport & Rendering Pipeline
- **Grid Size:** 100x100 bounded by outer walls (`#`).
- **Render Distance:** 11x11 local tile viewport centered on player position (`O`).
- **Flicker Mitigation:** Uses `std::ostringstream` double-buffering combined with `SetConsoleCursorPosition` to overwrite standard output without clearing the screen. Lines are sanitized via `FormatLine()` to pad trailing whitespace and wipe ghost text.

### 2. Character System & Leveling Logic
- **Core Attributes:** Strength, Intellect, Dexterity, Agility, Essence, Vigor.
- **Auto-Scaling (Levels 1–10):** Automatically increases Vigor (HP = Vigor x 10) and Essence (Resource = Essence x 10) while granting spendable stat points.
- **Level 10 Class Evolution:**
  - Evaluates highest core attribute: **Tank** (STR), **Mage** (INT), **Archer** (DEX), **Rogue** (AGI).
  - **Jack of All Trades:** Special path unlocked automatically if all 6 core attributes are strictly equal at Level 10.
  - **Tie-Breaker System:** Opens an interactive console selection menu if two or more primary stats are tied.
- **Class Locks:** Restricts point spending post-Level 10 based on the selected class archetype.

### 3. Combat Mechanics & Cooldowns
- **Melee Range Check:** Attacks only hit enemies (`X`) situated within a 1-tile radius (dx <= 1, dy <= 1).
- **Light Attack:**
  - **Trigger:** `Q` key or Mouse Left-Click.
  - **Cooldown:** 1,000 ms.
  - **Formula:** 5 + (MaxAttr / 2) to 10 + MaxAttr damage minus enemy armor mitigation.
- **Heavy Attack:**
  - **Trigger:** `E` key or Mouse Right-Click.
  - **Cooldown:** 2,000 ms.
  - **Formula:** 10 + MaxAttr to 25 + (MaxAttr x 2) damage minus enemy armor mitigation.

### 4. Enemy Generation & World Events
- **Spawn Capacity:** Active enemy pool scales dynamically to 5 + Player Level.
- **Procedural Scaling:** Enemy stat pool generates randomly based on 50%–90% of total player attribute points.
- **Action Log:** Rolling 3-line message buffer tracking spawns, attacks, and XP gain.

### 5. Input System & Controls
- **Event Reader:** Processes Windows input events via `ReadConsoleInput` and state checks using `GetAsyncKeyState`.

| Control | Function |
| :--- | :--- |
| **W, A, S, D** | Move Character |
| **Q / Left Click** | Execute Light Attack |
| **E / Right Click** | Execute Heavy Attack |
| **ENTER** | Toggle Character Sheet / Pause Menu |
| **ESC** | Open Quit Menu & Trigger Auto-Save |

### 6. Persistence & File I/O
- **Save File:** Serializes player data (name, stats, position, XP milestones) and active world enemies (coordinates, level, HP, armor, class) directly to `savegame.txt`.

---

## Technical Environment

- **Language:** C++11 / C++17
- **Platform:** Windows (requires `<windows.h>` and `<conio.h>`)
- **Compiler Workload:** MSVC v143 (Visual Studio 2022)

# 🐍 Serpentech

**Serpentech** is a modern, feature-rich Snake Game written in **C** using the powerful **Raylib** graphics library.  
Designed with flexibility and gameplay diversity, Serpentech offers **multiple game modes**, **power boosters**, and **scoreboard persistence** — all in a lightweight and fast application. Whether you're playing solo, competing with friends, or challenging an AI-driven opponent, Serpentech delivers a polished and engaging arcade experience.

---

## 🚀 Game Modes

1. **Classic Mode**  
   Play the traditional snake game with a twist — experience dynamic boosters, persistent high scores, and enhanced gameplay responsiveness.

2. **TimeStreak Mode**  
   A high-speed challenge mode! Rack up the most points in a limited time using boosters and precision control.

3. **Multiplayer Mode** *(Same system)*  
   Two players compete on the same screen in real-time snake action.

4. **VS Computer Mode**  
   Battle against a computer-controlled snake that uses a **shortest path algorithm** to grab fruits before you do!

---

## 🧪 Power Boosters

Power-ups randomly spawn in **Classic** and **TimeStreak** modes to supercharge gameplay:

| Booster Name     | Effect Description                                                                 |
|------------------|-------------------------------------------------------------------------------------|
| ![2x_booster](https://github.com/user-attachments/assets/b4f64a86-8ac3-41a5-94d0-bb6d32606fff) Speed Booster  | **Doubles** the snake's speed temporarily.                                          |
| ![slow_downer](https://github.com/user-attachments/assets/5a0d1683-931c-4fb9-bdf8-9d8c114da234) Slow Downer    | **Halves** the snake's speed to offer better control.                              |
| ![point_doubler](https://github.com/user-attachments/assets/9515f4cd-bd94-47a0-a1ff-01c00df7e777) Double Point   | Earn **2x points** per fruit collected for a short duration.                        |
| ![length_shortener](https://github.com/user-attachments/assets/ab301176-f220-4d40-aefb-145ede6213d9) Length Reducer | Instantly **reduces snake length by 3 segments**, maintaining minimum length 4. |

> ⚠️ **Power boosters despawn if you didn't collect it in a short span of time.**

---

## 📊 Scoreboard Support

- Persistent high score files are maintained for:
  - **Classic Mode**
  - **TimeStreak Mode**
- Scores are automatically saved, loaded, and displayed in-game.

---

## 💪 Controls

### VS Computer / TimeStreak / Classic

* `W` → Move Up
* `A` → Move Left
* `S` → Move Down
* `D` → Move Right

### Multiplayer Mode

* **Player 1:**

  * `W`, `A`, `S`, `D` for movement
* **Player 2:**

  * Arrow Keys (↑ ← ↓ →) for movement

---

## 🗃️ Screenshots

| Enter Name Screen                                                            | Menu Screen                                                      | Classic Mode                                                           |
| ---------------------------------------------------------------------------- | ---------------------------------------------------------------- | ---------------------------------------------------------------------- |
| ![image](https://github.com/user-attachments/assets/7aacdf39-68f6-41eb-a477-7d1c81d3bce2)| ![image](https://github.com/user-attachments/assets/18d87ae0-4c79-40e6-b34d-d3363ef3bb31)| ![image](https://github.com/user-attachments/assets/a8506b7e-e1c4-4051-a3ce-3056599d0f5f)
 |

| TimeStreak Mode                                                              | VS Computer Mode                                                              | Multiplayer Mode                                                               |
| ---------------------------------------------------------------------------- | ----------------------------------------------------------------------------- | ------------------------------------------------------------------------------ |
| ![image](https://github.com/user-attachments/assets/cc20db30-24df-43b2-b2d3-bd3024e54c40)| ![image](https://github.com/user-attachments/assets/499f12bf-4189-4304-9ee4-51e502912591)| ![image](https://github.com/user-attachments/assets/86e0d1cd-97ec-4ff4-8b86-b549b135117c)
 |

---

## 🧩 Special Game Rules

### 🎮 Multiplayer Mode Rules

* **Dual Snake Control**: Two snakes are controlled by separate sets of keys:

  * Player 1: `W`, `A`, `S`, `D`
  * Player 2: `↑`, `↓`, `←`, `→`
* **Collision Detection**:

  * If a snake hits its own tail or the game border, the opponent wins.
  * If both snakes collide head-on, the score is compared to decide the winner.
* **Fruits**: Multiple fruits are placed on the board simultaneously, and each snake can consume them independently.
* **Food Interaction**:

  * Eating food adds score and increases length.
  * Each fruit is initialized with awareness of other fruits to avoid overlap.
* **Winning Condition**:

  * First snake to cause the other to crash wins.
  * If time ends or a collision occurs simultaneously, the highest score wins.

### 🤖 VS Computer Mode Rules

* **Single Player + AI**: Player uses `W`, `A`, `S`, `D` to move. The AI snake uses a simple strategy to approach food.
* **Collision Logic**:

  * If either the player or the AI hits their own tail, the other wins.
  * If both collide, the scores are compared to determine the winner.
* **Food Competition**:

  * Both player and AI compete for a shared fruit.
  * The one who reaches the fruit first gets the point.
* **AI Behavior**:

  * AI determines direction each frame using the `AIThink` function.
  * AI avoids collision with the player while approaching food.
* **Game Over Display**:

  * Message displays the winner and prompts return to main menu with `ENTER`.

---

## 🛠️ Setup & Compilation

### 📅 Prerequisites (Only if building from source)

* C compiler (`gcc`, `clang`, etc.)
* [raylib](https://www.raylib.com/) library
* Git
* (Optional) CMake

> ⚠️ **If you're using the prebuilt `.exe` on Windows, you can skip installing these.**

---

### Windows Users

#### 🛠️ Option 1: Using MSYS2 (Recommended)

1. Install [MSYS2](https://www.msys2.org/)
2. Open the MSYS2 terminal and run:

   ```bash
   pacman -Syu
   pacman -S mingw-w64-x86_64-gcc mingw-w64-x86_64-raylib
   ```
3. Clone and compile:

   ```bash
   git clone https://github.com/Thiyaga1586/Serpentech.git
   cd Serpentech
   gcc -o Serpentech.exe main.c -lraylib -lopengl32 -lgdi32 -lwinmm
   ./Serpentech.exe
   ```

#### 🛠️ Option 2: Manual Setup

* Download the [Raylib Windows Installer](https://github.com/raysan5/raylib/releases)
* Use IDEs like Code::Blocks or Visual Studio with Raylib linked properly
* Follow [Raylib Windows Setup Guide](https://github.com/raysan5/raylib/wiki/Working-on-WINDOWS)

### 🛠️ Option 3: Ready to Play (Pre-compiled EXE)

If you are on **Windows** and prefer not to compile the source code yourself:

1. Visit the [Releases](https://github.com/Thiyaga1586/Serpentech/releases) section of this repository.
2. Download the latest [Serpentech.exe](https://github.com/Thiyaga1586/Serpentech/Sepentech.exe) build.
3. Double-click to run — **no installation required**.
4. All game data, assets, and embedded highscore files are included and ready to go.

> ✅ No compiler setup needed
> ✅ No additional dependencies
> ✅ All four modes and power boosters fully functional
> ✅ Highscore persistence works out of the box

Enjoy the game instantly with a plug-and-play experience!

---

### Linux

```bash
# Install raylib
sudo apt install libraylib-dev       # Debian/Ubuntu

# Compile and run
git clone https://github.com/Thiyaga1586/Serpentech.git
cd Serpentech
gcc -o Serpentech main.c -lraylib -lm -ldl -lpthread -lGL
./Serpentech
```

---

### MacOS

1. Install [Homebrew](https://brew.sh/)
2. Install raylib:

```bash
brew install raylib
```

3. Clone and compile:

```bash
git clone https://github.com/Thiyaga1586/Serpentech.git
cd Serpentech
gcc -o Serpentech main.c -lraylib -lm -framework OpenGL -framework Cocoa -framework IOKit
./Serpentech
```

> ⚠️ **MacOS/Linux users must build from source for now. Prebuilt binaries are not cross-platform.**

---

## 📖 License

This project is licensed under the **MIT License**. See the [LICENSE](./LICENSE) file for more info.

## 💡 Credits

* Developed by [Thiyaga1586](https://github.com/Thiyaga1586)
* Built using [raylib](https://www.raylib.com/)

---

### 🔗 Links

* [🔗 GitHub Repository](https://github.com/Thiyaga1586/Serpentech)
* [🌟 Raylib Documentation](https://www.raylib.com/cheatsheet/cheatsheet.html)

---

Enjoy the game and feel free to contribute. 🚀


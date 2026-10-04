# AGENTS.md — lab-4 (image-graphics-lab)

Context for AI agents (and future-you) working in this folder. This is a university graphics-lab workspace, not a shipped product.

**The C++/OpenGL rewrite in `Project1/` is now the primary deliverable** (the teacher's assignment requires C++; the original Python version is kept only as reference). See [docs/phase-1-plan.md](docs/phase-1-plan.md) and [docs/phase-2-plan.md](docs/phase-2-plan.md) for the build plan, and [docs/code-walkthrough.md](docs/code-walkthrough.md) once it exists for a line-by-line explanation of `Project1`'s C++ classes.

## Layout

| Path | What it is |
|---|---|
| [Project1/](Project1/) | **The real lab assignment.** C++ / OpenGL 3.3 core, originally a Visual Studio project. Built with MinGW-w64 via a hand-written `Makefile` (no Visual Studio involved — VS Code + `mingw32-make` only, by explicit choice). |
| [opengl-cpp/](opengl-cpp/) | Scratch/reference project created while setting up the C++ toolchain. Not the lab deliverable — a CMake-based sandbox with its own copies of GLFW/GLAD/GLM under `deps/`. Kept mainly because `Project1`'s Makefile borrows its GLFW binaries (see below). |
| [legacy/](legacy/) | The original Python/PyOpenGL implementation of this same cannon project (`main.py`, `cannon.py`, `lighting.py`, `projectile.py`, `utils.py`, `verify.py`, `test.py`, its `venv/`) — kept for reference only, not being extended further. Run it with `legacy\venv\Scripts\Activate.ps1` then `python legacy\main.py` (or `python legacy\test.py` for the simpler rectangle-rotation sanity check). |
| [docs/](docs/) | Planning docs (`project-context.md` spec, `phase-1-plan.md`, `phase-2-plan.md`) plus the old Python project's report/proposal (`REPORT.md`, `report.pdf`, `verification.txt`, `images/`). |

There is no git repo at this root — no `git status`/history to check.

## Toolchain (already installed on this machine)

- **WinLibs MinGW-w64** (GCC 16, GNU Make, CMake, Ninja, GDB — all in one package), installed via `winget install --id BrechtSanders.WinLibs.POSIX.UCRT`.
- Its `bin` dir is on the **User** PATH:
  `C:\Users\LENOVO\AppData\Local\Microsoft\WinGet\Packages\BrechtSanders.WinLibs.POSIX.UCRT_Microsoft.Winget.Source_8wekyb3d8bbwe\mingw64\bin`
- No Visual Studio is installed or required. `mingw32-make`, `g++`, `cmake` work in any **freshly opened** shell (a shell/VSCode window opened before the PATH change won't see it — fully restart it, don't just open a new tab).
- Sanity check: `g++ --version` / `mingw32-make --version`.

## Project1 — build & run

```powershell
cd Project1
mingw32-make            # compiles to build_mingw\app.exe
mingw32-make run        # build (if needed) + run in one step
mingw32-make clean       # deletes build_mingw\app.exe
```
Or run the exe directly after building — **must run from inside `Project1/`**, not from `build_mingw/`, because shaders (`default.vert`, `default.frag`) are loaded by relative path:
```powershell
.\build_mingw\app.exe
```

### Why the Makefile doesn't use the project's own `.vcxproj`/`Libraries\lib\glfw3.lib`

- `Project1.slnx` / `Project1.vcxproj` are MSBuild files — only Visual Studio / VS Build Tools can process them, and we deliberately avoided installing those (user wanted a lightweight setup).
- `Project1\Libraries\lib\glfw3.lib` is built for the **MSVC ABI** and is not link-compatible with MinGW's `ld`.
- The source itself (`Main.cpp`, `VAO/VBO/EBO`, `shaderClass`, `glad.c`) is portable standard C++ with no MSVC-specific code, so it compiles fine under g++ — it just needs a MinGW-compatible GLFW static lib. The [Makefile](Project1/Makefile) points at `../opengl-cpp/deps/glfw/lib-mingw-w64/libglfw3.a` for that (GLFW headers/libs downloaded from the official 3.5.1 Windows binaries release). **This is why `opengl-cpp/deps/glfw` must not be deleted** — `Project1`'s build depends on it.
- `Project1\Libraries\include` (glad, GLFW headers, glm) is still used directly — only the `.lib` was swapped out.
- Everything links `-static-libgcc -static-libstdc++ -static`, so `app.exe` is self-contained (no `libstdc++-6.dll` etc. needed at runtime).

### Known dead code
`Project1/ChatGPT.cpp` defines its own `int main()` and is **not** referenced anywhere in `Project1.vcxproj` or the `Makefile` — it's scratch/exploration code. Never add it to the build's source list (`Main.cpp` already has `int main()`; compiling both is a duplicate-symbol link error).

### Current state of `Main.cpp`
`Main.cpp` no longer draws the single test square described in earlier revisions of this file — it builds the full Phase 2 scene: a ground plane, the cannon scene graph (`Carriage` → `Wheel`s → `Shaft`), a breakable brick wall in front of the gun, and a list of in-flight `Projectile`s. Shading is the `lit.vert`/`lit.frag` directional-light shader.

Classes under `Project1/`, roughly bottom-up:

| File | Role |
|---|---|
| `Dimensions.h` | Every measurement of the cannon (metres), carried over from `legacy/cannon.py` so the proportions match `docs/images/01_overview.png`. Header-only constants — **change sizes here, not in the class files**. |
| `Palette.h` | Every color in the scene, header-only. |
| `Mesh` | Owns one shape's VAO/VBO/EBO and draws it. |
| `Primitives` | Geometry generators: `CreateCone` (the workhorse — a cylinder is the equal-radii case), `CreateCylinder`, `CreateTube`, `CreateBox`, `CreateSphere`, `CreatePlane`. **Everything it makes is built standing up, length along +Y**; orientation is not a parameter. |
| `Part` / `Local::` | A `Part` is `{Mesh, glm::mat4 local}` — one rigid piece plus where it sits inside its owner. `Local::Move/MoveTurn/TurnMove/AlongX/AlongZ/AlongNegZ` build those matrices readably; `AlongX`/`AlongZ` are what turn the Y-up primitives sideways. `DrawParts()` uploads `objectMatrix * part.local` per part. |
| `Transform` | position + one rotation axis/angle + scale → a model matrix. |
| `Carriage` | The wooden body: trail beams, transoms, trail spade, cheeks, quoin block, axle, bolster. **Root of the cannon's scene graph** — `GetMatrix()` is the parent of both wheels and the barrel. `MoveForward()` drives it. |
| `Wheel` | Spoked cartwheel (tire tube + felloe tube + N spoke boxes + hub + brass caps). Rolls about Z via `Roll(distance)`. |
| `Shaft` | The barrel: a stack of cones (breech → reinforce → chase → muzzle swell) with brass rings, a dark bore, and trunnions. Local origin **is** the trunnion pivot, so `Elevate()` is a plain rotation with no correcting translation. Trunnions are drawn without the elevation rotation since they're the hinge, not the thing swinging on it. New in Phase 2: `GetMuzzleWorldPosition(parentMatrix)` returns the world-space tip of the barrel and `GetForwardWorldDirection(parentMatrix)` returns the world-space firing axis at the current elevation — both are what `Projectile` uses to spawn each shot. |
| `Projectile` | A single cannon ball: a sphere mesh plus world-space position and velocity. `Update(deltaTime, gravity)` is semi-implicit Euler (per spec, see `phase-2-plan.md` and `legacy/verification.txt §2`). `IsDead()` is true once the ball hits the ground or ages out, so `Main.cpp` can drop it from its vector. |
| `Wall` | A grid of axis-aligned bricks (one `Mesh` each, plus a `local` matrix and an `alive` flag). `CheckHit(sphereCentre, sphereRadius)` is the standard sphere-vs-AABB test — any brick touched by the ball's bounding sphere flips its flag, and `Draw()` simply skips dead bricks. |

Controls: ←/→ drive (wheels roll without slipping), ↑/↓ elevate the barrel (clamped 0–45°), **Space fires a cannon ball**, Esc quits.

### `Project1/tools/` — documentation image renderer

[docs/code-walkthrough.md](docs/code-walkthrough.md) is a full Bangla-language teaching walkthrough (concept → math → picture → code) and every picture in it is **rendered from the real classes**, not drawn by hand:

- `tools/RenderDocShots.cpp` — its own `main()`; opens a hidden GLFW window, draws each primitive / each build-up stage / the assembly, reads pixels back with `glReadPixels` and writes `.bmp`. **Never add it to `SRCS`** (duplicate `main()` — same rule as `ChatGPT.cpp`).
- `tools/flat.frag` — a deliberately unlit fragment shader, used only for the "before lighting" comparison image.
- `tools/bmp2png.ps1` — converts the `.bmp`s to `.png` and deletes them.
- Output lands in `docs/images/walkthrough/` (38 files).

Regenerate with `mingw32-make shots` (from inside `Project1/`). The Makefile splits sources into `COMMON` (everything but `main()`), `SRCS` (= `Main.cpp` + COMMON) and `SHOT_SRCS` (= the tool + COMMON), so the tool always renders the *current* geometry.

Staged pictures ("tire only", "tire + felloe", …) work via `DrawPartRange()` in `Part.h` plus the `PartsForDocs()` / `BarrelPartsForDocs()` / `TrunnionPartsForDocs()` accessors on `Wheel`/`Carriage`/`Shaft`. Those accessors exist **only** for the doc tool — the game never calls them. Part ordering is documented next to each accessor; if you reorder the `push_back`s in a constructor, fix those comments and re-run `mingw32-make shots`.

## opengl-cpp — scratch/reference only

CMake + MinGW setup used to validate the toolchain before touching `Project1`. Build with:
```powershell
cd opengl-cpp
cmake -B build -S . -G "MinGW Makefiles"
cmake --build build
.\build\app.exe
```
Not the lab deliverable — don't confuse this with `Project1`. Its `deps/glfw` is a live dependency of `Project1`'s Makefile though (see above), so leave `opengl-cpp/deps/` in place even if the rest of this folder is otherwise ignored.

## legacy/ — Python version (reference only)

`legacy/test.py` is a minimal legacy-GL (`glBegin`/`glEnd`) rotating rectangle, rotating around its own center (not a corner — different convention from `Project1`). `legacy/main.py` + `cannon.py` + `lighting.py` + `projectile.py` + `utils.py` are the full Python/PyOpenGL cannon scene that `Project1` is now being reimplemented from, in C++. Uses the `venv/` inside `legacy/`.
```powershell
.\legacy\venv\Scripts\Activate.ps1
python legacy\test.py     # sanity check
python legacy\main.py     # the full Python scene
```
(Requires `glfw` and `PyOpenGL` installed in that venv — check with `pip list` if it fails to import.)

## Housekeeping notes
- `Project1/run_out*.log`, `run_err*.log` are leftover stdout/stderr captures from earlier manual test runs (via `Start-Process -RedirectStandardOutput/-RedirectStandardError`) — safe to delete, not build artifacts.
- `Project1/build_mingw/`, `Project1/x64/`, `Project1/.vs/`, `opengl-cpp/build/` are all build output — safe to delete/regenerate, never hand-edit.

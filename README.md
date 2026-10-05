# Medieval Cannon & Compound Castle

An interactive OpenGL 3.3 (core profile) medieval siege scene in C++. Three cannons face a
defended compound castle with double-walled fortifications, a working drawbridge over a moat,
breakable doors, brick-by-brick wall damage, archers on the towers, and a 15-soldier army
behind the cannons. You rotate each cannon, aim, and fire — and watch the castle react.

Built with **MinGW-w64 + GLFW 3.5.1 + GLAD + GLM**, with a hand-written Makefile
(no Visual Studio required). See [`AGENTS.md`](AGENTS.md) for the full project layout.

---

## Controls

| Key                    | Action                                              |
|------------------------|-----------------------------------------------------|
| **Mouse drag (left)**  | Orbit the camera around the castle                  |
| **Mouse wheel**        | Zoom in / out                                       |
| `1` / `2` / `3`        | Toggle selection of the left / centre / right cannon|
| `A`                    | Select **all** three cannons at once                |
| **← / →**              | Yaw the most-recently-selected cannon left / right  |
| **↑ / ↓**              | Drive the active cannon forward / backward along its barrel |
| **W / S**            | Elevate the active cannon's barrel up / down        |
| `Space`                | Fire every selected cannon that's currently idle       |
| `R` (hold)             | Raise / lower the drawbridge                        |
| `F11`                  | Toggle fullscreen ↔ windowed                        |
| `Esc`                  | Quit                                                |

Arrow keys, `W`, and `S` always act on the **most-recently-toggled cannon** (the one you
last pressed `1`, `2`, or `3` on, or the centre cannon if you used `A`).

## Gameplay loop

1. Pick a cannon with `1` / `2` / `3` (or all with `A`).
2. Use the arrows / `W` / `S` to rotate it and elevate the barrel.
3. A **yellow aim line** on the ground shows where the selected cannon is pointing.
4. Press `Space` — the assigned crew soldier walks over, lights the fuse, and after a
   ~1.5-second delay the cannon fires. The selected cannon shows a **muzzle flash**.
5. Each shot strikes the door (25 % damage per shot, with visible tilt) or a brick
   (10 % damage per shot, with the brick darkening as it accumulates damage).
6. After 4 shots the door panels detach and tumble out of the doorway with break-physics.
   After 10 shots a brick is destroyed.
7. Hold `R` to raise the drawbridge — the chains shorten visibly while the deck swings up.

## Castle

* **4 corner towers** at the four corners (8 m body + 0.7 m parapet + merlons)
* **2 gatehouse flanking towers** (5 m body + parapet)
* **Double curtain walls** with a 1 m walkable corridor between outer and inner walls,
  topped by merlons (Great-Wall style)
* **FortGate**: two stacks of breakable bricks flanking a doorway, with non-breakable
  stone pillars and a wooden lintel
* **Wooden door**: two hinged panels that tilt visibly as damage accumulates and then
  detach with hinge-rotation physics
* **Moat** (6 m wide) crossed by a **retractable drawbridge** with chains
* **6 archers** on top of the towers with longbows and quivers
* **Z-running river** continuing the moat outward in both directions

## Attackers

* **3 cannons** (left, centre, right) with proper carriages, spoked wheels, barrels
  with brass rings and a dark bore
* **3 cannon crew** (warm copper jerkins) — one per cannon
* **15 army soldiers** in a 5×3 grid behind the cannons (grey iron jerkins, swords)

## Building

You need MinGW-w64 (`g++`, `mingw32-make`) and a MinGW-compatible GLFW 3.5.1 static library
under `opengl-cpp/deps/glfw/lib-mingw-w64/`. See [`AGENTS.md`](AGENTS.md) for toolchain
details.

```powershell
cd Project1
mingw32-make            # compile to build_mingw\app.exe
mingw32-make run        # build + run
mingw32-make clean      # delete build_mingw\app.exe
mingw32-make shots      # regenerate the docs/code-walkthrough.md images
```

Run the binary from inside `Project1/` (so the shaders load via relative path):

```powershell
.\build_mingw\app.exe
```

## Project layout

| Path                        | What it is                                       |
|-----------------------------|--------------------------------------------------|
| `Project1/`                 | The C++ / OpenGL deliverable                     |
| `Project1/Main.cpp`         | Scene entry point — orchestrates the whole world |
| `Project1/<Class>.h/.cpp`    | One file per scene-graph class (Cannon, Door, …) |
| `Project1/tools/`           | Doc-image renderer (`RenderDocShots.cpp`)        |
| `docs/`                     | Phase plans + the long Bangla code-walkthrough   |
| `opengl-cpp/`               | Scratch/reference project (provides GLFW libs)  |
| `legacy/`                   | The Python / PyOpenGL original (kept for reference)|
| `AGENTS.md`                 | Detailed project context for AI / future-you    |

## Phase history

* **Phase 1** — Single cannon (carriage + 2 wheels + barrel), lit by a directional light.
* **Phase 2** — Projectile physics (semi-implicit Euler), muzzle spawn, gravity.
* **Phase 3** — Three cannons, a `FortGate` (breakable brick front wall with non-breakable
  pillars and lintel), trees, a stationary wooden dummy target.
* **Phase 4** — A `Castle` composition with the FortGate + flanking towers + main
  `Door` (breakable), shield + towers.
* **Phase 5** — Cannon recoil, sphere-vs-AABB damage, door break-physics with hinge
  rotation. Camera orbit + 360° view.
* **Phase 6** — Many more trees, a wide river, **retractable drawbridge** with chains,
  archers on the towers, cannon crew + army, selectable cannons (1/2/3/A), delayed
  firing animation.
* **Phase 7** — Cannon yaw with muzzle flash, gradual door damage (25 % per shot),
  gradual brick damage (10 % per shot with visible darkening), double curtain walls with
  walkable corridor, river rotated to run alongside the castle, soldiers / archers
  facing the right directions, drawbridge goes up correctly, on-screen aim indicator,
  README.
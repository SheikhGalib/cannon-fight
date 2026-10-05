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
| **W / S**              | Elevate the active cannon's barrel up / down        |
| `Space`                | Fire every selected cannon that's currently idle       |
| `R` (hold)             | Raise / lower the drawbridge                        |
| `N`                    | Toggle **day / night**                              |
| `B`                    | Start the **battle simulation**                     |
| `P`                    | Pause / resume the battle simulation               |
| `T`                    | Reset the battle simulation                       |
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
* **6 archers** on top of the towers with longbows and quivers (dark blue uniforms)
* **4 castle defenders** inside the compound, near the gold crest (auto-replace dead archers)
* **Gold crest** (objective) — chest, gold pile, flag. Pulses gold when the attackers win.
* **Z-running river** continuing the moat outward in both directions

## Attackers

* **3 cannons** (left, centre, right) with proper carriages, spoked wheels, barrels
  with brass rings and a dark bore
* **3 cannon crew** (dark red jerkins) — one per cannon
* **15 army soldiers** in a 5×3 grid behind the cannons (dark red uniforms, swords)
* **6 camp tents** in two rows behind the army formation

## Phase 8 features

* **Gouraud (per-vertex) shading** — `gouraud.vert`/`gouraud.frag` replace the per-fragment `lit` shader.
* **Aim trajectory** — for each selected cannon, a yellow parabolic preview arc + a bright marker at the
  predicted hit point on the wall or ground (simulated by stepping the same ballistic equation the real
  projectile uses).
* **Wall-collision** — cannonballs stop on contact with the stone (curtain walls, corner towers, gatehouse
  towers), instead of passing through.
* **Battle simulation** — `B` starts an auto-siege: the bridge raises, archers shoot arrows at the army,
  arrows kill soldiers, the bridge drops, attackers advance. `P` pauses, `T` restarts. The gold crest
  pulses when the attackers take the gate.
* **Day/night** — `N` toggles. Night = darker sky + low moon light.
* **XYZ coordinate map** — a 3-axis overlay (red `+X`, green `+Y`, blue `+Z`) drawn in the bottom-left
  corner in screen space.
* **Distant scenery** — a ring of low-poly mountain cones (with white snow caps) + hill cones around the
  scene, so the canvas reads as infinite rather than truncated.
* **Distinct uniforms** — attackers dark red, defenders dark blue (per `images/all-type-solders-example.jpg`).

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
mingw32-make capture    # build capture.exe (Phase 9 video tool)
mingw32-make video      # run capture.exe then ffmpeg -> presentation.mp4
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
* **Phase 6** — A wide river, **retractable drawbridge** with chains,
  archers on the towers, cannon crew + army, selectable cannons (1/2/3/A), delayed
  firing animation.
* **Phase 7** — Cannon yaw with muzzle flash, gradual door damage (25 % per shot),
  gradual brick damage (10 % per shot with visible darkening), double curtain walls with
  walkable corridor, river rotated to run alongside the castle, soldiers / archers
  facing the right directions, drawbridge goes up correctly, on-screen aim indicator.
* **Phase 8** — Trees removed. Soldiers stand on the flat tower top (not floating on the
  parapets). Aim indicator shows the actual projectile hit point via simulated trajectory.
  Cannonballs collide with stone walls. Gouraud shading. Day/night toggle (`N`).
  Battle simulation (`B` / `P` / `T`) with arrow combat, soldier death, defender
  auto-replacement. Gold crest inside the castle with defenders. Camp tents behind the
  army. Distant mountain + valley scenery. XYZ coordinate map. Distinct attacker
  (dark red) / defender (dark blue) uniforms.
* **Phase 9** — Wall breakability: every curtain-wall stone segment now has its own
  health pool; cannonballs strip 10 % per hit and darken the segment; at 0 it disappears
  and cannonballs fly through the gap.  Mountains are bigger (20..35 m) and clustered
  on the +Z / -Z sides of the scene (not a uniform ring).  Trees are back, but only in
  the safe zones (sides + back of the scene); the fight zone rectangle between the
  cannons and the castle stays clear.  XYZ coordinate map moved from the bottom-left
  corner to a side panel in the bottom-right corner.  Battle simulation tightened:
  cannons auto-fire during the Advance phase, the door-broken event ends the fight
  early, the Defending phase ends early if either side is wiped out.  A
  `tools/CaptureSim.cpp` capture tool runs the full scene + battle sim and saves
  per-frame BMPs; `tools/encode_video.ps1` (ffmpeg) combines them into
  `presentation.mp4` for the project presentation.

## Phase 9 video

The `make capture` + `make video` pipeline produces a short
presentation video from the running battle simulation:

```powershell
cd Project1
mingw32-make video    # builds capture.exe, runs it, then ffmpeg -> presentation.mp4
```

The output is also copied to `images/phase-9-presentation.mp4` for
the project layout.
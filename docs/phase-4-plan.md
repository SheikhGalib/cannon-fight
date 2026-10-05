# Phase 4 — Disney-style castle with breakable wooden doors

**Goal.** Replace the Phase 3 flat front-of-fort (two brick stacks + lintel + pillars, with a dummy robot behind them) with a small Disney-fantasy-style castle: two flanking square towers, a crenellated curtain wall extending left/right, and **breakable wooden double doors** in the gate opening that the centre cannon aims at. The cannon balls now hit the doors, not the bricks.

**Reference images** (`images/fort-example-1.jpg`, `-2.jpg`, `-3.jpg`): a stone gatehouse with a wooden double door, two flanking towers, crenellations on the wall and tower tops, arrow-slit windows, and flags on the towers.

---

## Composition

```
                ┌─────────┐                                       ┌─────────┐
                │░merlons │                                       │░merlons │
            ┌───┤  ◢ ▣ ▢  ├───┐ flag                       flag ┌──┤  ◢ ▣ ▢  ├───┐
            │ ░ │  Tower  ░ │   │  /│\                       /│\ │   │  Tower  ░ │ ░ │
            │ ░ │ (stone) ░ │   │ / │ \                     / │ \│   │ (stone) ░ │ ░ │
            │ ░ │ [≡][≡]  ░ │   │/  │  \                   /  │  \   │ [≡][≡]  ░ │ ░ │
            │ ░ │         ░ │   /   │   \                 /   │   \  │         ░ │ ░ │
        ┌───┘ ░ └─────────┘ └──┐───────────────────────────┐ └──┘  ░ └─────────┘ ░ └───┐
        │ ░ ░ ░ ░ ░ ░ ░ ░ ░ ░ ░ │ ░ ░ ░ ░ ░ ░ ░ ░ ░ ░ ░ ░ ░ ░ ░ │ ░ ░ ░ ░ ░ ░ ░ ░ ░ ░ ░ ░ ░ ░ │
        │ ░ curtain wall (stone)░ │       door opening         │░ curtain wall (stone) ░ │
        │ ░ ░ ░ ░ ░ ░ ░ ░ ░ ░ ░ ░ │ ┃door┃        ┃door┃      │ ░ ░ ░ ░ ░ ░ ░ ░ ░ ░ ░ ░ ░ ░ │
        │ ░ ░ ░ ░ ░ ░ ░ ░ ░ ░ ░ ░ │ ┃left┃   ⌬    ┃right┃     │ ░ ░ ░ ░ ░ ░ ░ ░ ░ ░ ░ ░ ░ ░ │
        │ ░ ░ ░ ░ ░ ░ ░ ░ ░ ░ ░ ░ │ ┃   ┃ dummy  ┃   ┃      │ ░ ░ ░ ░ ░ ░ ░ ░ ░ ░ ░ ░ ░ ░ │
        └─────────┬──────────────┘─────────────────────────────┘──────────────┬─────────┘
                  │  breakable brick stack (Phase 3, still here)              │
                  │  (under the lintel, flanking the door)                    │
                  └────────────  ground  ──────────────────────────────────────┘
```

The dummy robot stands **inside** the castle, past the door. The cannon ball, fired through the door opening, can either smash a door panel (which kills that panel) or, if the door is gone, hit the brick stacks behind (Phase 3 still works) or the dummy robot itself (the robot is not breakable but the ball will stop on contact — out of scope for Phase 4).

---

## New classes

| Class | File(s) | What it does |
|---|---|---|
| `Crenellation` | `Crenellation.h/.cpp` | Small helper that takes (length, merlon size, gap size, depth, color) and returns a `std::vector<Part>` of upright merlons (the "teeth" of a crenellated parapet). No collision, no breakability — pure visual. |
| `Door` | `Door.h/.cpp` | Two wooden door panels in a doorway. Each panel is its own `Mesh` + `alive` flag, with a sphere-vs-AABB hit test (the panels are flat vertical AABBs). The panels are pre-decorated with horizontal "plank" boxes across their face so they read as real wooden doors. **Breakable**. |
| `Tower` | `Tower.h/.cpp` | One square stone tower: a tall box for the body, 4 corner quoins (slightly darker), 2 arrow-slit windows per visible side (dark `Palette::Bore`-coloured boxes), a `Crenellation` parapet on top, a thin `Palette::Iron` flagpole and a `Palette::Wood`-red flag. **Static, never breaks**. |
| `Drawbridge` | (inside `Castle.cpp`) | A short wooden bridge in front of the door — just a flat box with plank striping. Decorative. |
| `Castle` | `Castle.h/.cpp` | The top-level composition. Owns: two `Tower`s (one each side of the gate), the existing `FortGate` (now with `Door`s inside it instead of just a doorway), two short crenellated curtain walls extending left/right, and a `Drawbridge`. `CheckHit(centre, radius)` dispatches to the doors, then to the gate's brick stacks. |

## What stays from Phase 3

- `FortGate` class — kept as-is. `Castle` reuses it for the breakable brick stacks + lintel + pillars (it now also gets the wooden doors inside it, see below).
- `Tree`, `Robot`, `Cannon`, `Projectile` — unchanged.
- All existing `Palette` colors — no new colors needed; `Wood` for doors and flags, `Stone` for towers, `Bore` (almost-black) for arrow slits, `Iron` for the flagpole.

## What changes in FortGate

To make the doors part of the same `FortGate` they currently replace the "doorway" with two `Door` panels. The construction signature gains a `doorHeight` and the inner doorway zone gets the door panels. The brick stacks (left of doorway, right of doorway) are unchanged.

Actually, on reflection, the cleaner approach is to keep `FortGate` exactly as it is (it's already perfect for the brick + lintel + pillars assembly) and put the doors in the **new `Castle` class** as a sibling — `Castle` owns `FortGate` (for the brick stacks/lintel/pillars) and two `Door` objects (for the door panels). The collision dispatch in `Castle::CheckHit` calls both. That keeps `FortGate` untouched and makes the `Castle` an additive wrapper.

## Trajectory tuning

Phase 3's default `Projectile::DefaultSpeed = 14 m/s` and default elevation `12°` (set in `Cannon`'s constructor) give a range of about 8.1 m — the ball falls just short of the wall at `x = 12`. For Phase 4 the cannon must reach the **door** at `x = 12` cleanly, so the centre cannon's elevation is raised to `18°` at construction (range ≈ 12.0 m at 14 m/s). The two flankers are left at the same elevation for visual consistency. `Projectile::DefaultSpeed` stays at 14 — the user can always nudge it via `Projectile.h`.

The door panels are tall (full height of the doorway) and 0.10 m thick, so a ball aimed at door centre is essentially guaranteed to intersect a door panel before reaching anything behind.

## Collision

- **Door panels** (new): the existing sphere-vs-AABB test in `FortGate::CheckHit` style. Each door panel is a vertical AABB `(gateWidth/2, doorHeight, 0.10 m)` centred on the gate's centre. Implemented in `Door::CheckHit(centre, radius)`.
- **Brick stacks** (Phase 3, unchanged): `FortGate::CheckHit`.
- **`Castle::CheckHit`** runs `Door::CheckHit` first (in case both brick and door happen to be in the same hit, the door gets priority — that's where the player is aiming), then `FortGate::CheckHit`.

## Visual additions

- Two towers (square cross-section, 2.5 m wide × 2.5 m deep × 5.5 m tall, including crenellations and a 1 m flagpole on top)
- Crenellated parapet on each tower top (8 merlons around 4 sides)
- Crenellated curtain walls on each side of the gate (6 m long, 1 merlon every 0.4 m)
- Arrow-slit windows (4 per tower, one per face)
- Two wooden doors (2.0 m wide × 3.0 m tall total opening, split into two 1.0 m × 3.0 m panels, 0.10 m thick)
- Decorative plank strips across each door
- A drawbridge (3 m wide × 1.5 m deep, 0.15 m thick, 4 wooden plank strips on top)
- A small banner/flag on each tower

## File additions

- `Project1/Crenellation.h`, `Crenellation.cpp`
- `Project1/Door.h`, `Door.cpp`
- `Project1/Tower.h`, `Tower.cpp`
- `Project1/Castle.h`, `Castle.cpp`
- `Project1/tools/CapturePhase4.cpp` (Phase 4 verification screenshot)
- `Project1/tools/CapturePhase4Overview.cpp` (clean cover render, no balls)

## File modifications

- `Project1/Main.cpp` — instantiate `Castle` instead of `FortGate`; raise the centre cannon's elevation to `18°` for the door-hitting trajectory
- `Project1/Makefile` — add the new sources to `COMMON`
- `Project1/AGENTS.md` — extend the class table with Phase 4 entries
- `docs/code-walkthrough.md` — add Phase 4 walkthrough sections
- `docs/report.tex` and `docs/report.pdf` — rebuild with the new screenshots

## Out of scope (deliberate)

- Functional drawbridge (one that raises/lowers on a hinge)
- Watchtower defenders
- Hinges, knockers, torches on the doors
- Multiple castles, moat, drawbridge chain
- Shadows

These can be follow-up phases; Phase 4 is "make the wall a castle" + "the doors break" + "regenerate the report".

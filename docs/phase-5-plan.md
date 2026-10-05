# Phase 5 plan — full castle compound with moat and bridge

## What changes vs Phase 4

The Phase 4 gatehouse is preserved as the **main gatehouse** of a larger
**compound castle**. On top of it we add:

1. **Four corner towers** — big (8 m × 8 m × 8 m), one at each corner of the
   castle footprint, with arrow slits on every face, a crenellated parapet
   all the way around, a flagpole on top.
2. **Four curtain walls** — straight, between each pair of adjacent corner
   towers, so the compound is fully enclosed on all four sides.
3. **Moat** — a water-filled trench surrounding the castle footprint on the
   +X-facing side (the side the cannons are on), 4 m wide, 0.2 m deep.
4. **Stone bridge** — crosses the moat from the cannons' side to the main
   gatehouse door.
5. **Camera presets** — keys `1`, `2`, `3`, `4` instantly switch between
   four fixed camera angles.

The **cannons move to the far side of the moat** so they fire across the
water onto the gatehouse door (this is what the example pictures show —
cannons lined up on the opposite bank, bridge between them and the castle).

## Layout (top-down)

```
+---------------------------------------------------------------+
|                          TREES                                |
|                                                               |
|  C  ═══════════════════════════════════════════════════ C     |
|  T  │                                                   │ T   |
|  ║  │                                                   │ ║   |
|  ║  │                MAIN GATEHOUSE                     │ ║   |
|  ║  │              ┌─┐         ┌─┐                      │ ║   |
|  ║  │              │T│  door→  │T│   (flanking towers) │ ║   |
|  ║  │              └─┘ ─ ─ ─ ─ └─┘                      │ ║   |
|  ║  │                                                   │ ║   |
|  C  │                                                   │ C   |
|  ═════════════════════════════════════════════════════      |
|       M  M  M  M  M  M  M  M  M  M  M  M   ← moat (water)     |
|       ░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░   ← bridge          |
|                                                               |
|  [C1]  [C2]  [C3]    ← cannons, facing +X                    |
|                                                               |
+---------------------------------------------------------------+
```

(Looking from above with +X = right, +Z = towards the viewer.)

Coordinate plan:

* Castle centre: `centreWorld = (12, 0, 0)`
* Castle compound is **16 m wide × 16 m deep** centred on it (so corners
  at `(4, 0, -8)`, `(20, 0, -8)`, `(4, 0, +8)`, `(20, 0, +8)`).
* Main gatehouse at `x = 12` (middle of the +Z-facing wall — wait, let
  me revise; the gate faces +X toward the cannons).

Actually a cleaner layout — let me re-plan with the gate on the +X side:

* Compound centre: `(12, 0, 0)` — same as Phase 4.
* Compound size: 16 × 16 m, so corners at `(4, 0, -8)`, `(20, 0, -8)`,
  `(4, 0, +8)`, `(20, 0, +8)`.
* Main gatehouse at `x = 12`, **on the +X face** of the compound. This
  means the gate is in the curtain wall from `(12, ?, -2.5)` to `(12, ?, +2.5)`.

Hmm, that's awkward because the gatehouse needs to be on a wall, not in a
corner. Let me re-plan:

* Compound is a 16 × 16 m square centred at `(12, 0, 0)`. Its perimeter runs
  along x = 4 and x = 20, and along z = -8 and z = +8.
* Corner towers are at those four corner positions.
* Curtain walls fill the four edges.
* Main gatehouse: in the **+X-facing wall**, between corner towers at
  `(20, 0, -8)` and `(20, 0, +8)`. Wait, that's the same as the curtain
  wall. Hmm.

Let me reconsider. The reference images show a castle with a gatehouse
**sticking out of the wall** — there's a protruding structure with a door.

Simpler approach: gatehouse is its own free-standing piece, sitting in the
middle of the +X-facing curtain wall (replacing the middle section of that
wall). The curtain wall runs along z from -8 to +8 at x = 20; the middle
section from z=-2.5 to z=+2.5 is replaced by the gatehouse.

* Compound centre: `(12, 0, 0)` — same.
* Corner towers: 4 at `(4, 0, -8)`, `(20, 0, -8)`, `(4, 0, +8)`,
  `(20, 0, +8)`. Each is 4 m × 4 m, so corners fill 4×4 each.
* Curtain walls:
  * **+Z wall** (south): x from 4 to 20, z = +8. Length 16 m.
  * **-Z wall** (north): x from 4 to 20, z = -8.
  * **-X wall** (west): x = 4, z from -8 to +8.
  * **+X wall** (east): x = 20, but with the gatehouse in the middle.
* Main gatehouse: replaces the centre 5 m of the +X wall, from z = -2.5 to
  z = +2.5 at x = 20. Two flanking towers (Phase 4's Tower class) at
  `(20, 0, -2.5)` and `(20, 0, +2.5)`. FortGate and doors between them.
* Moat: surrounds the castle at x = 20 + 2 (so x ≈ 22 to 26). The moat
  extends in the +X direction by 4 m width. We could moat all 4 sides, but
  let's just put a +X moat since that's the side the action is on (matches the
  example photos: water is on the side where the cannons are).
* Bridge: crosses the moat from `(20, 0, 0)` outward to `(26, 0, 0)`,
  passing through the gatehouse door area.

Actually, let me look at the reference more carefully: the moat seems to
surround the entire castle on the side facing the viewer. Let me put the
moat on the +X side only (matching the camera/cannon side), and put the
bridge perpendicular to the wall.

Final layout:

| Element | Position | Size |
|---|---|---|
| Castle centre | (12, 0, 0) | reference point |
| Corner tower 1 (NW) | (4, 0, -8) | 4×8×4 |
| Corner tower 2 (NE) | (20, 0, -8) | 4×8×4 |
| Corner tower 3 (SW) | (4, 0, +8) | 4×8×4 |
| Corner tower 4 (SE) | (20, 0, +8) | 4×8×4 |
| Curtain wall N | z = +8, x ∈ [4, 20] | 16×3×0.6 |
| Curtain wall S | z = -8, x ∈ [4, 20] | 16×3×0.6 |
| Curtain wall W | x = 4, z ∈ [-8, +8] | 0.6×3×16 |
| Curtain wall E (split by gatehouse) | x = 20, two segments z ∈ [-8, -2.5] ∪ [+2.5, +8] | 0.6×3×5.5 each |
| Main gatehouse | x = 20, z ∈ [-2.5, +2.5] | Tower (Phase 4) flanking towers + FortGate + Door + drawbridge |
| Moat | x ∈ [22, 26], z ∈ [-6, +6] | 4 wide × 12 long × 0.2 deep water plane |
| Bridge | x ∈ [20, 26], z ∈ [-1, +1] | 6 long × 2 wide stone bridge over the moat |
| Cannons | x = -5, z ∈ [-2.5, 0, +2.5] | three at z = ±2.5 and 0, all facing +X |

Hmm wait, the +X wall has curtain E going from z = -8 to z = -2.5 (one
piece), and z = +2.5 to z = +8 (another piece), each 5.5 m long. The
gatehouse in the middle 5 m. But the existing `FortGate` was 5 m wide
(the brick stacks), and the existing Tower is 2.5 m wide. Two flanking
towers at 2.5 each plus the 5 m gate = 10 m total, but only 5 m of wall
was removed. Conflict — I need to make the gatehouse total exactly 5.5 m
wide (the gap I left) or remove more curtain wall.

Let me revise: the gatehouse is **5 m wide** (z = -2.5 to z = +2.5), with
two flanking towers each 2.5 m at z = ±3.75 and the FortGate in between
(5 m wide spanning z = -1.25 to z = +1.25). Then the curtain wall on +X
has gaps from z = -8 to -3.75 and from z = +3.75 to +8 (each 4.25 m).

Wait this is getting tangled. Let me start over with a simpler design.

## Simpler layout: gate on +Z

Let me face the gate on the +Z side instead of +X:

* Castle centre: `(0, 0, 12)`. Or keep at `(12, 0, 0)` but with +Z facing
  the camera.

Actually let me just KEEP the existing layout where everything is centred
around the gatehouse, and add corner towers + moat + bridge around it.

Phase 4 layout, centred at `centreWorld = (12, 0, 0)`:

* Main gatehouse: two flanking Towers at `(8.25, 0, 0)` and `(15.75, 0, 0)`
  (centred at x = 12 ± 3.75), with FortGate and Doors between them.
* Curtain walls: two segments extending ±6 m along X from each Tower
  (so left curtain from x = 1 to x = 7, right curtain from x = 17 to x =
  23), centred on z = 0.

For Phase 5, I'll keep this gatehouse in the centre, and add:

* **Corner towers** at the four corners of a 24 × 24 m compound centred at
  `(12, 0, 0)`:
  * `(0, 0, -12)`, `(24, 0, -12)`, `(0, 0, +12)`, `(24, 0, +12)`.
  * Each 4 m × 4 m × 8 m tall.
* **Curtain walls** between corner towers:
  * North: x from 0 to 24, z = -12.
  * South: x from 0 to 24, z = +12.
  * West: x = 0, z from -12 to +12.
  * East: x = 24, z from -12 to +12. (No gate on the east side.)
* **Main gatehouse** is on the **+Z side** of the compound — actually no,
  the existing Phase 4 gatehouse sits on the X axis (curtains extend along
  X). Let me think about this differently.

Actually the simplest design that achieves the user's goal:

* The **main gatehouse** (Phase 4's `Castle` object) is the prominent feature.
* It is surrounded by a **moat** on the cannon side (+X direction in the
  current code, where cannons are).
* A **bridge** crosses the moat.
* The **cannons** are on the far side of the moat.

For "Full compound: 4 corners + 1 gatehouse", I need to add corner towers
+ curtain walls forming a full enclosure, with the gatehouse on one side.

Let me re-orient: put the gatehouse on the **-Z** side of the compound (so
cannons at z = +∞ shoot in the -Z direction). Or keep the gatehouse where
it is and just rotate my thinking.

You know what, let me just go with the design the user asked for and not
over-think. Final layout:

* **Compound**: 24 × 24 m square centred at `(12, 0, 0)`.
  * NW corner tower at `(0, 0, 0)`.
  * NE corner tower at `(24, 0, 0)`.
  * SW corner tower at `(0, 0, 24)`.
  * SE corner tower at `(24, 0, 24)`.
* **Main gatehouse**: on the +X side, centred at `(24, 0, 12)`, with the
  door facing +X (toward the cameras/cannons). The gatehouse is 5 m wide
  along Z.
* **Curtain walls** form the perimeter except where the gatehouse is.
* **Moat** sits on the +X side of the compound at x ∈ [26, 30], z ∈ [8, 16].
* **Bridge** crosses from x = 24 (gate) to x = 30 (cannon side), at z ∈ [11, 13].
* **Cannons** at x = -5, z ∈ [10, 12, 14]. Each faces +X.

Wait, but I had the cannons facing +X in the existing code. If I move
them to face the gate at x=24 along +X, they're firing along +X. But the
cannons themselves are at x = -5, z = {10, 12, 14} — they face +X (toward
the castle at x=24). The cannon's local +X is world +X. Good.

So actually the easiest thing is to:
1. ROTATE the entire Phase 4 castle so its gate now faces +X (cannon
   direction). Currently the gate is at z=0 facing +Z or -Z direction. Let
   me check.

Looking at Phase 4 code: `Castle(vec3(12.0f, 0.0f, 0.0f), ...)`. The
FortGate bricks are at x ∈ [11.6, 12.4], z ∈ [gate width ± ...]. Looking at
FortGate.cpp: `leftPillarX = centreWorld.x - halfGate; rightPillarX =
centreWorld.x + halfGate`. So bricks are at x = 11 and x = 13. The doorway
is at z ∈ [-1, +1] (gateWidth = 2). So the gate faces ±Z direction.

In Phase 4, the cannons are at z = ±2.5 (leftCannon z=+2.5, rightCannon
z=-2.5), all facing +X. But the gate is in the X=12 wall facing ±Z. So
the cannons at z=0 fire toward x=12... but the gate is in the Z direction.
Hmm, that doesn't match.

Wait let me re-read. In Phase 4 Main.cpp: `Cannon centreCannon(vec3(0.0f,
0.0f, 0.0f));` — centre cannon at origin. Cannon default forward is +X.
Castle at `(12, 0, 0)`. So centre cannon fires from (0,0,0) toward +X.
The ball trajectory passes through (12, ?, 0). The castle's gate is at
x=12, z ∈ [-1, +1] (gateWidth = 2). So the ball flies STRAIGHT INTO the
gate at z=0. ✓

So actually the gate faces +X (the cannon direction) but its width is
along Z. The FortGate is a "wall" in the X=12 plane. From a top-down view,
the cannon at (0,0,0) fires toward the wall at x=12, hitting the gate at
z=0.

For Phase 5, the gate stays in this orientation. The moat goes in FRONT of
the wall (x ∈ [12, 16] maybe, with the wall at x=12 being the OUTER edge
of the castle, not the centre). The bridge crosses the moat.

Wait, but in Phase 4 the Castle is centred at x=12 with the gate there.
The curtain walls extend left and right (along X) from the towers. There's
no "moat side" — the castle has walls on left, right, and front (+Z side),
and the back is open.

Hmm. Let me look at the reference images again:

[Image 4](images/example-scene.jpg) shows:
* The castle has a gatehouse that **sticks OUT** of the main wall (forward,
  toward the viewer).
* A moat surrounds the gatehouse.
* A bridge crosses the moat.
* Cannons are on the FAR side of the moat (closest to viewer), facing the
  castle.
* The rest of the castle (behind the gatehouse) extends back into the
  distance.

So the layout is: castle compound behind, gatehouse jutting forward into
the moat, moat between viewer and castle, bridge from this side to gate,
cannons here.

For Phase 5 I'll do:

* Compound is a square (24×24 m) at `(12, 0, 0)`.
* Gatehouse is on the +X-facing side: replaces the central 5 m of the +X
  curtain wall, sticking out 1 m further into +X.
* Moat is between the gatehouse and the +X side of the compound, at x ∈
  [gatehouse+1, gatehouse+22].
* Bridge crosses the moat at z=0.
* Cannons at x = gatehouse+24, z = {0, ±2.5}.

OK let me just write this and stop second-guessing. Final plan:

### Coordinate system

X = forward (cannon firing axis), Y = up, Z = right.

### Compound

* Compound footprint: 24 × 24 m, centred at `(12, 0, 0)`.
* Corner positions: `(0, 0, ±12)` and `(24, 0, ±12)` (square corners).
* Each corner tower: 4 m × 8 m × 4 m, at a corner.

### Curtain walls

* North wall: x from 2 to 22, at z = -12 (16 m long, 3 m tall, 0.6 m thick).
* South wall: x from 2 to 22, at z = +12.
* West wall: z from -10 to -2 at x = 0 (left of gatehouse, 8 m long).
  Plus z from +2 to +10 at x = 0 (right of gatehouse, 8 m long).
* East wall: z from -10 to -2 at x = 24 (8 m). Plus z from +2 to +10 at x
  = 24 (8 m).
* Wait, the gatehouse is on the +X side, so I should split the EAST wall.
  Let me redo:

Actually the gatehouse should be on whichever side the cannons are on.
The cannons fire in the +X direction, so the gate should face -X (the
cannons). The cannons are at x = -5 firing in +X. So the gate is at the
WEST side of the castle, at x = 0.

Hmm, but that means the gatehouse's door opens into the moat on the
west side, and the bridge goes from x = -5 (cannons) over the moat to
x = 0 (door).

Let me redo:

* Compound is at centre `(12, 0, 0)`, with corners `(0, 0, ±12)`,
  `(24, 0, ±12)`. Side length 24.
* Walls: north z=-12, south z=+12, west x=0, east x=24.
* Gatehouse is on the WEST side (x=0), centred at `(0, 0, 0)`. Gate
  faces -X (toward the cannons).
* Cannons at x = -8, z = {0, ±2.5}, facing +X.
* Moat between cannons and gatehouse: x ∈ [-7, -1], z ∈ [-3, +3] (a
  strip 6 m wide, 6 m long along Z). The moat is centred at x = -4, z = 0.
* Bridge crosses the moat from (x=-1, z=0) to (x=-7, z=0), at z ∈
  [-1.5, +1.5]. The bridge is 6 m long along X, 3 m wide along Z, with
  some height.

This is clean. Let me lock this in.

### Camera presets

Keys `1`-`4`:
* **1**: Behind the cannons (the original Phase 4 view) — looking from
  `(−5, 3, 7)` toward `(8, 0.7, 0)`. Default.
* **2**: Front of castle — looking from `(35, 4, 12)` toward `(12, 2, 0)`.
* **3**: Side — looking from `(12, 6, 25)` toward `(12, 2, 0)`.
* **4**: Top-down — looking from `(12, 40, 5)` toward `(12, 0, 5)`.

### Things to build

| File | Role |
|---|---|
| `Water.h/.cpp` | Flat blue plane for the moat; accepts a centre and size. |
| `Bridge.h/.cpp` | A stone bridge: a long flat box (deck) plus two low rails on the sides. |
| `CornerTower.h/.cpp` | Larger Tower (4×8×4 m), more arrow slits. Reuses `Crenellation` and `Primitives`. |
| `CompoundCastle.h/.cpp` | Owns 4 corner towers + 4 curtain walls + 1 main gatehouse + bridge + moat. |

### Things to change in Main.cpp

* Switch from `Castle` to `CompoundCastle`.
* Add `Water` moat and `Bridge`.
* Move cannons to x = -8.
* Add 1-4 camera presets.

## What's kept

* All Phase 4 classes (`Tower`, `Door`, `Crenellation`, `Castle`, etc.) —
  the main gatehouse is still built from these.
* `FortGate`, `Door`, `Tree`, `Robot`.

## Out of scope

* Animated water surface.
* Trees in the castle courtyard (only outside).
* Stone-textured brick (we still use `Palette::Stone`).
* Real shadows.
* Bridge that can be raised/lowered.
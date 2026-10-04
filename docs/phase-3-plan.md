# Phase 3 Plan — Trees, Fort Gate, Replicated Cannons, Dummy Robot

Phase 2 made the cannon *do* something (fire a projectile that breaks a
wall). Phase 3 makes the world around the cannon *be* something — a small
medieval backdrop so the cannon has a stage to stand on.

All of these are intentionally simple, "first-pass" props — the brief is
"very basic", not photoreal. They share the same approach as the cannon
itself: each one is a small class that holds a list of `Part`s made from
`Primitives::Create*`, drawn the same way every other object is drawn.

## Step 1 — `Tree`

A tree is two pieces:

- a `Primitives::CreateCylinder(...)` for the trunk (brown bark colour),
- a `Primitives::CreateCone(...)` for the leaves (a green cone standing
  on top of the trunk — the brief calls it "a pyramid for the leaves", a
  cone is the closest primitive we have and reads identically at this
  scale).

The trunk sits at the tree's origin, the cone is offset upward by the
trunk height. A `Tree` just owns the two `Mesh`es and the trunk's
position, draws both inside `Draw(parentMatrix)`.

Phase 3 places six of them in a rough line behind the wall, so the
camera has something leafy to look at when it is not looking at the gun.

## Step 2 — `FortGate` (the front of a medieval fort)

The brief asks for "the medieval style fort's front part — not the whole
fort — just the gate and wall for the cannon ball break effect". So we are
not modelling a whole castle: we model **one short wall segment with a
gate-shaped hole in the middle**.

This replaces the simple brick wall from Phase 2:

- A row of bricks on each side of a rectangular doorway opening,
- a heavy lintel bar across the top of the doorway (a single thick box),
- two short side pillars anchoring the lintel.

A hit from a cannon ball removes individual bricks the same way Phase 2
did, except the wall now has a recognizable shape with a gate in it.

`Wall` becomes `FortGate` in the source (same logic, more interesting
geometry). The collision routine stays the same axis-aligned box test.

## Step 3 — `Robot` (wooden dummy, 18th-century automaton style)

The brief asks for a simple, iron-and-gears-looking dummy robot — cubic
body, cubic head, cylindrical arms and legs. We model it that way,
literally:
- a `CreateBox` torso (a darker iron colour from `Palette.h`),
- a smaller `CreateBox` head on top of the torso,
- two `CreateCylinder` arms hanging down the sides,
- two `CreateCylinder` legs going down to the ground.

It stands in a fixed spot inside the fort gate opening, so when the player
fires through the broken wall they have a target. It is decorative — it
does not animate in Phase 3 — but everything is wired through the same
`Part`-and-`Local::*` plumbing so it could later be made to walk without
touching the cannon code.

## Step 4 — Multiple cannons

The brief says "replicate cannons side by side". We achieve that by
turning the existing cannon into a small `Cannon` class that owns one
`Carriage`, two `Wheel`s and one `Shaft`, and instantiating it three
times in a row across the field.

Each `Cannon` instance has its own `MoveForward`, `Elevate`, and `Fire`,
so they can be aimed and fired independently. For Phase 3 only the
*centre* cannon takes keyboard input (it is the one the camera is
framed on); the outer two are parked, but their parts are real so the
visual is convincing.

## Step 5 — Scene assembly in `Main.cpp`

`Main.cpp` becomes:

1. one ground plane (unchanged),
2. one `FortGate` with the gate opening facing the guns,
3. six `Tree`s behind the fort,
4. three `Cannon`s in a row, the middle one keyboard-controlled,
6. one `Robot` inside the gate opening,
7. one `Projectile` list (unchanged from Phase 2),
8. one input loop with Spacebar firing from the centre cannon only.

## Definition of done for Phase 3

- [ ] `mingw32-make run` opens a window showing: three cannons in a row,
      a small fort-gate wall in front of them, six trees behind the
      fort, and a wooden dummy robot inside the gate opening.
- [ ] Pressing Spacebar still launches a cannon ball that arcs through
      the gate and breaks bricks on the far side.
- [ ] All Phase 1 / Phase 2 behaviour still works (driving, elevation,
      wall hits).
- [ ] All new classes are under ~150 lines each and use the existing
      `Primitives` and `Part` plumbing.
- [ ] `docs/code-walkthrough.md` has new sections for each Phase 3
      class.

## Explicitly out of scope

This is the last phase for this project. Out of scope: sound, multiple
light sources, shadow mapping, post-processing, fog, skybox texturing,
more advanced physics (collision response, debris), animation of the
robot or the trees. The brief is "very basic" — Phase 3 stays basic.
# Phase 2 Plan — Cannon Balls, Projectile Physics, Breaking Wall

This phase adds the *acting* part of the cannon: the cannon ball that flies out
of the muzzle, the gravity that pulls it back down, and a wall that breaks when
the ball hits it. Everything else from the original spec (Phong lighting,
recoil, camera presets, two contrasting materials) was either already pulled
into Phase 1 or is left for Phase 3 — see [phase-3-plan.md](phase-3-plan.md).

The build stays grounded in the same rules as Phase 1:

- Each new concept is one small class.
- Each new class reuses `Primitives::Create*` and `Part` + `Local::*`, the same
  way `Wheel` and `Shaft` already do.
- No new math is reinvented — projectile motion is the textbook semi-implicit
  Euler, the same integrator style as the Python version in
  `docs/verification.txt` §2.

## Step 1 — `Projectile`: a single cannon ball in flight

A `Projectile` is just a sphere mesh plus a velocity vector. Each frame:

```
position += velocity * deltaTime
velocity.y -= gravity * deltaTime
```

- Spawn position is the *world-space muzzle tip* of the gun at the moment of
  fire — exactly the use case for hierarchical transforms (see
  `docs/verification.txt` §2/§6 from the Python build): compute
  `carriageMatrix * shaftTransform * muzzleLocal` and that's where the ball
  appears.
- Spawn velocity is the unit forward direction (X-axis, rotated by the
  current elevation) multiplied by a fixed muzzle speed.
- The ball is removed when its `y` goes negative (it has hit the ground) or
  after a generous lifetime cap so a missed shot does not pile up forever.
- One `std::vector<Projectile>` in `Main.cpp`; spacebar `push_back`s a new one.

Why Euler is enough here: the time step is small (~16 ms at 60 fps), the
trajectory is short (the wall is in front of the gun), and a tiny bias toward
the ground on a parabolic arc is invisible at this scale. The Python version
used the same method — see `docs/verification.txt` §2 for the closed-form
comparison the Python numbers were checked against.

### A small muzzle helper

Rather than make `Shaft` itself know about `Projectile`, add one
`GetMuzzleWorldPosition(...)` method on `Shaft` that, given a parent matrix,
returns the world-space tip of the barrel. `Main.cpp` uses it to spawn each
shot, so the ball always comes out of the actual current muzzle, no matter
how the gun has been driven or aimed.

## Step 2 — `Wall` (the breakable thing)

The wall is a single `Mesh` of stacked bricks, sitting at a fixed position in
front of the gun. It owns its own `std::vector<Brick>` where each brick is one
`Part` plus an `alive` flag.

- On startup every brick is `alive = true`.
- After every projectile update, `Main.cpp` calls `wall.CheckHit(ball)` for
  each live ball: a simple sphere-vs-AABB test (the brick is axis-aligned, so
  this is just six per-axis range comparisons).
- A dead brick disappears by simply being skipped during `Draw()`. There is no
  fancy destruction animation — "the ball punched a hole in the wall" reads
  fine on a still frame, and Phase 3 may add falling debris on top later.

The wall is not a scene-graph child of the gun — it sits in world space at a
fixed distance from the origin, exactly where the user can aim at it.

## Step 3 — Input + integration

- **Spacebar** fires: spawn one `Projectile` at the muzzle tip with the
  current elevation baked into its velocity.
- The left/right and up/down arrows keep doing what they did in Phase 1.
- The wall is drawn each frame after the projectile update loop so a brick
  can disappear the same frame it's hit.

The projectile list and the wall's brick list are the only new state in
`Main.cpp`. Everything else (carriage, wheels, barrel) stays exactly as it
was — that is the whole point of building the foundation in Phase 1.

## Definition of done for Phase 2

- [ ] `mingw32-make run` still works.
- [ ] Pressing spacebar launches a cannon ball out of the muzzle; it follows
      a visible parabolic arc and lands on the ground.
- [ ] A wall of bricks sits in front of the gun. When a cannon ball hits a
      brick, that brick disappears in the same frame.
- [ ] Multiple balls can be in flight at once.
- [ ] All new classes are under ~150 lines and reuse the existing primitives.
- [ ] `docs/code-walkthrough.md` has a new section for `Projectile` and
      `Wall`.

## Explicitly out of scope (Phase 3 territory)

A tree-lined backdrop, multiple cannons, the medieval fort gate, the wooden
dummy robot, falling-brick debris animation, sound, mouse-look camera. Those
are documented in [phase-3-plan.md](phase-3-plan.md).
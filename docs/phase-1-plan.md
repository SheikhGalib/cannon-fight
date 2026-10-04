# Phase 1 Plan — Wheels + a Movable Shaft

## Goal

Get the smallest possible piece of the cannon on screen in C++/OpenGL, built the
"right" way (small reusable classes), so every later phase is just *adding* to a
foundation that already works — never rewriting it.

Phase 1 delivers exactly:

- Two wheels (cylinders), sitting in fixed positions.
- One shaft (a long cylinder standing in for the cannon barrel), mounted at a
  pivot point.
- The shaft can be rotated up/down with the arrow keys ("elevation"), clamped
  to a sensible range. That's the one moving part this phase asks for.

Nothing else. No carriage body, no rolling, no lighting, no firing — those are
all Phase 2 (see [phase-2-plan.md](phase-2-plan.md)).

## Why this scope (and not more)

[project-context.md](project-context.md) is the full spec for the original
Python project (which `legacy/` still contains, working, as a reference). It
lists build milestones 1–7. Phase 1 here corresponds to *starting* milestone 2
("static carriage") and *starting* milestone 4 ("barrel + elevation"), but
skipping the carriage frame box, the wheel-rolling animation, and lighting —
those come back in Phase 2. Building two primitives and one control first means
every part of the toolchain (Makefile, GLM math, shader plumbing, keyboard
input) gets proven out while there's still almost nothing to debug.

## Design (the classes, and why they exist)

Each class has one job — that's the "S" in SOLID (Single Responsibility) the
user asked for. None of them know about each other's internals.

| Class | File | Responsibility |
|---|---|---|
| `Mesh` | `Mesh.h/.cpp` | Owns one shape's GPU buffers (VAO/VBO/EBO) and knows how to draw itself. Doesn't know what shape it is. |
| `Primitives::CreateCylinder(...)` | `Primitives.h/.cpp` | A *function*, not a class — generates the vertex/index data for a cylinder and hands back a ready-to-draw `Mesh`. Both wheels and the shaft call this same function with different numbers, which is the "reusable function" the user asked for: one geometry generator, many objects. |
| `Transform` | `Transform.h/.cpp` | Holds a position, one rotation (axis + angle), and a scale, and turns them into a single 4x4 matrix (`GetMatrix()`). This is the reusable move/rotate math every object shares. |
| `Wheel` | `Wheel.h/.cpp` | A domain object: "a wheel at this position, this size." Owns a `Mesh` + `Transform`, draws itself given a shader and a parent matrix. |
| `Shaft` | `Shaft.h/.cpp` | Same idea for the barrel stand-in: owns a `Mesh` + `Transform`, plus an `elevationDegrees` value and an `Elevate(delta)` method that changes it (clamped). |

`Main.cpp` just creates one `Shader`, two `Wheel`s, one `Shaft`, and each frame:
reads the arrow keys, calls `shaft.Elevate(...)`, then calls `Draw()` on each
object. No object reaches into another's internals — that's what keeps this
easy to extend without breaking things later (Open/Closed: Phase 2 adds new
classes like `Carriage` instead of editing these ones).

### Why one `Transform` per object is enough for now

A `Transform` can only express *one* rotation axis at a time. That's fine for
Phase 1:
- A wheel's only rotation is a fixed 90° turn so the cylinder (normally
  standing up) lies on its side, axle pointing sideways. One axis.
- The shaft's rotation is "point forward" (a fixed −90° turn) *plus* the
  elevation angle — but both of those happen to turn around the same axis (the
  sideways axis, Z), so they add together into one number. Still one axis.

Phase 2's wheel *rolling* needs a second, independent rotation (spin around the
axle) on top of the fixed lie-on-its-side turn. `Transform` will need to grow
to support that (documented in phase-2-plan.md) — but there's no point adding
that capability now when nothing uses it yet (YAGNI).

## What "movable" means here

Holding **Up** tips the shaft up, **Down** tips it back down, clamped to
0°–45° (the same range the Python version and the spec use). It's driven by
polling `glfwGetKey` once per frame in the render loop — the simplest possible
input handling, easy to read and easy to extend (Phase 2 will add the
left/right drive keys the same way).

## Definition of done for Phase 1

- [ ] `mingw32-make run` opens a window and shows two wheel shapes and one
      shaft shape, positioned so the shaft looks mounted between/above the
      wheels.
- [ ] Holding Up/Down visibly rotates the shaft between 0° and 45° and stops
      cleanly at the limits.
- [ ] Every new file is under 100–150 lines and does one clearly-named thing.
- [ ] `docs/code-walkthrough.md` exists and explains each new file and the key
      lines, in plain language, so the user can explain it to the teacher.
- [ ] `docs/running-guide.md` exists and a person who has never touched this
      project can build and run it from it alone.

## Explicitly out of scope for Phase 1

Carriage frame, wheel rolling / forward-backward movement, recoil, full Phong
materials (wood vs. metal contrast), projectile, camera controls beyond a
fixed view. All of this is Phase 2 — see [phase-2-plan.md](phase-2-plan.md).

## Revision: basic lighting pulled forward from Phase 2

The first working version of Phase 1 used flat, unlit vertex colors (the
original `default.vert`/`default.frag` pair) — every cylinder rendered as a
single-color silhouette with no visible roundness, which in practice looked
broken, not just "unfinished." Rather than ship that and wait for Phase 2's
full lighting step to fix it, a **small, deliberately partial** slice of
lighting was pulled forward:

- New `lit.vert`/`lit.frag` shaders (the old `default.vert`/`default.frag`
  are left in place, untouched, as the simplest possible reference pair).
- `Primitives::CreateCylinder`/`CreatePlane` now bake a real per-vertex
  normal into every vertex (see `docs/code-walkthrough.md` for how), and
  `Mesh`'s vertex layout grew from 6 floats (position + color) to 9
  (position + normal + color).
- `lit.frag` does **ambient + one diffuse term** against a single fixed
  directional light — not the full ambient+diffuse+specular Phong model, and
  no wood-vs-metal material contrast yet. That's still genuinely Phase 2
  Step 4's job (see `phase-2-plan.md`); this revision only goes as far as
  "shapes read as solid 3D forms instead of flat blobs."
- Also added: a ground plane (`Primitives::CreatePlane`, drawn directly in
  `Main.cpp`, no class of its own since it doesn't move), a two-tone
  tire+hub look for `Wheel` (two concentric cylinders sharing one
  `Transform`), and two decorative brass bands on `Shaft` (using a new
  `yOffset` parameter on `CreateCylinder`, so a ring can be positioned partway
  along a longer cylinder without needing its own `Transform`). None of these
  add new classes or change the object model from the rest of this document
  — `Wheel` and `Shaft` still each own their own geometry + one `Transform`
  and still take a `parentMatrix` in `Draw()` exactly as described above.

Everything else in this document (scope, class responsibilities, definition
of done) is unchanged.

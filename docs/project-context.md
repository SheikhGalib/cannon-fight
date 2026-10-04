# Medieval Field Cannon — OpenGL Project Kickoff Prompt

Use this as a spec for yourself, or paste it directly into an AI coding
assistant (Claude, ChatGPT, Claude Code, etc.) to get started on real code.

## Project summary

Build a 3D scene in Python using PyOpenGL (with GLUT/freeglut for windowing)
depicting a medieval field cannon on a wheeled carriage. The user can move
the cannon forward/backward (wheels rotate to match distance traveled),
elevate the barrel to a chosen angle, and optionally fire a cannonball that
follows a projectile arc while the barrel recoils and springs back. The
project must demonstrate hierarchical transformations and Phong lighting
with two contrasting materials (wood carriage, metal barrel).

## Tech stack

- Python 3.11+
- PyOpenGL + PyOpenGL_accelerate
- GLUT / freeglut for window + input handling
- numpy for vector/matrix math (optional but recommended)

## Scene hierarchy (this is the core structure — keep it shallow)

```
Carriage (root, translates along ground)
├── Wheel (left)   — rotates about its own axle as carriage moves
├── Wheel (right)  — same, mirrored position
└── Barrel Mount (fixed to carriage, pivot point for elevation)
    └── Barrel     — rotates about the mount's pivot for elevation;
                      translates along its own axis briefly for recoil
```

Build each part from basic primitives (cubes/boxes for the carriage frame,
cylinders for wheels and barrel, a cone or truncated cylinder for the
muzzle end if you want detail). No external 3D models needed.

## Required transformations

1. **Translation** — moving the whole carriage forward/backward.
2. **Rotation (rolling)** — wheel spin angle tied directly to distance
   traveled (`wheel_angle += distance_moved / wheel_radius`), same trick
   used for any rolling wheel/car.
3. **Rotation (elevation)** — barrel rotates about the mount pivot,
   clamped to a sensible range (e.g. 0° to 45°).
4. **(Optional) Translation (recoil)** — barrel briefly slides back along
   its own axis on fire, then eases back to rest position.
5. **(Optional) Projectile motion** — cannonball spawned at the muzzle tip,
   moves under simple gravity (`x += vx*dt`, `y += vy*dt; vy -= g*dt`).

## Controls (suggested keybinds)

| Key | Action |
|---|---|
| Left / Right arrow | Move cannon backward / forward |
| Up / Down arrow | Elevate / lower barrel |
| Spacebar | Fire (triggers recoil + projectile, if implemented) |
| L | Toggle light on/off or cycle light position |
| C | Cycle camera angle (optional) |

## Lighting requirements

- Implement Phong lighting: ambient + diffuse + specular components.
- Use one directional or point light (simulating sunlight or a torch).
- Give the carriage a matte "wood" material (low specular) and the barrel
  a shiny "metal" material (high specular, tighter highlight) so the
  contrast is visually obvious — this is your clearest way to show the
  lighting model is actually working, not just hardcoded colors.

## Suggested file structure

```
MedievalCannonOpenGL/
├── main.py            # window setup, main loop, input handling
├── cannon.py           # Cannon class: draws carriage, wheels, barrel; holds state (position, elevation angle, recoil offset)
├── lighting.py         # light + material setup helpers
├── projectile.py       # (optional) cannonball physics + drawing
└── utils.py            # shared helpers (primitive drawing, matrix helpers)
```

## Build milestones (do these in order — don't skip ahead)

1. **Window + camera**: get a window open with a perspective projection
   and a fixed camera looking at the origin. Confirm you can see a single
   test cube.
2. **Static carriage**: build the carriage body + 2–4 wheels as static
   geometry, correctly positioned relative to each other.
3. **Wheel rolling**: add keyboard movement, tie wheel rotation to
   distance traveled.
4. **Barrel + elevation**: add the barrel mount and barrel, add
   keyboard-controlled elevation rotation about the correct pivot point.
5. **Lighting**: add a light source and Phong shading, apply the two
   materials, confirm the specular highlight moves correctly on the
   barrel when you rotate the camera or move the light.
6. **(Optional) Recoil + firing**: add the recoil animation and
   projectile arc on spacebar.
7. **Polish**: ground plane, background color, maybe a simple skybox
   color gradient — keep this minimal, it's not graded heavily.

## Rubric self-check before submitting

Make sure your final project visibly demonstrates each of these, since
these are almost certainly what's being graded:

- [ ] Translation (cannon movement)
- [ ] Rotation (wheel spin + barrel elevation)
- [ ] Hierarchical/composite transformations (parent-child parts moving together correctly)
- [ ] Perspective projection + a working camera
- [ ] Phong lighting: ambient, diffuse, specular all visibly present
- [ ] At least two distinct materials showing different lighting response
- [ ] Some form of continuous animation/interactivity (not just a static scene)

Keep the scope here — don't add terrain, multiple cannons, or complex
physics. A clean, correctly-working version of exactly this is a strong,
easy-to-explain first project.
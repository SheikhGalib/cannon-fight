# Running Guide — Build & Run `Project1` (Phase 1)

For a from-scratch machine setup (installing the compiler etc.), see the
"Toolchain" section of [../AGENTS.md](../AGENTS.md) — this guide assumes that
part is already done, i.e. `g++ --version` and `mingw32-make --version` both
work in your terminal.

## 1. Open a terminal in the right place

You need a shell (PowerShell, or VS Code's built-in terminal) that can see
`mingw32-make` and `g++` on its PATH. If you installed the toolchain
recently, **restart the terminal/VS Code window** first — a shell opened
before the PATH was updated won't see the new tools.

Check it works:

```powershell
g++ --version
mingw32-make --version
```

Both should print a version number, not "command not found."

## 2. Build

```powershell
cd Project1
mingw32-make
```

This compiles everything (`Main.cpp`, `Wheel.cpp`, `Shaft.cpp`, `Mesh.cpp`,
`Primitives.cpp`, `Transform.cpp`, plus the pre-existing `VAO`/`VBO`/`EBO`/
`shaderClass`/`glad` files) into `build_mingw\app.exe`. If it prints nothing
and returns you to the prompt, it worked — g++ only prints on warnings or
errors.

## 3. Run

Either:

```powershell
mingw32-make run
```

(builds first if anything changed, then runs it in one step), or, if it's
already built:

```powershell
.\build_mingw\app.exe
```

**Important:** run it from inside `Project1\`, not from inside
`build_mingw\`. The shader files (`default.vert`, `default.frag`) are loaded
by a relative path from wherever you launch the program, and they live in
`Project1\`, not in `build_mingw\`.

## 4. What you should see

A window titled "Medieval Cannon - Phase 1: Wheels + Shaft" (1000x700), a
sky-blue background over a green ground plane, showing:
- Two wheels, side by side — each a dark tire with a lighter wood-colored
  hub in the middle.
- One dark iron-colored shaft (the barrel stand-in) with two thin brass
  bands around it, mounted above and between the wheels, pointing forward.
- Everything shaded by a fixed light (brighter on the side facing it, dimmer
  on the side facing away) so the shapes read as solid 3D forms, not flat
  colored silhouettes.

There's no carriage frame connecting the wheels and the shaft yet, and no
spokes on the wheels — that's expected; see `docs/phase-2-plan.md` Step 1.

**Controls:**

| Key | Action |
|---|---|
| `↑` (Up arrow) | Tip the shaft up (elevate), up to 45° |
| `↓` (Down arrow) | Tip the shaft back down, down to 0° |
| `Esc` | Quit |

Holding Up/Down should smoothly rotate the shaft and stop cleanly at the
limits — it shouldn't be possible to flip it upside down or past horizontal.

## 5. Cleaning up

```powershell
mingw32-make clean
```

Deletes `build_mingw\app.exe` so the next `mingw32-make` rebuilds everything
from scratch. This never touches your source files (`.cpp`/`.h`), only the
compiled output.

## Troubleshooting

- **"g++ is not recognized" / "mingw32-make is not recognized"** — the
  toolchain isn't on PATH in this shell. See the Toolchain section of
  `AGENTS.md`; you likely just need to restart the terminal.
- **Window opens but is black / program exits immediately with an error
  printed to the terminal** — that's almost always a shader compile error
  (check the terminal output for `SHADER_COMPILATION_ERROR`) or the program
  being run from the wrong folder (see step 3 — `default.vert`/`default.frag`
  must be found relative to the current directory).
- **Link errors mentioning `undefined reference`** — check the `Makefile`'s
  `SRCS` line includes every `.cpp` file you've added; a new class's `.cpp`
  file has to be listed there to actually get compiled and linked in.
- **`ChatGPT.cpp` causes a "duplicate symbol" / multiple `main` error** — it
  is scratch code, intentionally not in the `Makefile`'s `SRCS` list. Don't
  add it.

## Running the old Python version (for comparison)

The original Python/PyOpenGL implementation still exists and works, kept in
[`legacy/`](../legacy/) for reference:

```powershell
.\legacy\venv\Scripts\Activate.ps1
python legacy\main.py
```

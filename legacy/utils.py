"""
utils.py
========

Low-level drawing helpers for the Medieval Field Cannon project.

Everything here is built from analytic geometry so that *every vertex carries a
correct outward normal*.  Correct normals are the whole reason the Phong
lighting in `lighting.py` looks right, so the normals are derived rather than
guessed (see the report, section "Analytic normals for a truncated cone").

All meshes are generated once and baked into OpenGL **display lists**; the
per-frame cost then collapses to a single `glCallList`.
"""

import math

from OpenGL.GL import *
from OpenGL.GLUT import *

TAU = 2.0 * math.pi

# --------------------------------------------------------------------------
# display-list cache
# --------------------------------------------------------------------------

_LIST_CACHE = {}


def _cached(key, builder):
    """Return a display list for `key`, building it with `builder()` once."""
    lst = _LIST_CACHE.get(key)
    if lst is None:
        lst = glGenLists(1)
        glNewList(lst, GL_COMPILE)
        builder()
        glEndList()
        _LIST_CACHE[key] = lst
    return lst


def clear_cache():
    for lst in _LIST_CACHE.values():
        glDeleteLists(lst, 1)
    _LIST_CACHE.clear()


# --------------------------------------------------------------------------
# primitives
# --------------------------------------------------------------------------

def _build_box(sx, sy, sz):
    hx, hy, hz = sx * 0.5, sy * 0.5, sz * 0.5
    faces = (
        ((0, 0, 1), ((-hx, -hy, hz), (hx, -hy, hz), (hx, hy, hz), (-hx, hy, hz))),
        ((0, 0, -1), ((hx, -hy, -hz), (-hx, -hy, -hz), (-hx, hy, -hz), (hx, hy, -hz))),
        ((1, 0, 0), ((hx, -hy, hz), (hx, -hy, -hz), (hx, hy, -hz), (hx, hy, hz))),
        ((-1, 0, 0), ((-hx, -hy, -hz), (-hx, -hy, hz), (-hx, hy, hz), (-hx, hy, -hz))),
        ((0, 1, 0), ((-hx, hy, hz), (hx, hy, hz), (hx, hy, -hz), (-hx, hy, -hz))),
        ((0, -1, 0), ((-hx, -hy, -hz), (hx, -hy, -hz), (hx, -hy, hz), (-hx, -hy, hz))),
    )
    glBegin(GL_QUADS)
    for n, verts in faces:
        glNormal3f(*n)
        for v in verts:
            glVertex3f(*v)
    glEnd()


def box(sx, sy, sz):
    """Axis-aligned box of the given size, centred on the origin."""
    glCallList(_cached(('box', round(sx, 5), round(sy, 5), round(sz, 5)),
                       lambda: _build_box(sx, sy, sz)))


def _build_frustum(r0, r1, h, slices, stacks, caps):
    """Truncated cone along +Z, from z=0 (radius r0) to z=h (radius r1).

    Lateral surface  P(phi,t) = ((r0+(r1-r0)t)cos phi, (...)sin phi, h t)
    Outward normal   N(phi)   = normalize(h cos phi, h sin phi, r0-r1)
    (derivation: N = dP/dphi x dP/dt, then divide out the radius factor).
    """
    dr = r1 - r0
    nz = r0 - r1
    inv = 1.0 / math.hypot(h, dr) if (h or dr) else 1.0
    for j in range(stacks):
        t0 = j / stacks
        t1 = (j + 1) / stacks
        z0, z1 = h * t0, h * t1
        ra, rb = r0 + dr * t0, r0 + dr * t1
        glBegin(GL_QUAD_STRIP)
        for i in range(slices + 1):
            phi = TAU * i / slices
            c, s = math.cos(phi), math.sin(phi)
            glNormal3f(c * h * inv, s * h * inv, nz * inv)
            glVertex3f(rb * c, rb * s, z1)
            glVertex3f(ra * c, ra * s, z0)
        glEnd()
    if caps:
        if r0 > 1e-6:
            _build_disk(r0, 0.0, -1, slices)
        if r1 > 1e-6:
            _build_disk(r1, h, 1, slices)


def _build_disk(r, z, sign, slices):
    glBegin(GL_TRIANGLE_FAN)
    glNormal3f(0.0, 0.0, float(sign))
    glVertex3f(0.0, 0.0, z)
    rng = range(slices + 1) if sign > 0 else range(slices, -1, -1)
    for i in rng:
        phi = TAU * i / slices
        glVertex3f(r * math.cos(phi), r * math.sin(phi), z)
    glEnd()


def cylinder(r0, r1, h, slices=32, stacks=1, caps=True):
    """Cylinder / truncated cone along +Z starting at the origin."""
    key = ('cyl', round(r0, 5), round(r1, 5), round(h, 5), slices, stacks, caps)
    glCallList(_cached(key, lambda: _build_frustum(r0, r1, h, slices, stacks, caps)))


def disk(r, slices=32):
    """Flat disk in the z=0 plane facing +Z."""
    glCallList(_cached(('disk', round(r, 5), slices),
                       lambda: _build_disk(r, 0.0, 1, slices)))


def _build_tube(ri, ro, h, slices):
    """Hollow ring (annulus extruded along +Z): outer wall, inner wall with
    inward normals, and two flat annular end caps.  Used for wheel rims."""
    # outer wall
    _build_frustum(ro, ro, h, slices, 1, False)
    # inner wall -- same surface with the normal flipped so it lights correctly
    glBegin(GL_QUAD_STRIP)
    for i in range(slices + 1):
        phi = TAU * i / slices
        c, s = math.cos(phi), math.sin(phi)
        glNormal3f(-c, -s, 0.0)
        glVertex3f(ri * c, ri * s, 0.0)
        glVertex3f(ri * c, ri * s, h)
    glEnd()
    # end caps
    for z, sign in ((0.0, -1.0), (h, 1.0)):
        glBegin(GL_QUAD_STRIP)
        glNormal3f(0.0, 0.0, sign)
        rng = range(slices + 1) if sign > 0 else range(slices, -1, -1)
        for i in rng:
            phi = TAU * i / slices
            c, s = math.cos(phi), math.sin(phi)
            glVertex3f(ro * c, ro * s, z)
            glVertex3f(ri * c, ri * s, z)
        glEnd()


def tube(ri, ro, h, slices=32):
    key = ('tube', round(ri, 5), round(ro, 5), round(h, 5), slices)
    glCallList(_cached(key, lambda: _build_tube(ri, ro, h, slices)))


def _build_sphere(r, slices, stacks):
    for j in range(stacks):
        th0 = math.pi * j / stacks
        th1 = math.pi * (j + 1) / stacks
        glBegin(GL_QUAD_STRIP)
        for i in range(slices + 1):
            phi = TAU * i / slices
            for th in (th1, th0):
                nx = math.sin(th) * math.cos(phi)
                ny = math.cos(th)
                nz = math.sin(th) * math.sin(phi)
                glNormal3f(nx, ny, nz)          # unit sphere: N = P/r
                glVertex3f(r * nx, r * ny, r * nz)
        glEnd()


def sphere(r, slices=24, stacks=16):
    glCallList(_cached(('sph', round(r, 5), slices, stacks),
                       lambda: _build_sphere(r, slices, stacks)))


def _build_torus(R, r, rings, sides):
    for i in range(rings):
        a0 = TAU * i / rings
        a1 = TAU * (i + 1) / rings
        glBegin(GL_QUAD_STRIP)
        for j in range(sides + 1):
            b = TAU * j / sides
            cb, sb = math.cos(b), math.sin(b)
            for a in (a1, a0):
                ca, sa = math.cos(a), math.sin(a)
                # tube centre C = (R ca, R sa, 0);  N = (ca cb, sa cb, sb)
                glNormal3f(ca * cb, sa * cb, sb)
                glVertex3f((R + r * cb) * ca, (R + r * cb) * sa, r * sb)
        glEnd()


def torus(R, r, rings=32, sides=16):
    """Torus in the z=0 plane (tube centre circles the +Z axis)."""
    glCallList(_cached(('tor', round(R, 5), round(r, 5), rings, sides),
                       lambda: _build_torus(R, r, rings, sides)))


def _build_grid_plane(size, divisions):
    """Tessellated ground quad in y=0.  Tessellation matters for the
    fixed-function (per-vertex) path: a single huge quad would sample the
    point light at only four corners."""
    step = 2.0 * size / divisions
    glNormal3f(0.0, 1.0, 0.0)
    for i in range(divisions):
        x0 = -size + i * step
        x1 = x0 + step
        glBegin(GL_QUAD_STRIP)
        for j in range(divisions + 1):
            z = -size + j * step
            glVertex3f(x1, 0.0, z)
            glVertex3f(x0, 0.0, z)
        glEnd()


def grid_plane(size=30.0, divisions=60):
    glCallList(_cached(('grid', round(size, 3), divisions),
                       lambda: _build_grid_plane(size, divisions)))


# --------------------------------------------------------------------------
# convenience wrappers  (primitives are authored along +Z)
# --------------------------------------------------------------------------

def cylinder_along_x(r0, r1, length, slices=32, stacks=1, caps=True):
    """Cylinder growing along +X -- used for the barrel."""
    glPushMatrix()
    glRotatef(90.0, 0.0, 1.0, 0.0)
    cylinder(r0, r1, length, slices, stacks, caps)
    glPopMatrix()


def cylinder_along_y(r0, r1, length, slices=32, stacks=1, caps=True):
    glPushMatrix()
    glRotatef(-90.0, 1.0, 0.0, 0.0)
    cylinder(r0, r1, length, slices, stacks, caps)
    glPopMatrix()


# --------------------------------------------------------------------------
# 2D text overlay
# --------------------------------------------------------------------------

def begin_2d(width, height):
    """Switch to a pixel-space orthographic projection for HUD drawing."""
    glMatrixMode(GL_PROJECTION)
    glPushMatrix()
    glLoadIdentity()
    glOrtho(0.0, max(width, 1), 0.0, max(height, 1), -1.0, 1.0)
    glMatrixMode(GL_MODELVIEW)
    glPushMatrix()
    glLoadIdentity()


def end_2d():
    glMatrixMode(GL_PROJECTION)
    glPopMatrix()
    glMatrixMode(GL_MODELVIEW)
    glPopMatrix()


def text(x, y, s, font=None, color=(1.0, 1.0, 1.0)):
    font = font or GLUT_BITMAP_9_BY_15
    glColor3f(*color)
    glRasterPos2f(x, y)
    for ch in s:
        glutBitmapCharacter(font, ord(ch))


def filled_rect(x, y, w, h, color):
    glColor4f(*color)
    glBegin(GL_QUADS)
    glVertex2f(x, y)
    glVertex2f(x + w, y)
    glVertex2f(x + w, y + h)
    glVertex2f(x, y + h)
    glEnd()


def clamp(v, lo, hi):
    return lo if v < lo else (hi if v > hi else v)


def lerp(a, b, t):
    return a + (b - a) * t

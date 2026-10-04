"""
cannon.py
=========

The medieval field cannon: geometry, state and the transformation hierarchy.

Local frame convention
----------------------
    +X  forward (the direction the barrel points at 0 deg elevation)
    +Y  up
    +Z  to the right (the wheel axle runs along Z)

Scene graph (kept deliberately shallow, as the spec asks)::

    Carriage  T(x, 0, 0)                     <- translation along the ground
      |
      +-- Wheel L   T(0, R, -track) Rz(phi)  <- rolling rotation, phi = -x/R
      +-- Wheel R   T(0, R, +track) Rz(phi)
      +-- Axle, trail beams, transoms, cheeks  (rigid with the carriage)
      +-- Barrel mount  T(p)                 <- trunnion pivot
            |
            +-- Barrel  Rz(theta) T(-rho, 0, 0)
                         ^elevation  ^recoil along the barrel's own axis

The composite matrix for a barrel vertex is therefore

    M_barrel = T(x,0,0) . T(p) . Rz(theta) . T(-rho,0,0)

and `muzzle_state()` evaluates exactly that product with numpy so the
cannonball is spawned at the true muzzle tip, whatever the elevation and
however far the barrel has recoiled.
"""

import math

import numpy as np
from OpenGL.GL import *

import utils
from lighting import BRASS, DARK_IRON, IRON, WOOD, WOOD_LIGHT

# --------------------------------------------------------------------------
# dimensions (metres)
# --------------------------------------------------------------------------

WHEEL_RADIUS = 0.62
WHEEL_WIDTH = 0.16
WHEEL_TRACK = 0.68          # |z| of each wheel
SPOKE_COUNT = 10

BEAM_FRONT = (0.70, 0.85)   # (x, y) of the front end of a trail beam
BEAM_REAR = (-2.35, 0.17)   # (x, y) of the rear end (rests on the ground)
BEAM_THICK = 0.20
BEAM_WIDE = 0.15
BEAM_Z = 0.32               # |z| of each beam

PIVOT = (0.10, 1.15, 0.0)   # trunnion pivot, in carriage space

BARREL_BREECH = -0.45       # x of the rearmost point of the barrel
MUZZLE_X = 1.70             # x of the muzzle face, in barrel space
BORE_RADIUS = 0.075

# --------------------------------------------------------------------------
# motion constants
# --------------------------------------------------------------------------

DRIVE_SPEED = 2.0           # m/s while an arrow key is held
ELEV_SPEED = 26.0           # deg/s
ELEV_MIN, ELEV_MAX = 0.0, 45.0
X_MIN, X_MAX = -14.0, 14.0

RECOIL_MAX = 0.42           # m
RECOIL_IMPULSE = 3.4        # m/s given to the barrel at the shot
RECOIL_K = 90.0             # spring constant   [1/s^2]
RECOIL_C = 2.0 * math.sqrt(RECOIL_K) * 0.85   # slightly under-damped
MUZZLE_SPEED = 15.0         # m/s of the cannonball
FIRE_COOLDOWN = 0.45        # s


# --------------------------------------------------------------------------
# small matrix helpers (mirror of the GL matrix stack)
# --------------------------------------------------------------------------

def translation(x, y, z):
    m = np.eye(4)
    m[0, 3], m[1, 3], m[2, 3] = x, y, z
    return m


def rotation_z(deg):
    c, s = math.cos(math.radians(deg)), math.sin(math.radians(deg))
    m = np.eye(4)
    m[0, 0], m[0, 1] = c, -s
    m[1, 0], m[1, 1] = s, c
    return m


# --------------------------------------------------------------------------


class Cannon:
    """Holds the animation state and draws the whole assembly."""

    def __init__(self):
        self.x = 0.0
        self.wheel_angle = 0.0      # degrees, about +Z
        self.elevation = 20.0       # degrees
        self.recoil = 0.0           # metres the barrel has slid back
        self.recoil_vel = 0.0
        self.cooldown = 0.0
        self.flash = 0.0            # muzzle-flash timer
        self.distance = 0.0         # total signed distance travelled
        self.barrel_material = IRON  # swapped by the 'M' key to show materials

    # -- state -----------------------------------------------------------
    def reset(self):
        self.__init__()

    def drive(self, ds):
        """Translate the carriage by `ds` and roll the wheels to match.

        Rolling without slipping: an arc `R * dphi` of the rim must equal the
        distance `ds` travelled, and the wheel turns *clockwise* seen from +Z
        when moving in +X, hence the minus sign.
        """
        new_x = utils.clamp(self.x + ds, X_MIN, X_MAX)
        moved = new_x - self.x
        self.x = new_x
        self.distance += moved
        self.wheel_angle -= math.degrees(moved / WHEEL_RADIUS)

    def elevate(self, dtheta):
        self.elevation = utils.clamp(self.elevation + dtheta, ELEV_MIN, ELEV_MAX)

    def can_fire(self):
        return self.cooldown <= 0.0

    def fire(self):
        """Kick the barrel backwards; the spring in `update` brings it home."""
        if not self.can_fire():
            return False
        self.recoil_vel += RECOIL_IMPULSE
        self.cooldown = FIRE_COOLDOWN
        self.flash = 0.12
        return True

    def update(self, dt):
        self.cooldown = max(0.0, self.cooldown - dt)
        self.flash = max(0.0, self.flash - dt)

        # damped harmonic oscillator, semi-implicit (symplectic) Euler:
        #   a  = -k rho - c rho'          rho'' + c rho' + k rho = 0
        #   v += a dt ;  rho += v dt
        a = -RECOIL_K * self.recoil - RECOIL_C * self.recoil_vel
        self.recoil_vel += a * dt
        self.recoil += self.recoil_vel * dt
        if self.recoil < 0.0:                  # never push past the rest pose
            self.recoil = 0.0
            self.recoil_vel = max(0.0, self.recoil_vel)
        elif self.recoil > RECOIL_MAX:
            self.recoil = RECOIL_MAX
            self.recoil_vel = min(0.0, self.recoil_vel)

    # -- kinematics -------------------------------------------------------
    def barrel_matrix(self):
        """The full composite matrix applied to barrel-space vertices."""
        return (translation(self.x, 0.0, 0.0)
                @ translation(*PIVOT)
                @ rotation_z(self.elevation)
                @ translation(-self.recoil, 0.0, 0.0))

    def muzzle_state(self):
        """(position, unit direction) of the muzzle in world space."""
        m = self.barrel_matrix()
        p = m @ np.array([MUZZLE_X, 0.0, 0.0, 1.0])
        d = m @ np.array([1.0, 0.0, 0.0, 0.0])       # w=0 -> direction only
        d = d[:3] / np.linalg.norm(d[:3])
        return p[:3], d

    # -- drawing ----------------------------------------------------------
    def draw(self):
        glPushMatrix()
        glTranslatef(self.x, 0.0, 0.0)          # (1) translation of the root

        self._draw_carriage()

        for z in (-WHEEL_TRACK, WHEEL_TRACK):   # (2) rolling wheels
            glPushMatrix()
            glTranslatef(0.0, WHEEL_RADIUS, z)
            glRotatef(self.wheel_angle, 0.0, 0.0, 1.0)
            self._draw_wheel()
            glPopMatrix()

        glPushMatrix()                          # (3) barrel mount = pivot
        glTranslatef(*PIVOT)
        glRotatef(self.elevation, 0.0, 0.0, 1.0)    # (4) elevation
        self._draw_trunnions()
        glTranslatef(-self.recoil, 0.0, 0.0)        # (5) recoil along own axis
        self._draw_barrel()
        glPopMatrix()

        glPopMatrix()

    # ....................................................................
    def _draw_carriage(self):
        WOOD.apply()

        # -- two trail beams, tilted so the rear rests on the ground -------
        dx = BEAM_REAR[0] - BEAM_FRONT[0]
        dy = BEAM_REAR[1] - BEAM_FRONT[1]
        length = math.hypot(dx, dy)
        angle = math.degrees(math.atan2(-dy, -dx))       # front-facing angle
        cx = 0.5 * (BEAM_FRONT[0] + BEAM_REAR[0])
        cy = 0.5 * (BEAM_FRONT[1] + BEAM_REAR[1])
        for z in (-BEAM_Z, BEAM_Z):
            glPushMatrix()
            glTranslatef(cx, cy, z)
            glRotatef(angle, 0.0, 0.0, 1.0)
            utils.box(length, BEAM_THICK, BEAM_WIDE)
            glPopMatrix()

        # -- transoms (cross members between the beams) --------------------
        for t in (0.12, 0.55, 0.9):
            px = utils.lerp(BEAM_FRONT[0], BEAM_REAR[0], t)
            py = utils.lerp(BEAM_FRONT[1], BEAM_REAR[1], t)
            glPushMatrix()
            glTranslatef(px, py, 0.0)
            utils.box(0.16, 0.14, 2.0 * BEAM_Z)
            glPopMatrix()

        # -- trail spade at the rear ---------------------------------------
        glPushMatrix()
        glTranslatef(BEAM_REAR[0] - 0.06, BEAM_REAR[1] - 0.02, 0.0)
        DARK_IRON.apply()
        utils.box(0.24, 0.30, 2.0 * BEAM_Z + 0.16)
        glPopMatrix()

        # -- cheeks: uprights carrying the trunnions -----------------------
        WOOD.apply()
        beam_y_at = lambda x: utils.lerp(  # noqa: E731 - tiny local helper
            BEAM_FRONT[1], BEAM_REAR[1],
            (x - BEAM_FRONT[0]) / (BEAM_REAR[0] - BEAM_FRONT[0]))
        base_y = beam_y_at(PIVOT[0])
        h = PIVOT[1] - base_y + 0.18
        for z in (-BEAM_Z, BEAM_Z):
            glPushMatrix()
            glTranslatef(PIVOT[0], base_y + h * 0.5 - 0.02, z)
            utils.box(0.40, h, BEAM_WIDE)
            glPopMatrix()
        # a stepped quoin block under the breech, as on a real carriage
        glPushMatrix()
        glTranslatef(PIVOT[0] - 0.62, base_y + 0.22, 0.0)
        utils.box(0.42, 0.34, 2.0 * BEAM_Z)
        glPopMatrix()

        # -- axle ----------------------------------------------------------
        DARK_IRON.apply()
        glPushMatrix()
        glTranslatef(0.0, WHEEL_RADIUS, -(WHEEL_TRACK + 0.10))
        utils.cylinder(0.065, 0.065, 2.0 * (WHEEL_TRACK + 0.10), 20)
        glPopMatrix()

        # axle bolster tying the axle to the beams
        WOOD.apply()
        glPushMatrix()
        glTranslatef(0.0, WHEEL_RADIUS + 0.10, 0.0)
        utils.box(0.26, 0.22, 2.0 * WHEEL_TRACK - 0.10)
        glPopMatrix()

    # ....................................................................
    def _draw_wheel(self):
        """Wheel in its own frame: disc lies in XY, axle along Z."""
        glPushMatrix()
        glTranslatef(0.0, 0.0, -WHEEL_WIDTH * 0.5)

        DARK_IRON.apply()                                   # iron tyre
        utils.tube(0.555, WHEEL_RADIUS, WHEEL_WIDTH, 40)

        WOOD_LIGHT.apply()                                  # wooden felloe
        utils.tube(0.44, 0.555, WHEEL_WIDTH * 0.86, 40)

        glPopMatrix()

        WOOD_LIGHT.apply()                                  # spokes
        for i in range(SPOKE_COUNT):
            glPushMatrix()
            glRotatef(360.0 * i / SPOKE_COUNT, 0.0, 0.0, 1.0)
            glTranslatef(0.5 * (0.12 + 0.46), 0.0, 0.0)
            utils.box(0.46 - 0.12, 0.075, 0.075)
            glPopMatrix()

        glPushMatrix()                                      # hub
        glTranslatef(0.0, 0.0, -0.16)
        utils.cylinder(0.135, 0.135, 0.32, 24)
        BRASS.apply()
        glTranslatef(0.0, 0.0, 0.32)
        utils.cylinder(0.09, 0.05, 0.07, 16)
        glPopMatrix()
        glPushMatrix()
        glTranslatef(0.0, 0.0, -0.16)
        glRotatef(180.0, 0.0, 1.0, 0.0)
        utils.cylinder(0.09, 0.05, 0.07, 16)
        glPopMatrix()

    # ....................................................................
    def _draw_trunnions(self):
        """The two stubs the barrel pivots on -- they stay with the mount, so
        they are drawn *before* the recoil translation."""
        BRASS.apply()
        for zdir in (-1.0, 1.0):
            glPushMatrix()
            glTranslatef(0.0, 0.0, zdir * 0.16)
            if zdir < 0.0:
                glRotatef(180.0, 0.0, 1.0, 0.0)
            utils.cylinder(0.09, 0.085, 0.24, 20)
            glPopMatrix()

    # ....................................................................
    def _draw_barrel(self):
        self.barrel_material.apply()

        glPushMatrix()
        glTranslatef(BARREL_BREECH, 0.0, 0.0)

        utils.sphere(0.205, 28, 20)                       # breech / base ring
        glPushMatrix()                                    # cascabel knob
        glTranslatef(-0.16, 0.0, 0.0)
        utils.sphere(0.085, 16, 12)
        glTranslatef(-0.02, 0.0, 0.0)
        utils.cylinder_along_x(0.045, 0.045, 0.14, 14)
        glPopMatrix()

        # first reinforce -> chase -> muzzle swell
        utils.cylinder_along_x(0.200, 0.185, 0.55, 40)
        glPushMatrix()
        glTranslatef(0.55, 0.0, 0.0)
        utils.cylinder_along_x(0.175, 0.125, 1.42, 40)
        glPopMatrix()
        glPushMatrix()
        glTranslatef(1.97, 0.0, 0.0)
        utils.cylinder_along_x(0.125, 0.165, 0.11, 40)     # swell
        glTranslatef(0.11, 0.0, 0.0)
        utils.cylinder_along_x(0.165, 0.150, 0.07, 40)     # muzzle face
        glPopMatrix()
        glPopMatrix()

        # brass astragal rings around the tube
        BRASS.apply()
        for x, r in ((0.10, 0.190), (0.72, 0.155)):
            glPushMatrix()
            glTranslatef(x, 0.0, 0.0)
            glRotatef(90.0, 0.0, 1.0, 0.0)
            utils.torus(r, 0.022, 32, 12)
            glPopMatrix()

        # vent field / touch hole on top of the breech
        DARK_IRON.apply()
        glPushMatrix()
        glTranslatef(-0.30, 0.16, 0.0)
        utils.cylinder_along_y(0.035, 0.028, 0.07, 12)
        glPopMatrix()

        # the bore: a dark tube sunk into the muzzle face
        glPushMatrix()
        glTranslatef(MUZZLE_X - 0.001, 0.0, 0.0)
        glRotatef(180.0, 0.0, 1.0, 0.0)
        glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT, (0.01, 0.01, 0.01, 1.0))
        glMaterialfv(GL_FRONT_AND_BACK, GL_DIFFUSE, (0.03, 0.03, 0.03, 1.0))
        glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR, (0.10, 0.10, 0.10, 1.0))
        glMaterialf(GL_FRONT_AND_BACK, GL_SHININESS, 16.0)
        utils.cylinder(BORE_RADIUS, BORE_RADIUS, 0.30, 20)
        glPopMatrix()

        if self.flash > 0.0:
            self._draw_flash()

    def _draw_flash(self):
        k = self.flash / 0.12
        glDisable(GL_CULL_FACE)
        glMaterialfv(GL_FRONT_AND_BACK, GL_EMISSION, (1.0 * k, 0.82 * k, 0.35 * k, 1.0))
        glMaterialfv(GL_FRONT_AND_BACK, GL_DIFFUSE, (0.0, 0.0, 0.0, 1.0))
        glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR, (0.0, 0.0, 0.0, 1.0))
        glPushMatrix()
        glTranslatef(MUZZLE_X, 0.0, 0.0)
        utils.cylinder_along_x(0.30 * k, 0.04, 1.05 * k, 20, 1, False)
        utils.sphere(0.26 * k, 16, 12)
        glPushMatrix()
        glTranslatef(0.34 * k, 0.0, 0.0)
        utils.sphere(0.20 * k, 14, 10)
        glPopMatrix()
        glPopMatrix()
        glMaterialfv(GL_FRONT_AND_BACK, GL_EMISSION, (0.0, 0.0, 0.0, 1.0))
        glEnable(GL_CULL_FACE)

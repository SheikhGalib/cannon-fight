"""
projectile.py
=============

Cannonball flight.

Each ball is a point mass under constant gravity, integrated with
**semi-implicit (symplectic) Euler**::

    v <- v + g dt
    p <- p + v dt

which, unlike explicit Euler, does not gain energy over time and reproduces the
analytic parabola closely for the time-steps used here.  The analytic solution
is kept in `predict_range()` so the HUD can show the textbook prediction next
to what the simulation actually does.
"""

import math

import numpy as np
from OpenGL.GL import *

import utils
from lighting import BALL, unlit

GRAVITY = np.array([0.0, -9.81, 0.0])
BALL_RADIUS = 0.075
RESTITUTION = 0.35          # energy kept across a ground bounce
FRICTION = 0.72             # tangential damping at a bounce
MAX_TRAIL = 900
MAX_BALLS = 12
LIFETIME = 14.0


class Cannonball:
    def __init__(self, position, velocity):
        self.p = np.array(position, dtype=float)
        self.v = np.array(velocity, dtype=float)
        self.trail = [self.p.copy()]
        self.age = 0.0
        self.alive = True
        self.at_rest = False

    def update(self, dt):
        if not self.alive:
            return
        self.age += dt
        if self.age > LIFETIME:
            self.alive = False
            return
        if self.at_rest:
            return

        self.v = self.v + GRAVITY * dt          # semi-implicit Euler
        self.p = self.p + self.v * dt

        if self.p[1] < BALL_RADIUS:             # ground contact
            self.p[1] = BALL_RADIUS
            if abs(self.v[1]) < 0.8:
                self.v[:] = 0.0
                self.at_rest = True
            else:
                self.v[1] = -self.v[1] * RESTITUTION
                self.v[0] *= FRICTION
                self.v[2] *= FRICTION

        if len(self.trail) < MAX_TRAIL:
            if np.linalg.norm(self.p - self.trail[-1]) > 0.05:
                self.trail.append(self.p.copy())

    def draw(self, draw_trail=True):
        if draw_trail and len(self.trail) > 1:
            # the trail is flat-shaded: step out of the Phong shader so the
            # per-vertex glColor values are the ones that reach the framebuffer
            with unlit():
                glLineWidth(3.0)
                glBegin(GL_LINE_STRIP)
                n = len(self.trail)
                for i, q in enumerate(self.trail):
                    a = 0.30 + 0.70 * (i / max(n - 1, 1))
                    glColor3f(1.0 * a, 0.80 * a, 0.35 * a)
                    glVertex3f(*q)
                glEnd()
                glLineWidth(1.0)

        BALL.apply()
        glPushMatrix()
        glTranslatef(*self.p)
        utils.sphere(BALL_RADIUS, 20, 14)
        glPopMatrix()


class ProjectileSystem:
    def __init__(self):
        self.balls = []
        self.show_trails = True

    def clear(self):
        self.balls.clear()

    def spawn(self, muzzle_pos, direction, speed):
        p = np.asarray(muzzle_pos, dtype=float) + np.asarray(direction) * (BALL_RADIUS + 0.02)
        self.balls.append(Cannonball(p, np.asarray(direction, dtype=float) * speed))
        if len(self.balls) > MAX_BALLS:
            self.balls.pop(0)

    def update(self, dt):
        for b in self.balls:
            b.update(dt)
        self.balls = [b for b in self.balls if b.alive]

    def draw(self):
        for b in self.balls:
            b.draw(self.show_trails)

    # -- analytic reference ----------------------------------------------
    @staticmethod
    def predict(muzzle_pos, elevation_deg, speed):
        """Closed-form flight time and range for a shot fired from height h.

        y(t) = h + v sin(a) t - g t^2 / 2 = 0  =>
            t = ( v sin a + sqrt( (v sin a)^2 + 2 g h ) ) / g
            R = v cos(a) * t
        """
        g = 9.81
        h = float(muzzle_pos[1])
        a = math.radians(elevation_deg)
        vy = speed * math.sin(a)
        vx = speed * math.cos(a)
        disc = vy * vy + 2.0 * g * max(h, 0.0)
        t = (vy + math.sqrt(max(disc, 0.0))) / g
        return t, vx * t

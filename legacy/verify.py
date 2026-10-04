"""
verify.py
=========

Numerical self-checks for the claims made in the report.  No OpenGL context is
needed -- everything here exercises the same maths the renderer uses.

    python verify.py
"""

import math

import numpy as np

import cannon as C
from cannon import Cannon
from projectile import BALL_RADIUS, GRAVITY, Cannonball, ProjectileSystem

LINE = '-' * 72


def head(title):
    print('\n' + LINE + '\n' + title + '\n' + LINE)


# --------------------------------------------------------------------------
def check_rolling():
    head('1.  Rolling constraint    phi = -s / R')
    cn = Cannon()
    step, n = 0.01, 260
    for _ in range(n):
        cn.drive(step)
    s = step * n
    expected = -math.degrees(s / C.WHEEL_RADIUS)
    print('  wheel radius R          : %.4f m' % C.WHEEL_RADIUS)
    print('  distance driven s       : %.4f m' % s)
    print('  expected  -s/R          : %+10.4f deg' % expected)
    print('  simulated wheel_angle   : %+10.4f deg' % cn.wheel_angle)
    print('  error                   : %.3e deg' % abs(expected - cn.wheel_angle))
    arc = C.WHEEL_RADIUS * math.radians(abs(cn.wheel_angle))
    print('  rim arc length R|phi|   : %.4f m  (must equal s)' % arc)


# --------------------------------------------------------------------------
def check_muzzle():
    head('2.  Muzzle point from the composite matrix')
    for elev, recoil in ((0.0, 0.0), (32.0, 0.0), (45.0, 0.30)):
        cn = Cannon()
        cn.x, cn.elevation, cn.recoil = 1.5, elev, recoil
        p, d = cn.muzzle_state()
        th = math.radians(elev)
        # closed form:  p = x + p_pivot + Rz(theta) (MUZZLE_X - rho, 0, 0)
        L = C.MUZZLE_X - recoil
        px = cn.x + C.PIVOT[0] + L * math.cos(th)
        py = C.PIVOT[1] + L * math.sin(th)
        print('  elev %4.1f deg, recoil %.2f m' % (elev, recoil))
        print('    matrix chain   : (%8.4f, %8.4f, %6.3f)' % (p[0], p[1], p[2]))
        print('    closed form    : (%8.4f, %8.4f, %6.3f)' % (px, py, 0.0))
        print('    |difference|   : %.3e m' % np.linalg.norm(p - np.array([px, py, 0.0])))
        print('    axis direction : (%.4f, %.4f, %.4f)   |d| = %.6f'
              % (d[0], d[1], d[2], np.linalg.norm(d)))


# --------------------------------------------------------------------------
def check_projectile():
    head('3.  Projectile: semi-implicit Euler vs. the closed-form parabola')
    g = -GRAVITY[1]
    print('  muzzle speed v0         : %.2f m/s' % C.MUZZLE_SPEED)
    print('  gravity g               : %.2f m/s^2' % g)
    print()
    print('  %5s %10s %10s %10s %10s %9s' %
          ('elev', 't_exact', 't_sim', 'R_exact', 'R_sim', 'rel.err'))
    for elev in (10.0, 20.0, 30.0, 35.0, 45.0):
        cn = Cannon()
        cn.elevation = elev
        p0, d0 = cn.muzzle_state()
        p0 = p0 + d0 * (BALL_RADIUS + 0.02)
        v0 = d0 * C.MUZZLE_SPEED

        # closed form, solved for the height at which the sim stops (y = r)
        h = p0[1] - BALL_RADIUS
        vy, vx = v0[1], v0[0]
        t_exact = (vy + math.sqrt(vy * vy + 2.0 * g * h)) / g
        r_exact = vx * t_exact

        ball = Cannonball(p0, v0)
        dt, t = 1.0 / 240.0, 0.0
        while not ball.at_rest and t < 10.0:
            ball.update(dt)
            t += dt
            if ball.at_rest or ball.v[1] > 0 and t > 0.1 and ball.p[1] <= BALL_RADIUS:
                break
            if ball.p[1] <= BALL_RADIUS and t > 0.05:
                break
        r_sim = ball.p[0] - p0[0]
        err = abs(r_sim - r_exact) / max(r_exact, 1e-9)
        print('  %5.1f %10.4f %10.4f %10.4f %10.4f %8.3f %%'
              % (elev, t_exact, t, r_exact, r_sim, 100.0 * err))

    print()
    print('  HUD prediction helper (fired from the muzzle height, y = 0 target):')
    cn = Cannon()
    cn.elevation = 32.0
    p, _ = cn.muzzle_state()
    t, r = ProjectileSystem.predict(p, 32.0, C.MUZZLE_SPEED)
    print('    elev 32 deg -> flight %.3f s, range %.3f m  (muzzle height %.3f m)'
          % (t, r, p[1]))


# --------------------------------------------------------------------------
def check_recoil():
    head('4.  Recoil: damped harmonic oscillator')
    k, c = C.RECOIL_K, C.RECOIL_C
    zeta = c / (2.0 * math.sqrt(k))
    wn = math.sqrt(k)
    wd = wn * math.sqrt(max(1.0 - zeta * zeta, 0.0))
    print('  k = %.2f 1/s^2   c = %.4f 1/s' % (k, c))
    print('  natural freq  w_n = sqrt(k)          : %.4f rad/s (%.3f Hz)' % (wn, wn / (2 * math.pi)))
    print('  damping ratio zeta = c / 2 sqrt(k)   : %.4f  (%s)'
          % (zeta, 'under-damped' if zeta < 1 else 'over/critically damped'))
    print('  damped freq   w_d = w_n sqrt(1-z^2)  : %.4f rad/s' % wd)
    print('  impulse v0                           : %.2f m/s' % C.RECOIL_IMPULSE)
    # x(t) = (v0/w_d) e^{-z w_n t} sin(w_d t);  x'(t)=0 at tan(w_d t) = w_d/(z w_n)
    t_p = math.atan2(wd, zeta * wn) / wd
    x_p = (C.RECOIL_IMPULSE / wd) * math.exp(-zeta * wn * t_p) * math.sin(wd * t_p)
    print('  analytic peak time  t_p              : %.4f s' % t_p)
    print('  analytic peak       x(t_p)           : %.4f m' % x_p)

    for steps in (240, 480, 4000):
        cn = Cannon()
        cn.fire()
        dt, t, peak, t_peak, settle = 1.0 / steps, 0.0, 0.0, 0.0, None
        while t < 3.0:
            cn.update(dt)
            t += dt
            if cn.recoil > peak:
                peak, t_peak = cn.recoil, t
            if settle is None and t > t_peak and abs(cn.recoil) < 0.02 * max(peak, 1e-9):
                settle = t
        print('  dt = 1/%-5d  peak %.4f m at t = %.3f s, |err| vs analytic %.2f %%, '
              'settles (2%%) at t = %.3f s'
              % (steps, peak, t_peak, 100.0 * abs(peak - x_p) / x_p,
                 settle if settle else float('nan')))
    print('  clamp RECOIL_MAX                     : %.4f m (never reached here)' % C.RECOIL_MAX)


# --------------------------------------------------------------------------
def check_normals():
    head('5.  Analytic normals of the truncated cone are correct')
    r0, r1, h = 0.175, 0.125, 1.42        # the barrel chase
    dr = r1 - r0
    worst_unit = 0.0
    worst_perp = 0.0
    for i in range(64):
        phi = 2.0 * math.pi * i / 64.0
        for t in (0.0, 0.37, 1.0):
            c, s = math.cos(phi), math.sin(phi)
            r = r0 + dr * t
            n = np.array([c * h, s * h, r0 - r1])
            n = n / np.linalg.norm(n)
            worst_unit = max(worst_unit, abs(np.linalg.norm(n) - 1.0))
            # the two surface tangents must both be orthogonal to n
            t_phi = np.array([-r * s, r * c, 0.0])
            t_t = np.array([dr * c, dr * s, h])
            worst_perp = max(worst_perp, abs(n @ t_phi), abs(n @ t_t))
    print('  surface P(phi,t) = ((r0+(r1-r0)t)cos phi, ... , h t),  r0=%.3f r1=%.3f h=%.3f'
          % (r0, r1, h))
    print('  N = normalize(h cos phi, h sin phi, r0-r1)')
    print('  max | |N| - 1 |          : %.3e' % worst_unit)
    print('  max |N . tangent|        : %.3e   (0 => N is the true surface normal)' % worst_perp)


# --------------------------------------------------------------------------
def check_hierarchy():
    head('6.  Hierarchy: a child inherits every ancestor transform')
    cn = Cannon()
    cn.elevation = 25.0
    p_a, _ = cn.muzzle_state()
    cn.x = 4.0                       # move only the root
    p_b, _ = cn.muzzle_state()
    print('  muzzle at x=0.0 : (%.4f, %.4f, %.3f)' % tuple(p_a))
    print('  muzzle at x=4.0 : (%.4f, %.4f, %.3f)' % tuple(p_b))
    print('  delta           : (%.4f, %.4f, %.3f)   <- pure translation of the root'
          % tuple(p_b - p_a))
    cn.x = 0.0
    cn.recoil = 0.30
    p_c, _ = cn.muzzle_state()
    d = p_c - p_a
    th = math.radians(25.0)
    print('  recoil 0.30 m   : delta = (%.4f, %.4f, %.3f)' % tuple(d))
    print('  expected -rho*(cos25, sin25, 0) = (%.4f, %.4f, %.3f)'
          % (-0.30 * math.cos(th), -0.30 * math.sin(th), 0.0))
    print('  |error|         : %.3e m'
          % np.linalg.norm(d - np.array([-0.30 * math.cos(th), -0.30 * math.sin(th), 0.0])))


if __name__ == '__main__':
    print('Medieval Field Cannon -- numerical verification')
    check_rolling()
    check_muzzle()
    check_projectile()
    check_recoil()
    check_normals()
    check_hierarchy()
    print('\n' + LINE)
    print('All checks completed.')

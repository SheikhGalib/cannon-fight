"""
main.py
=======

Medieval Field Cannon -- an OpenGL / PyOpenGL scene demonstrating hierarchical
transformations and the Phong reflection model.

Run interactively::

    python main.py

Render the figures used by the report (writes PNGs and exits)::

    python main.py --shots docs/images

Controls
--------
    Left / Right      drive the carriage backward / forward (wheels roll)
    Up / Down         elevate / depress the barrel (0 deg .. 45 deg)
    Space             fire (muzzle flash, recoil, cannonball)
    C                 cycle camera preset          mouse drag  orbit
    + / -             zoom                          wheel      zoom
    L                 light on / off                K          light orbit on/off
    N                 point light <-> directional   [ / ]      nudge light angle
    F                 per-fragment Phong <-> per-vertex Gouraud
    B                 Phong <-> Blinn-Phong specular
    M                 cycle barrel material (iron / brass / wood)
    T                 projectile trails on/off      X          clear projectiles
    W                 wireframe                     G          ground on/off
    H                 help overlay                  R          reset
    P                 save a screenshot             Esc / Q    quit
"""

import math
import os
import sys
import time

import numpy as np
from OpenGL.GL import *
from OpenGL.GLU import *
from OpenGL.GLUT import *

import utils
from cannon import (Cannon, DRIVE_SPEED, ELEV_SPEED, MUZZLE_SPEED, WHEEL_RADIUS)
from lighting import (BRASS, FOG_COLOR, GROUND, IRON, WOOD, Light, PhongShader,
                      setup_fog, unlit)
from projectile import ProjectileSystem

WINDOW_W, WINDOW_H = 1280, 720
TITLE = b'Medieval Field Cannon - OpenGL (hierarchical transforms + Phong lighting)'

SKY_TOP = (0.26, 0.43, 0.68)
SKY_MID = (0.55, 0.66, 0.80)
SKY_HORIZON = FOG_COLOR          # matches the fog so the horizon has no seam
GROUND_SIZE = 100.0
GROUND_DIVISIONS = 150

CAMERA_PRESETS = [
    # (name,             yaw, pitch, dist, height)
    ('three-quarter',    38.0, 17.0, 9.0, 1.05),
    ('side profile',     90.0,  9.0, 8.2, 1.05),
    ('low hero',         18.0,  5.0, 6.0, 0.95),
    ('high overview',    58.0, 42.0, 13.5, 1.20),
    ('down-range',      148.0, 12.0, 6.5, 1.10),
]

BARREL_MATERIALS = [IRON, BRASS, WOOD]


class App:
    def __init__(self):
        self.cannon = Cannon()
        self.projectiles = ProjectileSystem()
        self.light = Light()
        self.shader = PhongShader()

        self.width, self.height = WINDOW_W, WINDOW_H
        self.keys = set()
        self.special = set()

        self.cam_index = 0
        self.yaw, self.pitch, self.dist, self.cam_h = CAMERA_PRESETS[0][1:]
        self.cam_offset = 0.0        # shifts the look-at point down-range
        self.dragging = False
        self.last_mouse = (0, 0)

        self.per_fragment = True
        self.blinn = False
        self.wireframe = False
        self.show_ground = True
        self.show_grid = True
        self.show_hud = True
        self.show_help = True
        self.barrel_mat = 0

        self.last_time = None
        self.fps = 0.0
        self._fps_acc = 0.0
        self._fps_n = 0
        self.shot_dir = None
        self.status = ''
        self.status_t = 0.0

    # ------------------------------------------------------------------ GL
    def init_gl(self):
        glClearColor(0.5, 0.6, 0.7, 1.0)
        glEnable(GL_DEPTH_TEST)
        glDepthFunc(GL_LEQUAL)
        glShadeModel(GL_SMOOTH)
        glEnable(GL_NORMALIZE)
        glDisable(GL_CULL_FACE)
        glEnable(GL_LIGHTING)
        glHint(GL_PERSPECTIVE_CORRECTION_HINT, GL_NICEST)
        try:
            glEnable(GL_MULTISAMPLE)
        except Exception:
            pass
        self.light.setup()
        setup_fog()
        if not self.shader.compile():
            self.per_fragment = False
            print('[lighting] GLSL unavailable, using fixed-function pipeline')
            if self.shader.log:
                print(self.shader.log)
        else:
            print('[lighting] per-fragment Phong shader compiled')
        print('[gl] %s | %s' % (glGetString(GL_RENDERER).decode(),
                                glGetString(GL_VERSION).decode()))

    # -------------------------------------------------------------- camera
    def camera_target(self):
        return np.array([self.cannon.x + self.cam_offset, self.cam_h, 0.0])

    def camera_eye(self):
        t = self.camera_target()
        a, b = math.radians(self.yaw), math.radians(self.pitch)
        return t + self.dist * np.array([math.cos(b) * math.cos(a),
                                         math.sin(b),
                                         math.cos(b) * math.sin(a)])

    def apply_camera(self):
        glMatrixMode(GL_PROJECTION)
        glLoadIdentity()
        gluPerspective(50.0, self.width / max(self.height, 1), 0.1, 200.0)
        glMatrixMode(GL_MODELVIEW)
        glLoadIdentity()
        e, t = self.camera_eye(), self.camera_target()
        gluLookAt(e[0], e[1], e[2], t[0], t[1], t[2], 0.0, 1.0, 0.0)

    def set_preset(self, i):
        self.cam_index = i % len(CAMERA_PRESETS)
        name, self.yaw, self.pitch, self.dist, self.cam_h = CAMERA_PRESETS[self.cam_index]
        self.cam_offset = 0.0
        self.notify('camera: ' + name)

    # ------------------------------------------------------------ simulate
    def update(self, dt):
        if GLUT_KEY_RIGHT in self.special:
            self.cannon.drive(+DRIVE_SPEED * dt)
        if GLUT_KEY_LEFT in self.special:
            self.cannon.drive(-DRIVE_SPEED * dt)
        if GLUT_KEY_UP in self.special:
            self.cannon.elevate(+ELEV_SPEED * dt)
        if GLUT_KEY_DOWN in self.special:
            self.cannon.elevate(-ELEV_SPEED * dt)

        self.cannon.update(dt)
        self.projectiles.update(dt)
        self.light.update(dt)
        self.status_t = max(0.0, self.status_t - dt)

    def fire(self):
        if self.cannon.fire():
            pos, direction = self.cannon.muzzle_state()
            self.projectiles.spawn(pos, direction, MUZZLE_SPEED)
            t, r = ProjectileSystem.predict(pos, self.cannon.elevation, MUZZLE_SPEED)
            self.notify('fire!  predicted range %.2f m, flight %.2f s' % (r, t))

    def notify(self, msg):
        self.status = msg
        self.status_t = 2.5

    # --------------------------------------------------------------- draw
    def render(self):
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT)
        self.draw_sky()

        self.apply_camera()
        self.light.upload()          # after the view matrix -> correct eye space

        if self.wireframe:
            glPolygonMode(GL_FRONT_AND_BACK, GL_LINE)
        else:
            glPolygonMode(GL_FRONT_AND_BACK, GL_FILL)

        shading = self.per_fragment and self.shader.available
        if shading:
            self.shader.bind(self.light.enabled, self.blinn)
        else:
            self.shader.unbind()
            glEnable(GL_LIGHTING)

        if self.show_ground:
            GROUND.apply()
            utils.grid_plane(GROUND_SIZE, GROUND_DIVISIONS)
            if self.show_grid:
                self.draw_ground_grid()

        self.cannon.barrel_material = BARREL_MATERIALS[self.barrel_mat]
        self.cannon.draw()
        self.projectiles.draw()
        self.draw_light_marker()

        self.shader.unbind()
        glPolygonMode(GL_FRONT_AND_BACK, GL_FILL)

        if self.show_hud:
            self.draw_hud()

    def draw_ground_grid(self):
        """A 1 m reference grid on the field.  It is not decoration: without a
        fixed reference the translation of the carriage is invisible in a still
        image, because the camera tracks the cannon."""
        n, step = 22, 1.0
        with unlit():
            glBegin(GL_LINES)
            for i in range(-n, n + 1):
                major = (i % 5 == 0)
                glColor3f(*((0.30, 0.40, 0.26) if major else (0.24, 0.32, 0.20)))
                v = i * step
                glVertex3f(v, 0.006, -n * step)
                glVertex3f(v, 0.006, n * step)
                glVertex3f(-n * step, 0.006, v)
                glVertex3f(n * step, 0.006, v)
            glEnd()

    def draw_sky(self):
        """Three-stop vertical gradient.  The lowest stop is the fog colour, so
        the ground plane dissolves into the sky instead of ending in a seam."""
        w, h = self.width, self.height
        stops = ((0.00, SKY_HORIZON), (0.42, SKY_HORIZON),
                 (0.62, SKY_MID), (1.00, SKY_TOP))
        with unlit(fog=False):
            glDisable(GL_DEPTH_TEST)
            glDepthMask(GL_FALSE)
            utils.begin_2d(w, h)
            glBegin(GL_QUAD_STRIP)
            for t, c in stops:
                glColor3f(*c)
                glVertex2f(0, t * h)
                glVertex2f(w, t * h)
            glEnd()
            utils.end_2d()
            glDepthMask(GL_TRUE)
            glEnable(GL_DEPTH_TEST)

    def draw_light_marker(self):
        """A small emissive ball where the point light sits, plus a stem to the
        ground so its position is readable in a still image."""
        x, y, z = self.light.position
        col = (1.0, 0.94, 0.72) if self.light.enabled else (0.35, 0.35, 0.38)
        with unlit():
            glColor3f(*col)
            glPushMatrix()
            glTranslatef(x, y, z)
            utils.sphere(0.16, 16, 12)
            glPopMatrix()
            glLineWidth(1.0)
            glColor3f(col[0] * 0.6, col[1] * 0.6, col[2] * 0.6)
            glBegin(GL_LINES)
            glVertex3f(x, y, z)
            glVertex3f(x, 0.0, z)
            glEnd()

    # ---------------------------------------------------------------- HUD
    def draw_hud(self):
        self.shader.unbind()
        glDisable(GL_LIGHTING)
        glDisable(GL_FOG)
        glDisable(GL_DEPTH_TEST)
        glEnable(GL_BLEND)
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA)
        utils.begin_2d(self.width, self.height)

        c = self.cannon
        pos, _ = c.muzzle_state()
        t_fly, rng = ProjectileSystem.predict(pos, c.elevation, MUZZLE_SPEED)
        mode = 'per-fragment Phong' if (self.per_fragment and self.shader.available) \
            else 'per-vertex Gouraud'
        spec = 'Blinn-Phong' if self.blinn else 'Phong'

        left = [
            'MEDIEVAL FIELD CANNON',
            '',
            'carriage x      %+7.2f m   (travelled %+.2f m)' % (c.x, c.distance),
            'wheel angle     %+7.1f deg  = -s / R,  R = %.2f m' % (c.wheel_angle, WHEEL_RADIUS),
            'elevation       %7.1f deg' % c.elevation,
            'recoil          %7.3f m' % c.recoil,
            'muzzle          (%.2f, %.2f, %.2f)' % (pos[0], pos[1], pos[2]),
            'predicted range %7.2f m   (flight %.2f s)' % (rng, t_fly),
            'shading         %s / %s' % (mode, spec),
            'light           %s, %s' % ('ON' if self.light.enabled else 'OFF',
                                        'directional' if self.light.directional else 'point'),
            'camera          %s   %.0f fps' % (CAMERA_PRESETS[self.cam_index][0], self.fps),
        ]
        utils.filled_rect(10, self.height - 26 - 17 * len(left), 470,
                          17 * len(left) + 16, (0.0, 0.0, 0.0, 0.45))
        y = self.height - 32
        for i, line in enumerate(left):
            col = (1.0, 0.86, 0.45) if i == 0 else (0.92, 0.94, 0.96)
            utils.text(20, y, line, color=col)
            y -= 17

        if self.show_help:
            help_lines = [
                'Left/Right  drive        Up/Down  elevate      Space  fire',
                'C camera   mouse orbit   +/- zoom              R reset',
                'L light   K orbit  N point/dir  [ ] angle',
                'F Phong/Gouraud   B Blinn   W wireframe   G ground',
                'T trails   X clear   H help   P screenshot   Esc quit',
            ]
            utils.filled_rect(10, 10, 640, 17 * len(help_lines) + 14, (0.0, 0.0, 0.0, 0.45))
            y = 10 + 17 * len(help_lines) - 4
            for line in help_lines:
                utils.text(20, y, line, color=(0.85, 0.90, 0.95))
                y -= 17

        if self.status_t > 0.0:
            utils.text(self.width - 20 - 9 * len(self.status), self.height - 32,
                       self.status, color=(1.0, 0.80, 0.35))

        utils.end_2d()
        glDisable(GL_BLEND)
        glEnable(GL_DEPTH_TEST)
        glEnable(GL_FOG)
        glEnable(GL_LIGHTING)

    # ------------------------------------------------------------- capture
    def measure_fps(self, frames=30):
        """Time real render calls so the HUD figure shows a measured rate."""
        self.render()
        glFinish()
        t0 = time.perf_counter()
        for _ in range(frames):
            self.render()
        glFinish()
        dt = time.perf_counter() - t0
        self.fps = frames / dt if dt > 0 else 0.0
        return self.fps

    def screenshot(self, path):
        from PIL import Image
        glPixelStorei(GL_PACK_ALIGNMENT, 1)
        glReadBuffer(GL_BACK)
        data = glReadPixels(0, 0, self.width, self.height, GL_RGB, GL_UNSIGNED_BYTE)
        img = Image.frombytes('RGB', (self.width, self.height), data)
        img = img.transpose(Image.FLIP_TOP_BOTTOM)
        os.makedirs(os.path.dirname(os.path.abspath(path)), exist_ok=True)
        img.save(path)
        return path


APP = App()


# ============================================================== GLUT hooks

def on_display():
    APP.render()
    glutSwapBuffers()


def on_reshape(w, h):
    APP.width, APP.height = max(w, 1), max(h, 1)
    glViewport(0, 0, APP.width, APP.height)


def on_idle():
    now = time.perf_counter()
    if APP.last_time is None:
        APP.last_time = now
        return
    dt = min(now - APP.last_time, 0.05)
    APP.last_time = now
    APP._fps_acc += dt
    APP._fps_n += 1
    if APP._fps_acc >= 0.4:
        APP.fps = APP._fps_n / APP._fps_acc
        APP._fps_acc, APP._fps_n = 0.0, 0
    APP.update(dt)
    glutPostRedisplay()


def on_special(key, x, y):
    APP.special.add(key)


def on_special_up(key, x, y):
    APP.special.discard(key)


def on_key(key, x, y):
    k = key.decode('latin-1').lower() if isinstance(key, bytes) else str(key).lower()
    c = APP.cannon
    if k in ('\x1b', 'q'):
        try:
            glutLeaveMainLoop()
        except Exception:
            sys.exit(0)
    elif k == ' ':
        APP.fire()
    elif k == 'c':
        APP.set_preset(APP.cam_index + 1)
    elif k == 'l':
        APP.light.enabled = not APP.light.enabled
        APP.notify('light %s' % ('ON' if APP.light.enabled else 'OFF'))
    elif k == 'k':
        APP.light.animate = not APP.light.animate
        APP.notify('light orbit %s' % ('ON' if APP.light.animate else 'OFF'))
    elif k == 'n':
        APP.light.directional = not APP.light.directional
        APP.notify('%s light' % ('directional' if APP.light.directional else 'point'))
    elif k == '[':
        APP.light.angle = (APP.light.angle - 6.0) % 360.0
    elif k == ']':
        APP.light.angle = (APP.light.angle + 6.0) % 360.0
    elif k == 'f':
        if APP.shader.available:
            APP.per_fragment = not APP.per_fragment
            APP.notify('shading: %s' % ('per-fragment Phong' if APP.per_fragment
                                        else 'per-vertex Gouraud'))
        else:
            APP.notify('no GLSL support: fixed function only')
    elif k == 'b':
        APP.blinn = not APP.blinn
        APP.notify('specular: %s' % ('Blinn-Phong' if APP.blinn else 'Phong'))
    elif k == 'w':
        APP.wireframe = not APP.wireframe
    elif k == 'g':
        APP.show_ground = not APP.show_ground
    elif k == 'j':
        APP.show_grid = not APP.show_grid
    elif k == 't':
        APP.projectiles.show_trails = not APP.projectiles.show_trails
    elif k == 'x':
        APP.projectiles.clear()
    elif k == 'h':
        APP.show_help = not APP.show_help
    elif k == 'u':
        APP.show_hud = not APP.show_hud
    elif k == 'r':
        c.reset()
        APP.projectiles.clear()
        APP.set_preset(0)
        APP.notify('reset')
    elif k in ('+', '='):
        APP.dist = utils.clamp(APP.dist - 0.5, 2.5, 40.0)
    elif k in ('-', '_'):
        APP.dist = utils.clamp(APP.dist + 0.5, 2.5, 40.0)
    elif k == 'p':
        name = time.strftime('shot_%Y%m%d_%H%M%S.png')
        APP.render()
        APP.notify('saved ' + APP.screenshot(os.path.join('docs', 'images', name)))
    elif k == 'm':
        APP.barrel_mat = (APP.barrel_mat + 1) % len(BARREL_MATERIALS)
        APP.notify('barrel material: ' + BARREL_MATERIALS[APP.barrel_mat].name)


def on_mouse(button, state, x, y):
    if button == GLUT_LEFT_BUTTON:
        APP.dragging = (state == GLUT_DOWN)
        APP.last_mouse = (x, y)
    elif button == 3 and state == GLUT_DOWN:
        APP.dist = utils.clamp(APP.dist - 0.5, 2.5, 40.0)
    elif button == 4 and state == GLUT_DOWN:
        APP.dist = utils.clamp(APP.dist + 0.5, 2.5, 40.0)


def on_motion(x, y):
    if APP.dragging:
        dx, dy = x - APP.last_mouse[0], y - APP.last_mouse[1]
        APP.last_mouse = (x, y)
        APP.yaw = (APP.yaw + dx * 0.35) % 360.0
        APP.pitch = utils.clamp(APP.pitch - dy * 0.30, -5.0, 85.0)


def on_wheel(wheel, direction, x, y):
    APP.dist = utils.clamp(APP.dist - 0.6 * direction, 2.5, 40.0)


# ================================================================= figures

def run_shots(outdir):
    """Render a deterministic set of figures for the report, then exit."""
    app = APP
    app.show_hud = False
    app.show_help = False
    app.light.animate = False
    os.makedirs(outdir, exist_ok=True)

    def step(seconds, dt=1.0 / 240.0):
        n = int(seconds / dt)
        for _ in range(n):
            app.update(dt)

    def reset(elev=20.0, x=0.0, light=52.0, cam=0):
        app.cannon.reset()
        app.projectiles.clear()
        app.cannon.elevation = elev
        app.cannon.x = x
        app.light.angle = light
        app.light.enabled = True
        app.light.directional = False
        app.per_fragment = True
        app.wireframe = False
        app.show_ground = True
        app.show_grid = True
        app.set_preset(cam)
        app.cam_offset = 0.0
        app.status_t = 0.0

    shots = []

    def shot(name, fn):
        shots.append((name, fn))

    # 1 -- hero / overview
    def f_overview():
        reset(elev=22.0, cam=0)
        app.dist = 8.4
        app.yaw, app.pitch = 40.0, 16.0
    shot('01_overview.png', f_overview)

    # 2 -- wireframe of the same pose (shows the primitive tessellation)
    def f_wire():
        f_overview()
        app.wireframe = True
        app.show_ground = False
        app.dist, app.yaw, app.pitch, app.cam_h = 5.0, 44.0, 14.0, 1.15
    shot('02_wireframe.png', f_wire)

    # 3/4 -- rolling: identical camera, carriage driven 3.4 m
    def f_roll_a():
        reset(elev=0.0, cam=1)
        app.dist, app.yaw, app.pitch, app.cam_h = 8.6, 90.0, 7.0, 0.95
    shot('03_roll_before.png', f_roll_a)

    def f_roll_b():
        f_roll_a()
        for _ in range(260):            # drive 2.60 m, i.e. -240.2 deg of spin
            app.cannon.drive(0.01)
        app.cam_offset = -app.cannon.x  # keep the camera fixed in world space
    shot('04_roll_after.png', f_roll_b)

    # 5..7 -- elevation sweep, fixed camera
    def make_elev(deg):
        def f():
            reset(elev=deg, cam=1)
            app.dist, app.yaw, app.pitch, app.cam_h = 6.6, 96.0, 8.0, 1.15
        return f
    shot('05_elev_00.png', make_elev(0.0))
    shot('06_elev_25.png', make_elev(25.0))
    shot('07_elev_45.png', make_elev(45.0))

    # 8 -- recoil at maximum extension
    def f_recoil():
        reset(elev=30.0, cam=1)
        app.dist, app.yaw, app.pitch, app.cam_h = 6.2, 104.0, 10.0, 1.15
        app.cannon.fire()
        step(0.11)
    shot('08_recoil.png', f_recoil)

    def f_rest():
        reset(elev=30.0, cam=1)
        app.dist, app.yaw, app.pitch, app.cam_h = 6.2, 104.0, 10.0, 1.15
    shot('09_recoil_rest.png', f_rest)

    # 10 -- muzzle flash the instant the shot leaves
    def f_flash():
        reset(elev=28.0, cam=0)
        app.dist, app.yaw, app.pitch, app.cam_h = 6.8, 55.0, 12.0, 1.2
        APP.fire()
        step(0.014)
    shot('10_muzzle_flash.png', f_flash)

    # 11 -- projectile mid-flight, 12 -- complete parabola
    def f_flight():
        reset(elev=35.0, x=-9.0, light=250.0, cam=3)
        app.dist, app.yaw, app.pitch, app.cam_h = 22.0, 84.0, 15.0, 3.2
        app.cam_offset = 11.0
        APP.fire()
        step(1.0)
    shot('11_projectile_flight.png', f_flight)

    def f_arc():
        f_flight()
        step(2.4)
    shot('12_projectile_arc.png', f_arc)

    # 13/14 -- shading comparison on the same frame
    def f_phong():
        # the light sits on the opposite side of the barrel axis from the eye,
        # so the mirror condition R.V = 1 is reachable somewhere on the tube
        # and the specular streak is at its strongest
        reset(elev=18.0, light=122.0, cam=0)
        app.dist, app.yaw, app.pitch, app.cam_h = 2.7, 58.0, 13.0, 1.42
        app.cam_offset = 0.75
        app.per_fragment = True
    shot('13_shading_phong.png', f_phong)

    def f_gouraud():
        f_phong()
        app.per_fragment = False
    shot('14_shading_gouraud.png', f_gouraud)

    # 15 -- light off: only the ambient term survives
    def f_dark():
        f_phong()
        app.light.enabled = False
    shot('15_light_off.png', f_dark)

    # 16/17 -- the same pose with the light moved: the highlight tracks it
    def make_light(angle):
        def f():
            reset(elev=18.0, light=angle, cam=0)
            app.dist, app.yaw, app.pitch, app.cam_h = 5.2, 46.0, 14.0, 1.2
        return f
    shot('16_light_left.png', make_light(120.0))
    shot('17_light_right.png', make_light(10.0))

    # 18 -- two materials side by side (wood cheek + iron barrel in one frame)
    def f_materials():
        reset(elev=8.0, light=70.0, cam=0)
        app.dist, app.yaw, app.pitch, app.cam_h = 3.1, 74.0, 9.0, 1.05
    shot('18_materials.png', f_materials)

    # 19 -- directional light (w = 0), no attenuation
    def f_directional():
        reset(elev=18.0, light=58.0, cam=0)
        app.dist, app.yaw, app.pitch, app.cam_h = 5.2, 46.0, 14.0, 1.2
        app.light.directional = True
    shot('19_light_directional.png', f_directional)

    # 20..22 -- camera presets
    def make_cam(i):
        def f():
            reset(elev=24.0, cam=i)
        return f
    shot('20_cam_low.png', make_cam(2))
    shot('21_cam_overview.png', make_cam(3))
    shot('22_cam_downrange.png', make_cam(4))

    # 23 -- HUD visible
    def f_hud():
        reset(elev=32.0, cam=0)
        app.dist, app.yaw, app.pitch = 9.2, 40.0, 16.0
        APP.fire()
        step(0.6)
        app.show_hud = True
        app.show_help = True
        app.measure_fps()
    shot('23_hud.png', f_hud)

    state = {'i': 0, 'warm': 3}

    def driver():
        if state['warm'] > 0:
            state['warm'] -= 1
            app.render()
            glutSwapBuffers()
            return
        if state['i'] >= len(shots):
            print('[shots] wrote %d images to %s' % (len(shots), outdir))
            try:
                glutLeaveMainLoop()
            except Exception:
                os._exit(0)
            return
        name, fn = shots[state['i']]
        state['i'] += 1
        fn()
        app.render()
        path = app.screenshot(os.path.join(outdir, name))
        glutSwapBuffers()
        app.show_hud = False
        app.show_help = False
        print('[shots] %s' % path)

    glutIdleFunc(driver)


# ==================================================================== main

def main():
    argv = glutInit(sys.argv)
    shots = None
    if '--shots' in sys.argv:
        i = sys.argv.index('--shots')
        shots = sys.argv[i + 1] if len(sys.argv) > i + 1 else os.path.join('docs', 'images')

    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGBA | GLUT_DEPTH | GLUT_MULTISAMPLE)
    glutInitWindowSize(WINDOW_W, WINDOW_H)
    glutInitWindowPosition(60, 40)
    glutCreateWindow(TITLE)
    try:
        glutSetOption(GLUT_ACTION_ON_WINDOW_CLOSE, GLUT_ACTION_GLUTMAINLOOP_RETURNS)
    except Exception:
        pass

    APP.init_gl()

    glutDisplayFunc(on_display)
    glutReshapeFunc(on_reshape)
    glutKeyboardFunc(on_key)
    glutSpecialFunc(on_special)
    glutSpecialUpFunc(on_special_up)
    glutMouseFunc(on_mouse)
    glutMotionFunc(on_motion)
    try:
        glutMouseWheelFunc(on_wheel)
    except Exception:
        pass
    glutIgnoreKeyRepeat(1)

    if shots:
        run_shots(shots)
    else:
        glutIdleFunc(on_idle)

    glutMainLoop()


if __name__ == '__main__':
    main()

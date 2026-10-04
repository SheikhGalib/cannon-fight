import glfw
from OpenGL.GL import *
from OpenGL.GLUT import *

if not glfw.init():
    raise SystemExit("GLFW init failed")

window = glfw.create_window(800, 600, "Lab 4 - Rectangle Rotating Around Center", None, None)
if not window:
    glfw.terminate()
    raise SystemExit("Window creation failed")

glfw.make_context_current(window)
glutInit()

# Rectangle vertices (x, y) in normalized device coordinates,
# defined around the origin so the origin IS the rectangle's center.
half_w, half_h = 0.4, 0.25
vertices = [
    (-half_w, -half_h),
    (half_w, -half_h),
    (half_w, half_h),
    (-half_w, half_h),
]


def draw_rectangle():
    glColor3f(0.2, 0.6, 1.0)
    glBegin(GL_QUADS)
    for x, y in vertices:
        glVertex2f(x, y)
    glEnd()


def draw_points():
    glColor3f(1.0, 0.0, 0.0)
    glPointSize(8.0)
    glBegin(GL_POINTS)
    for x, y in vertices:
        glVertex2f(x, y)
    glEnd()


def draw_indices():
    glColor3f(1.0, 1.0, 1.0)
    for i, (x, y) in enumerate(vertices):
        glRasterPos2f(x + 0.03, y + 0.03)
        for ch in str(i):
            glutBitmapCharacter(GLUT_BITMAP_HELVETICA_18, ord(ch))


while not glfw.window_should_close(window):
    glClear(GL_COLOR_BUFFER_BIT)

    angle_deg = glfw.get_time() * 45.0  # 45 degrees per second

    glPushMatrix()
    glRotatef(angle_deg, 0.0, 0.0, 1.0)  # rotate around the origin (rectangle's center)

    draw_rectangle()
    draw_points()
    draw_indices()

    glPopMatrix()

    glfw.swap_buffers(window)
    glfw.poll_events()

glfw.terminate()

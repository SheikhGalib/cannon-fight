"""
lighting.py
===========

The lighting rig for the scene:

* `Material`  -- an (ambient, diffuse, specular, shininess) quadruple that maps
  straight onto the Phong reflection model, plus the catalogue of materials
  used by the cannon (matte wood vs. shiny iron is the required contrast).
* `Light`     -- a positional (point) light with quadratic attenuation.  It can
  orbit the scene so the specular highlight visibly sweeps across the barrel.
* `PhongShader` -- an optional GLSL program that evaluates the *same* Phong
  model **per fragment**.  It deliberately reads the legacy `gl_LightSource[0]`
  / `gl_FrontMaterial` uniforms, so exactly one set of `glLight*` / `glMaterial*`
  calls drives both the fixed-function path (per-vertex, Gouraud interpolated)
  and the shader path (per-fragment, true Phong).  Pressing `F` flips between
  them, which makes the difference easy to photograph for the report.

If shader compilation fails for any reason the program silently falls back to
the fixed-function pipeline, so the project always runs.
"""

import math

from OpenGL.GL import *

#: Atmospheric haze: blends the far edge of the ground plane into the sky so
#: the scene has no hard horizon seam.  f = exp(-(rho*z)^2), colour = mix(fog, c, f)
FOG_COLOR = (0.70, 0.73, 0.74)
FOG_DENSITY = 0.020


# --------------------------------------------------------------------------
# materials
# --------------------------------------------------------------------------


class Material:
    """Phong material coefficients (k_a, k_d, k_s, shininess n)."""

    __slots__ = ('name', 'ambient', 'diffuse', 'specular', 'shininess', 'emission')

    def __init__(self, name, ambient, diffuse, specular, shininess, emission=(0.0, 0.0, 0.0, 1.0)):
        self.name = name
        self.ambient = tuple(ambient)
        self.diffuse = tuple(diffuse)
        self.specular = tuple(specular)
        self.shininess = float(shininess)
        self.emission = tuple(emission)

    def apply(self, face=GL_FRONT_AND_BACK):
        glMaterialfv(face, GL_AMBIENT, self.ambient)
        glMaterialfv(face, GL_DIFFUSE, self.diffuse)
        glMaterialfv(face, GL_SPECULAR, self.specular)
        glMaterialf(face, GL_SHININESS, self.shininess)
        glMaterialfv(face, GL_EMISSION, self.emission)
        # keep glColor in sync so the wireframe / unlit view still reads well
        glColor3f(self.diffuse[0], self.diffuse[1], self.diffuse[2])


#: Oak carriage: warm brown, almost no specular lobe, very broad -> matte.
WOOD = Material('oak wood',
                ambient=(0.14, 0.085, 0.045, 1.0),
                diffuse=(0.52, 0.32, 0.16, 1.0),
                specular=(0.10, 0.08, 0.06, 1.0),
                shininess=8.0)

#: Cast-iron barrel: dark diffuse, strong and *tight* specular -> obviously shiny.
IRON = Material('cast iron',
                ambient=(0.06, 0.065, 0.075, 1.0),
                diffuse=(0.19, 0.20, 0.23, 1.0),
                specular=(0.92, 0.94, 1.00, 1.0),
                shininess=96.0)

#: Brass fittings / muzzle band.
BRASS = Material('brass',
                 ambient=(0.16, 0.12, 0.04, 1.0),
                 diffuse=(0.62, 0.47, 0.13, 1.0),
                 specular=(0.85, 0.75, 0.45, 1.0),
                 shininess=64.0)

#: Iron tyre around the wooden wheel.
DARK_IRON = Material('iron tyre',
                     ambient=(0.05, 0.05, 0.055, 1.0),
                     diffuse=(0.13, 0.13, 0.15, 1.0),
                     specular=(0.55, 0.55, 0.60, 1.0),
                     shininess=48.0)

#: Slightly lighter wood for spokes/hub so the wheel reads as a separate part.
WOOD_LIGHT = Material('light oak',
                      ambient=(0.16, 0.105, 0.06, 1.0),
                      diffuse=(0.60, 0.40, 0.22, 1.0),
                      specular=(0.12, 0.10, 0.08, 1.0),
                      shininess=10.0)

#: Grass field.
GROUND = Material('field',
                  ambient=(0.10, 0.13, 0.08, 1.0),
                  diffuse=(0.34, 0.44, 0.24, 1.0),
                  specular=(0.04, 0.05, 0.04, 1.0),
                  shininess=4.0)

#: Cannonball -- same family as the barrel so the arc stays readable.
BALL = Material('cannonball',
                ambient=(0.05, 0.05, 0.055, 1.0),
                diffuse=(0.16, 0.16, 0.18, 1.0),
                specular=(0.80, 0.82, 0.88, 1.0),
                shininess=80.0)

#: Marker for the light itself -- pure emission, unaffected by the light.
LAMP = Material('lamp',
                ambient=(0.0, 0.0, 0.0, 1.0),
                diffuse=(0.0, 0.0, 0.0, 1.0),
                specular=(0.0, 0.0, 0.0, 1.0),
                shininess=1.0,
                emission=(1.0, 0.94, 0.72, 1.0))

MATERIALS = [WOOD, WOOD_LIGHT, IRON, BRASS, DARK_IRON, GROUND, BALL]


# --------------------------------------------------------------------------
# the light
# --------------------------------------------------------------------------


class Light:
    """A single positional light orbiting the scene.

    `w = 1` in the position makes it a *point* light, so OpenGL recomputes
    `L = normalize(P_light - P_vertex)` per vertex and applies distance
    attenuation.  Setting `directional = True` switches to `w = 0`, i.e. the
    sun-at-infinity model where `L` is constant.
    """

    def __init__(self):
        self.enabled = True
        self.animate = True
        self.directional = False
        self.orbit_radius = 8.0
        self.height = 7.0
        self.angle = 40.0            # degrees around +Y
        self.speed = 22.0            # degrees / second
        self.target = (0.0, 1.0, 0.0)

        self.ambient = (0.20, 0.20, 0.22, 1.0)
        self.diffuse = (1.00, 0.96, 0.88, 1.0)
        self.specular = (1.00, 1.00, 0.98, 1.0)

        # attenuation 1 / (kc + kl d + kq d^2)
        self.kc, self.kl, self.kq = 1.0, 0.010, 0.0004

        self.global_ambient = (0.22, 0.22, 0.24, 1.0)

    # -- state ------------------------------------------------------------
    @property
    def position(self):
        a = math.radians(self.angle)
        return (self.target[0] + self.orbit_radius * math.cos(a),
                self.height,
                self.target[2] + self.orbit_radius * math.sin(a))

    def update(self, dt):
        if self.animate:
            self.angle = (self.angle + self.speed * dt) % 360.0

    # -- GL ---------------------------------------------------------------
    def setup(self):
        glLightModelfv(GL_LIGHT_MODEL_AMBIENT, self.global_ambient)
        glLightModeli(GL_LIGHT_MODEL_LOCAL_VIEWER, GL_TRUE)
        glLightModeli(GL_LIGHT_MODEL_TWO_SIDE, GL_FALSE)
        glLightfv(GL_LIGHT0, GL_AMBIENT, self.ambient)
        glLightfv(GL_LIGHT0, GL_DIFFUSE, self.diffuse)
        glLightfv(GL_LIGHT0, GL_SPECULAR, self.specular)
        glEnable(GL_LIGHT0)

    def upload(self):
        """Push the current position + attenuation into GL state.

        Must be called *after* the camera has been loaded into the modelview
        matrix: OpenGL stores the light position in eye space by multiplying it
        with the modelview matrix that is current at the time of the call.
        """
        x, y, z = self.position
        w = 0.0 if self.directional else 1.0
        glLightfv(GL_LIGHT0, GL_POSITION, (x, y, z, w))
        glLightf(GL_LIGHT0, GL_CONSTANT_ATTENUATION, self.kc)
        glLightf(GL_LIGHT0, GL_LINEAR_ATTENUATION, 0.0 if self.directional else self.kl)
        glLightf(GL_LIGHT0, GL_QUADRATIC_ATTENUATION, 0.0 if self.directional else self.kq)
        if self.enabled:
            glEnable(GL_LIGHT0)
        else:
            glDisable(GL_LIGHT0)


# --------------------------------------------------------------------------
# per-fragment Phong shader
# --------------------------------------------------------------------------

VERTEX_SRC = """
#version 120
varying vec3 vNormalEye;
varying vec3 vPosEye;

void main()
{
    // eye-space position and normal.  gl_NormalMatrix is the inverse-transpose
    // of the upper-left 3x3 of the modelview matrix.
    vPosEye    = vec3(gl_ModelViewMatrix * gl_Vertex);
    vNormalEye = gl_NormalMatrix * gl_Normal;
    gl_Position = ftransform();
}
"""

FRAGMENT_SRC = """
#version 120
varying vec3 vNormalEye;
varying vec3 vPosEye;

uniform bool  uLightEnabled;
uniform bool  uBlinn;
uniform vec3  uFogColor;
uniform float uFogDensity;

void main()
{
    vec3 N = normalize(vNormalEye);

    // emission + k_a * global ambient
    vec3 color = gl_FrontLightModelProduct.sceneColor.rgb;

    if (uLightEnabled) {
        vec4 lpos = gl_LightSource[0].position;      // already in eye space
        vec3  Lv  = lpos.xyz - vPosEye * lpos.w;     // w=0 -> directional
        float d   = length(Lv);
        vec3  L   = Lv / max(d, 1e-6);

        float att = 1.0;
        if (lpos.w > 0.5) {
            att = 1.0 / (gl_LightSource[0].constantAttenuation
                       + gl_LightSource[0].linearAttenuation    * d
                       + gl_LightSource[0].quadraticAttenuation * d * d);
        }

        vec3  V    = normalize(-vPosEye);            // local viewer
        float ndl  = max(dot(N, L), 0.0);

        float spec = 0.0;
        if (ndl > 0.0) {
            if (uBlinn) {
                vec3 H = normalize(L + V);           // Blinn-Phong half vector
                spec = pow(max(dot(N, H), 0.0), gl_FrontMaterial.shininess);
            } else {
                vec3 R = reflect(-L, N);             // classic Phong mirror dir
                spec = pow(max(dot(R, V), 0.0), gl_FrontMaterial.shininess);
            }
        }

        vec3 amb = gl_LightSource[0].ambient.rgb  * gl_FrontMaterial.ambient.rgb;
        vec3 dif = gl_LightSource[0].diffuse.rgb  * gl_FrontMaterial.diffuse.rgb  * ndl;
        vec3 spc = gl_LightSource[0].specular.rgb * gl_FrontMaterial.specular.rgb * spec;

        color += att * (amb + dif + spc);
    }

    // exponential-squared distance fog, evaluated in eye space
    float z = length(vPosEye);
    float f = clamp(exp(-(uFogDensity * z) * (uFogDensity * z)), 0.0, 1.0);
    color = mix(uFogColor, color, f);

    gl_FragColor = vec4(color, gl_FrontMaterial.diffuse.a);
}
"""


class PhongShader:
    """Compiles the per-fragment Phong program; degrades gracefully."""

    def __init__(self):
        self.program = None
        self.available = False
        self.log = ''
        self._u_light = -1
        self._u_blinn = -1
        self._u_fogc = -1
        self._u_fogd = -1
        self.active = False
        self.last_light = True
        self.last_blinn = False

    def compile(self):
        try:
            vs = self._shader(GL_VERTEX_SHADER, VERTEX_SRC)
            fs = self._shader(GL_FRAGMENT_SHADER, FRAGMENT_SRC)
            prog = glCreateProgram()
            glAttachShader(prog, vs)
            glAttachShader(prog, fs)
            glLinkProgram(prog)
            if not glGetProgramiv(prog, GL_LINK_STATUS):
                self.log = glGetProgramInfoLog(prog).decode(errors='replace')
                raise RuntimeError('link failed: ' + self.log)
            glDeleteShader(vs)
            glDeleteShader(fs)
            self.program = prog
            self._u_light = glGetUniformLocation(prog, 'uLightEnabled')
            self._u_blinn = glGetUniformLocation(prog, 'uBlinn')
            self._u_fogc = glGetUniformLocation(prog, 'uFogColor')
            self._u_fogd = glGetUniformLocation(prog, 'uFogDensity')
            self.available = True
            set_active_shader(self)
        except Exception as exc:            # noqa: BLE001 - any GL failure
            self.available = False
            self.log = str(exc)
        return self.available

    @staticmethod
    def _shader(kind, src):
        sh = glCreateShader(kind)
        glShaderSource(sh, src)
        glCompileShader(sh)
        if not glGetShaderiv(sh, GL_COMPILE_STATUS):
            raise RuntimeError(glGetShaderInfoLog(sh).decode(errors='replace'))
        return sh

    # -- binding ----------------------------------------------------------
    def bind(self, light_enabled=True, blinn=False):
        if not self.available:
            return False
        glUseProgram(self.program)
        glUniform1i(self._u_light, 1 if light_enabled else 0)
        glUniform1i(self._u_blinn, 1 if blinn else 0)
        glUniform3f(self._u_fogc, *FOG_COLOR)
        glUniform1f(self._u_fogd, FOG_DENSITY)
        self.last_light, self.last_blinn = light_enabled, blinn
        self.active = True
        return True

    def unbind(self):
        if self.available and self.active:
            glUseProgram(0)
        self.active = False


# --------------------------------------------------------------------------
# unlit drawing
# --------------------------------------------------------------------------

_ACTIVE = {'shader': None}


def set_active_shader(shader):
    _ACTIVE['shader'] = shader


def setup_fog():
    """Fixed-function fog, for the Gouraud path and all unlit geometry."""
    glFogi(GL_FOG_MODE, GL_EXP2)
    glFogfv(GL_FOG_COLOR, FOG_COLOR + (1.0,))
    glFogf(GL_FOG_DENSITY, FOG_DENSITY)
    glHint(GL_FOG_HINT, GL_NICEST)
    glEnable(GL_FOG)


class unlit:
    """Context manager for flat-coloured geometry (trails, markers, HUD).

    It drops out of the shader *and* out of `GL_LIGHTING` so that plain
    `glColor` values survive, then restores whichever path was active.
    """

    def __init__(self, fog=True):
        self.fog = fog
        self.restore = None

    def __enter__(self):
        sh = _ACTIVE['shader']
        if sh is not None and sh.active:
            self.restore = (sh, sh.last_light, sh.last_blinn)
            sh.unbind()
        glDisable(GL_LIGHTING)
        if not self.fog:
            glDisable(GL_FOG)
        return self

    def __exit__(self, *exc):
        if not self.fog:
            glEnable(GL_FOG)
        glEnable(GL_LIGHTING)
        if self.restore is not None:
            sh, le, bl = self.restore
            sh.bind(le, bl)
            self.restore = None
        return False

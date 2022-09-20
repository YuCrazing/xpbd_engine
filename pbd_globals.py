import taichi as ti
import math

BUFFER_MAX_SIZE = 500000


vec2 = ti.types.vector(2, ti.f32)
vec3 = ti.types.vector(3, ti.f32)
mat33 = ti.types.matrix(3, 3, ti.f32)

# object phase
PHASE_RIGID_BODY = 0
PHASE_SOFT_BODY = 1
PHASE_FLUID = 2
PHASE_CLOTH = 3

# object kinematic types
STATE_STATIC = 0
STATE_KINEMATIC = 1
STATE_DYNAMIC = 2

# cluster types
CLUSTER_SOFT_BODY = 0
CLUSTER_RIGID_BODY = 1
CLUSTER_FLUID = 2
CLUSTER_CLOTH = 3
CLUSTER_ROPE = 4

DYNAMIC_RIGID_COLOR = vec3(0.5, 0.3, 0.0)
STATIC_RIGID_COLOR = vec3(0.0, 0.3, 0.5)
FLUID_COLOR = vec3(0.0, 0.4, 0.8)

color_table = [
    vec3(0.5, 0.3, 0.0),
    vec3(0.0, 0.3, 0.5),
    vec3(0.0, 0.5, 0.3),
    vec3(0.4, 0.0, 0.1),
]


mat33_identity = mat33([[1.0, 0.0, 0.0], [0.0, 1.0, 0.0], [0.0, 0.0, 1.0]])
rad = math.pi / 6
mat33_xy_30 = mat33(
    [[ti.cos(rad), -ti.sin(rad), 0.0], [ti.sin(rad), ti.cos(rad), 0.0], [0.0, 0.0, 1.0]]
)

inf = 1e30
eps = 1e-6
pi = math.pi


@ti.func
def W_poly6(R, h):
    r = R.norm()
    res = 0.0
    if r <= h:
        h2 = h * h
        h4 = h2 * h2
        h9 = h4 * h4 * h
        h2_r2 = h2 - r * r
        res = 315.0 / (64 * pi * h9) * h2_r2 * h2_r2 * h2_r2
    else:
        res = 0.0
    return res


@ti.func
def W_spiky_gradient(R, h):
    r = R.norm()
    res = ti.Vector([0.0, 0.0, 0.0])
    if r < eps:
        res = ti.Vector([0.0, 0.0, 0.0])
    elif r <= h:
        h3 = h * h * h
        h6 = h3 * h3
        h_r = h - r
        res = -45.0 / (pi * h6) * h_r * h_r * (R / r)
    else:
        res = ti.Vector([0.0, 0.0, 0.0])
    return res


W = W_poly6
W_gradient = W_spiky_gradient


@ti.func
def isnan(x):
    return not (x >= 0 or x <= 0)


def blit_from_ext_arr_to_field(
    field: ti.template(), ext_arr: ti.ext_arr(), offset: ti.i32, size: ti.i32
):
    for i in range(size):
        field[i + offset] = ext_arr[i]

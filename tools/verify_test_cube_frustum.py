#!/usr/bin/env python3
"""Independent frustum vs chunk AABB check for the test_cube camera."""

from __future__ import annotations

import math

CHUNK = 32
CAMERA = {
    "position": (52.0, 44.0, 52.0),
    "yaw": -135.0,
    "pitch": -26.0,
    "fov_y": 50.0,
    "near": 0.1,
    "far": 256.0,
    "aspect": 1.0,
}


def normalize(v: tuple[float, float, float]) -> tuple[float, float, float]:
    ln = math.sqrt(sum(x * x for x in v))
    return (v[0] / ln, v[1] / ln, v[2] / ln)


def mat_mul_colmajor(a, b):
    out = [[0.0] * 4 for _ in range(4)]
    for j in range(4):
        bcol = [b[j][r] for r in range(4)]
        for r in range(4):
            out[j][r] = sum(a[k][r] * bcol[k] for k in range(4))
    return out


def mat_vec_colmajor(m, v):
    return tuple(sum(m[c][r] * v[c] for c in range(4)) for r in range(4))


def look_at(eye, center, up=(0.0, 1.0, 0.0)):
    f = normalize((center[0] - eye[0], center[1] - eye[1], center[2] - eye[2]))
    r = normalize((f[1] * up[2] - f[2] * up[1], f[2] * up[0] - f[0] * up[2], f[0] * up[1] - f[1] * up[0]))
    u = (r[1] * f[2] - r[2] * f[1], r[2] * f[0] - r[0] * f[2], r[0] * f[1] - r[1] * f[0])
    return [
        [r[0], r[1], r[2], 0.0],
        [u[0], u[1], u[2], 0.0],
        [-f[0], -f[1], -f[2], 0.0],
        [
            -r[0] * eye[0] - r[1] * eye[1] - r[2] * eye[2],
            -u[0] * eye[0] - u[1] * eye[1] - u[2] * eye[2],
            f[0] * eye[0] + f[1] * eye[1] + f[2] * eye[2],
            1.0,
        ],
    ]


def perspective(fov_y_deg: float, aspect: float, near: float, far: float):
    f = 1.0 / math.tan(math.radians(fov_y_deg) / 2.0)
    return [
        [f / aspect, 0.0, 0.0, 0.0],
        [0.0, f, 0.0, 0.0],
        [0.0, 0.0, far / (near - far), -1.0],
        [0.0, 0.0, (near * far) / (near - far), 0.0],
    ]


def extract_planes_vp(vp):
    planes = []
    for axis, positive in ((0, True), (0, False), (1, True), (1, False), (2, True), (2, False)):
        p = []
        for r in range(4):
            if positive:
                p.append(vp[3][r] + vp[axis][r])
            else:
                p.append(vp[3][r] - vp[axis][r])
        ln = math.sqrt(sum(x * x for x in p[:3]))
        planes.append(tuple(x / ln for x in p))
    return planes


def aabb_intersects_planes(mn, mx, planes) -> bool:
    for plane in planes:
        px = mx[0] if plane[0] >= 0 else mn[0]
        py = mx[1] if plane[1] >= 0 else mn[1]
        pz = mx[2] if plane[2] >= 0 else mn[2]
        if plane[0] * px + plane[1] * py + plane[2] * pz + plane[3] < 0:
            return False
    return True


def forward_from_yaw_pitch(yaw: float, pitch: float):
    yr = math.radians(yaw)
    pr = math.radians(pitch)
    return (
        math.cos(yr) * math.cos(pr),
        math.sin(pr),
        math.sin(yr) * math.cos(pr),
    )


def corner_ndc_visible(clip: tuple[float, float, float, float]) -> bool:
    x, y, z, w = clip
    if w <= 1e-5:
        return False
    ndc_x = x / w
    ndc_y = y / w
    ndc_z = z / w
    return -1.0 <= ndc_x <= 1.0 and -1.0 <= ndc_y <= 1.0 and -1.0 <= ndc_z <= 1.0


def main() -> None:
    eye = CAMERA["position"]
    fwd = forward_from_yaw_pitch(CAMERA["yaw"], CAMERA["pitch"])
    center = (eye[0] + fwd[0], eye[1] + fwd[1], eye[2] + fwd[2])
    v = look_at(eye, center)
    p = perspective(CAMERA["fov_y"], CAMERA["aspect"], CAMERA["near"], CAMERA["far"])
    vp = mat_mul_colmajor(p, v)
    planes = extract_planes_vp(vp)

    print(f"camera eye={eye} look_at≈{tuple(round(c, 2) for c in center)}")
    print("chunk(cx,cy,cz)  frustum_planes  corner_in_ndc  has_geometry")
    frustum_hits = 0
    for cz in (-1, 0, 1):
        for cy in (-1, 0, 1):
            for cx in (-1, 0, 1):
                mn = (cx * CHUNK, cy * CHUNK, cz * CHUNK)
                mx = (mn[0] + CHUNK, mn[1] + CHUNK, mn[2] + CHUNK)
                hit = aabb_intersects_planes(mn, mx, planes)
                corners = any(
                    corner_ndc_visible(mat_vec_colmajor(vp, (x, y, z, 1.0)))
                    for x in (mn[0], mx[0])
                    for y in (mn[1], mx[1])
                    for z in (mn[2], mx[2])
                )
                if hit:
                    frustum_hits += 1
                print(
                    f"({cx:2},{cy:2},{cz:2})  "
                    f"{'inside' if hit else 'outside':7}  "
                    f"{'yes' if corners else 'no':3}  yes"
                )
    print(f"summary: frustum_inside={frustum_hits}/27 (all chunks have 6^3 geometry)")


if __name__ == "__main__":
    main()

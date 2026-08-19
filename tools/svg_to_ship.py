#!/usr/bin/env python3
"""Convert a hand-drawn (or exported) SVG into ship_playground.cpp-style draw code.

Draw straight-edge shapes in any SVG tool (Figma, Illustrator, pixel-art
exporters, whatever) as <polygon>, <rect>, or simple <path> elements made
only of M/L/H/V/Z (straight lines — flatten/convert any curves to lines
first). Give each shape a fill color; that becomes its C++ color. Y already
points down in SVG, same as this engine's screen-space convention, so no
flip is needed. The whole drawing's bounding box is normalized so its
half-height maps to r=1 (i.e. paste the output into a function that
receives `r`), and centered so its middle sits at the origin. Nose should
point up (-Y) in your drawing to match drawShipHull's convention.

Convex shapes are drawn with a single DrawTriangleFan call each. Concave
outlines (staircase pixel-art edges, notches, zigzags) fall back to
ear-clip triangulation into multiple DrawTriangle calls, since a plain fan
fills those wrong. <defs> subtrees and elements classed "frame-background"
(common in design-tool exports) are skipped automatically.

Usage:
    python3 svg_to_ship.py hull.svg

Prints C++ ready to paste into ship_playground.cpp's drawShip() or into a
ShipClass case in src/draw.cpp's drawShipHull().
"""

import re
import sys
import xml.etree.ElementTree as ET

SVG_NS = "{http://www.w3.org/2000/svg}"

NAMED_COLORS = {
    "black": (0, 0, 0), "white": (255, 255, 255), "red": (255, 0, 0),
    "green": (0, 128, 0), "blue": (0, 0, 255),
}


def parse_color(fill: str):
    fill = fill.strip()
    if fill.startswith("#"):
        hexpart = fill[1:]
        if len(hexpart) == 3:
            hexpart = "".join(c * 2 for c in hexpart)
        r, g, b = (int(hexpart[i:i + 2], 16) for i in (0, 2, 4))
        return (r, g, b)
    m = re.match(r"rgb\(\s*(\d+)[,\s]+(\d+)[,\s]+(\d+)\s*\)", fill)
    if m:
        return tuple(int(x) for x in m.groups())
    return NAMED_COLORS.get(fill.lower(), (200, 200, 200))


def parse_points_attr(points: str):
    nums = [float(n) for n in re.split(r"[,\s]+", points.strip()) if n]
    return list(zip(nums[0::2], nums[1::2]))


def parse_path_d(d: str):
    """Straight-line-only path parser: M/L/H/V/Z, upper (absolute) and lower (relative)."""
    tokens = re.findall(r"([MmLlHhVvZz])|(-?\d*\.?\d+(?:e-?\d+)?)", d)
    shapes = []
    pts = []
    cur = (0.0, 0.0)
    cmd = None
    nums = []

    def flush_cmd():
        nonlocal cur, pts
        if cmd in ("M", "L"):
            for i in range(0, len(nums), 2):
                cur = (nums[i], nums[i + 1])
                pts.append(cur)
        elif cmd in ("m", "l"):
            for i in range(0, len(nums), 2):
                cur = (cur[0] + nums[i], cur[1] + nums[i + 1])
                pts.append(cur)
        elif cmd == "H":
            for x in nums:
                cur = (x, cur[1]); pts.append(cur)
        elif cmd == "h":
            for x in nums:
                cur = (cur[0] + x, cur[1]); pts.append(cur)
        elif cmd == "V":
            for y in nums:
                cur = (cur[0], y); pts.append(cur)
        elif cmd == "v":
            for y in nums:
                cur = (cur[0], cur[1] + y); pts.append(cur)
        elif cmd in ("Z", "z"):
            if pts:
                shapes.append(pts.copy())
                pts.clear()

    for letter, num in tokens:
        if letter:
            if nums and cmd:
                flush_cmd()
            cmd = letter
            nums = []
        else:
            nums.append(float(num))
    if nums and cmd:
        flush_cmd()
    if pts:
        shapes.append(pts)
    return shapes


def dedupe(pts, eps=1e-6):
    out = []
    for p in pts:
        if not out or abs(p[0] - out[-1][0]) > eps or abs(p[1] - out[-1][1]) > eps:
            out.append(p)
    if len(out) > 1 and abs(out[0][0] - out[-1][0]) <= eps and abs(out[0][1] - out[-1][1]) <= eps:
        out.pop()
    return out


def collect_shapes(root):
    """Returns list of (points, color) in document order. Skips <defs> and background frames."""
    shapes = []

    def walk(el):
        tag = el.tag.replace(SVG_NS, "")
        if tag == "defs":
            return
        cls = el.get("class", "")
        if "frame-background" in cls or "frame-clip" in (el.get("id") or ""):
            return

        style = el.get("style", "")
        fill = el.get("fill")
        m = re.search(r"fill:\s*([^;]+)", style)
        if m:
            fill = m.group(1)

        if fill and fill != "none":
            color = parse_color(fill)
            if tag == "polygon" and el.get("points"):
                pts = dedupe(parse_points_attr(el.get("points")))
                if len(pts) >= 3:
                    shapes.append((pts, color))
            elif tag == "rect":
                x, y = float(el.get("x", 0)), float(el.get("y", 0))
                w, h = float(el.get("width", 0)), float(el.get("height", 0))
                shapes.append(([(x, y), (x + w, y), (x + w, y + h), (x, y + h)], color))
            elif tag == "path" and el.get("d"):
                for pts in parse_path_d(el.get("d")):
                    pts = dedupe(pts)
                    if len(pts) >= 3:
                        shapes.append((pts, color))

        for child in el:
            walk(child)

    walk(root)
    return shapes


def point_in_polygon(pt, poly):
    x, y = pt
    inside = False
    n = len(poly)
    j = n - 1
    for i in range(n):
        xi, yi = poly[i]
        xj, yj = poly[j]
        if (yi > y) != (yj > y):
            xcross = (xj - xi) * (y - yi) / (yj - yi) + xi
            if x < xcross:
                inside = not inside
        j = i
    return inside


def polygon_area(pts):
    return abs(sum(pts[i][0] * pts[(i + 1) % len(pts)][1] - pts[(i + 1) % len(pts)][0] * pts[i][1]
                   for i in range(len(pts)))) / 2


def outer_silhouette(shapes):
    """Rasterizes the union of every shape onto the integer pixel grid, traces the boundary
    of the filled region, and collapses straight runs into single edges. Returns one
    (points, color) — the ship's outer outline instead of dozens of separate shading shapes.
    Color is taken from the largest-area input shape (usually the main hull fill).
    """
    all_pts = [pt for pts, _ in shapes for pt in pts]
    minx = int(min(p[0] for p in all_pts))
    maxx = int(max(p[0] for p in all_pts)) + 1
    miny = int(min(p[1] for p in all_pts))
    maxy = int(max(p[1] for p in all_pts)) + 1
    w, h = maxx - minx, maxy - miny

    filled = [[False] * h for _ in range(w)]
    for gx in range(w):
        cx = minx + gx + 0.5
        for gy in range(h):
            cy = miny + gy + 0.5
            for pts, _ in shapes:
                if point_in_polygon((cx, cy), pts):
                    filled[gx][gy] = True
                    break

    def is_filled(x, y):
        return 0 <= x < w and 0 <= y < h and filled[x][y]

    edge_from = {}
    for x in range(w):
        for y in range(h):
            if not filled[x][y]:
                continue
            gx, gy = x + minx, y + miny
            if not is_filled(x, y - 1):
                edge_from[(gx, gy)] = (gx + 1, gy)
            if not is_filled(x + 1, y):
                edge_from[(gx + 1, gy)] = (gx + 1, gy + 1)
            if not is_filled(x, y + 1):
                edge_from[(gx + 1, gy + 1)] = (gx, gy + 1)
            if not is_filled(x - 1, y):
                edge_from[(gx, gy + 1)] = (gx, gy)

    visited = set()
    loops = []
    for start in list(edge_from.keys()):
        if start in visited:
            continue
        loop = []
        cur = start
        while cur not in visited:
            loop.append(cur)
            visited.add(cur)
            cur = edge_from.get(cur)
            if cur is None:
                break
        loops.append(loop)

    outline = max(loops, key=lambda lp: polygon_area(lp) if len(lp) >= 3 else 0)

    # Collapse straight runs (many unit-grid edges in the same direction) into one edge.
    simplified = []
    n = len(outline)
    for i in range(n):
        prev, cur, nxt = outline[(i - 1) % n], outline[i], outline[(i + 1) % n]
        d1 = (cur[0] - prev[0], cur[1] - prev[1])
        d2 = (nxt[0] - cur[0], nxt[1] - cur[1])
        if d1[0] * d2[1] - d1[1] * d2[0] != 0:
            simplified.append(cur)

    biggest = max(shapes, key=lambda s: polygon_area(s[0]))
    return simplified, biggest[1]


def cross(o, a, b):
    return (a[0] - o[0]) * (b[1] - o[1]) - (a[1] - o[1]) * (b[0] - o[0])


def signed_area(pts):
    s = 0.0
    n = len(pts)
    for i in range(n):
        x1, y1 = pts[i]
        x2, y2 = pts[(i + 1) % n]
        s += x1 * y2 - x2 * y1
    return s / 2


def point_in_triangle(p, a, b, c):
    d1, d2, d3 = cross(a, b, p), cross(b, c, p), cross(c, a, p)
    has_neg = d1 < 0 or d2 < 0 or d3 < 0
    has_pos = d1 > 0 or d2 > 0 or d3 > 0
    return not (has_neg and has_pos)


def is_convex(pts):
    n = len(pts)
    if n < 3:
        return False
    signs = [cross(pts[i], pts[(i + 1) % n], pts[(i + 2) % n]) for i in range(n)]
    signs = [s for s in signs if abs(s) > 1e-9]
    return bool(signs) and (all(s > 0 for s in signs) or all(s < 0 for s in signs))


def ear_clip(pts):
    """Returns a list of (i, j, k) index triples into pts. Handles concave simple polygons.

    Winding: this raylib build only fills triangles wound so cross(a,b,c) < 0 in this
    (screen-space, Y-down) coordinate system — the opposite of the usual "positive area
    is CCW" convention. Verified empirically against a hand-written triangle that's known
    to render. Ears are picked accordingly, and the whole polygon is pre-oriented to match.
    """
    idx = list(range(len(pts)))
    if signed_area(pts) > 0:
        idx.reverse()

    triangles = []
    guard = 0
    while len(idx) > 3 and guard < 20000:
        guard += 1
        n = len(idx)
        clipped = False
        for i in range(n):
            ip, ic, inx = idx[(i - 1) % n], idx[i], idx[(i + 1) % n]
            a, b, c = pts[ip], pts[ic], pts[inx]
            if cross(a, b, c) >= -1e-9:
                continue
            if any(point_in_triangle(pts[j], a, b, c)
                   for j in idx if j not in (ip, ic, inx)):
                continue
            triangles.append((ip, ic, inx))
            idx.pop(i)
            clipped = True
            break
        if not clipped:
            break  # degenerate/self-intersecting input; keep what we clipped so far
    if len(idx) == 3:
        triangles.append((idx[0], idx[1], idx[2]))
    return triangles


def main():
    args = [a for a in sys.argv[1:] if not a.startswith("--")]
    outline_only = "--outline" in sys.argv
    if len(args) != 1:
        print(__doc__)
        print("\nOptional --outline: collapse every shape into one simplified outer silhouette\n"
              "(rasterize the union, trace the boundary, merge straight runs) instead of\n"
              "emitting each SVG shape separately. Far fewer draw calls; loses interior\n"
              "shading/color detail — just the hull outline in one color.")
        sys.exit(1)

    tree = ET.parse(args[0])
    root = tree.getroot()
    shapes = collect_shapes(root)
    if not shapes:
        print("// No <polygon>/<rect>/straight <path> shapes found.", file=sys.stderr)
        sys.exit(1)

    if outline_only:
        shapes = [outer_silhouette(shapes)]

    all_pts = [pt for pts, _ in shapes for pt in pts]
    minx = min(p[0] for p in all_pts)
    maxx = max(p[0] for p in all_pts)
    miny = min(p[1] for p in all_pts)
    maxy = max(p[1] for p in all_pts)
    cx, cy = (minx + maxx) / 2, (miny + maxy) / 2
    scale = (maxy - miny) / 2 or 1.0

    def norm(pt):
        return ((pt[0] - cx) / scale, (pt[1] - cy) / scale)

    print("// Generated by tools/svg_to_ship.py — paste into drawShip()/drawShipHull().")
    print("// Uses the `at(x, y)` helper from ship_playground.cpp (local units, x*r,y*r from center).")
    skipped = 0
    fan_count = 0
    for i, (pts, color) in enumerate(shapes):
        npts = [norm(p) for p in pts]
        r, g, b = color

        if is_convex(npts):
            # Convex: one DrawTriangleFan call instead of N DrawTriangle calls. Needs the
            # same winding as ear_clip (cross(a,b,c) < 0 in this screen-space convention).
            ordered = list(npts)
            if signed_area(ordered) > 0:
                ordered.reverse()
            fan_count += 1
            print(f"    {{")
            print(f"        const Color shapeColor{i} = Color{{.r = {r}, .g = {g}, .b = {b}, .a = 255}};")
            print(f"        const std::array<Vector2, {len(ordered)}> shape{i} = {{")
            for x, y in ordered:
                print(f"            at({x:.3f}F, {y:.3f}F),")
            print(f"        }};")
            print(f"        DrawTriangleFan(shape{i}.data(), static_cast<int>(shape{i}.size()), shapeColor{i});")
            print(f"    }}")
            continue

        tris = ear_clip(npts)
        if not tris:
            skipped += 1
            continue
        print(f"    {{")
        print(f"        const Color shapeColor{i} = Color{{.r = {r}, .g = {g}, .b = {b}, .a = 255}};")
        print(f"        const std::array<Vector2, {len(npts)}> shape{i} = {{")
        for x, y in npts:
            print(f"            at({x:.3f}F, {y:.3f}F),")
        print(f"        }};")
        for a, b_, c in tris:
            print(f"        DrawTriangle(shape{i}[{a}], shape{i}[{b_}], shape{i}[{c}], shapeColor{i});")
        print(f"    }}")
    if skipped:
        print(f"// NOTE: {skipped} shape(s) skipped — degenerate or self-intersecting outline.",
              file=sys.stderr)
    print(f"// {fan_count}/{len(shapes)} shapes were convex -> single DrawTriangleFan each.",
          file=sys.stderr)


if __name__ == "__main__":
    main()

#!/usr/bin/env python3
"""Pygame visualizer for scheduled intersection traffic."""

from __future__ import annotations

import argparse
import json
import math
from dataclasses import dataclass
from pathlib import Path
from typing import Iterable, TYPE_CHECKING

if TYPE_CHECKING:
    import pygame

pygame = None


Point = tuple[float, float]
Color = tuple[int, int, int]


BACKGROUND = (236, 238, 232)
ROAD = (73, 78, 78)
ROAD_EDGE = (47, 51, 51)
LANE_MARK = (220, 215, 180)
ZONE = (249, 191, 59)
ZONE_ACTIVE = (237, 92, 72)
TEXT = (36, 42, 42)
MUTED_TEXT = (91, 100, 98)
PANEL = (250, 250, 244)
PANEL_LINE = (204, 207, 198)
PATH_LINE = (40, 131, 146)

CAR_COLORS: list[Color] = [
    (28, 126, 214),
    (224, 49, 49),
    (47, 158, 68),
    (245, 159, 0),
    (156, 54, 181),
    (12, 166, 120),
    (230, 73, 128),
    (90, 105, 220),
]


@dataclass(frozen=True)
class Lane:
    start: Point
    end: Point


@dataclass
class Vehicle:
    vehicle_id: int
    arrival: float
    src_lane: int
    dst_lane: int
    path: list[int]
    schedule: list[float]
    color: Color


@dataclass
class Scene:
    lanes: list[Lane]
    zones: list[Point]
    vehicles: list[Vehicle]
    zone_passing_time: float


class Transform:
    def __init__(self, points: Iterable[Point], size: tuple[int, int], margin: int) -> None:
        pts = list(points)
        if not pts:
            pts = [(0.0, 0.0), (1.0, 1.0)]
        xs = [p[0] for p in pts]
        ys = [p[1] for p in pts]
        min_x, max_x = min(xs), max(xs)
        min_y, max_y = min(ys), max(ys)
        if math.isclose(min_x, max_x):
            min_x -= 1.0
            max_x += 1.0
        if math.isclose(min_y, max_y):
            min_y -= 1.0
            max_y += 1.0

        drawable_w = max(1, size[0] - margin * 2)
        drawable_h = max(1, size[1] - margin * 2)
        self.scale = min(drawable_w / (max_x - min_x), drawable_h / (max_y - min_y))
        world_w = (max_x - min_x) * self.scale
        world_h = (max_y - min_y) * self.scale
        self.offset_x = margin + (drawable_w - world_w) / 2
        self.offset_y = margin + (drawable_h - world_h) / 2
        self.min_x = min_x
        self.max_y = max_y

    def world_to_screen(self, point: Point) -> tuple[int, int]:
        x = self.offset_x + (point[0] - self.min_x) * self.scale
        y = self.offset_y + (self.max_y - point[1]) * self.scale
        return round(x), round(y)


def load_json(path: Path) -> dict:
    with path.open(encoding="utf-8") as f:
        return json.load(f)


def as_point(raw: list[int | float], label: str) -> Point:
    if not isinstance(raw, list) or len(raw) != 2:
        raise ValueError(f"{label} must be a two-number coordinate")
    return float(raw[0]), float(raw[1])


def load_scene(intersection_path: Path, schedule_path: Path) -> Scene:
    intersection = load_json(intersection_path)
    schedule = load_json(schedule_path)

    lanes = [
        Lane(as_point(item["start"], f"lanes[{i}].start"), as_point(item["end"], f"lanes[{i}].end"))
        for i, item in enumerate(intersection.get("lanes", []))
    ]
    zones = [as_point(item, f"zones[{i}]") for i, item in enumerate(schedule.get("zones", []))]
    zone_passing_time = float(schedule.get("zone_passing_time", 10))

    vehicles: list[Vehicle] = []
    for i, item in enumerate(schedule.get("vehicles", [])):
        path = [int(x) for x in item.get("path", [])]
        raw_schedule = item.get("schedule")
        if raw_schedule is None:
            raw_schedule = infer_schedule(float(item.get("arrival", 0)), path, zone_passing_time)
        vehicle = Vehicle(
            vehicle_id=i,
            arrival=float(item.get("arrival", 0)),
            src_lane=int(item["src_lane"]),
            dst_lane=int(item["dst_lane"]),
            path=path,
            schedule=[float(x) for x in raw_schedule],
            color=CAR_COLORS[i % len(CAR_COLORS)],
        )
        validate_vehicle(vehicle, lanes, zones)
        vehicles.append(vehicle)

    return Scene(lanes=lanes, zones=zones, vehicles=vehicles, zone_passing_time=zone_passing_time)


def infer_schedule(arrival: float, path: list[int], zone_passing_time: float) -> list[float]:
    return [arrival + i * zone_passing_time for i in range(len(path))]


def validate_vehicle(vehicle: Vehicle, lanes: list[Lane], zones: list[Point]) -> None:
    if vehicle.src_lane < 0 or vehicle.src_lane >= len(lanes):
        raise ValueError(f"vehicle {vehicle.vehicle_id}: src_lane is out of range")
    if vehicle.dst_lane < 0 or vehicle.dst_lane >= len(lanes):
        raise ValueError(f"vehicle {vehicle.vehicle_id}: dst_lane is out of range")
    if len(vehicle.path) != len(vehicle.schedule):
        raise ValueError(f"vehicle {vehicle.vehicle_id}: path and schedule lengths differ")
    for zone_id in vehicle.path:
        if zone_id < 0 or zone_id >= len(zones):
            raise ValueError(f"vehicle {vehicle.vehicle_id}: zone {zone_id} is out of range")


def vehicle_timeline(scene: Scene, vehicle: Vehicle) -> list[tuple[float, Point]]:
    src = scene.lanes[vehicle.src_lane].start
    dst = scene.lanes[vehicle.dst_lane].end
    if not vehicle.path:
        return [(vehicle.arrival, src), (vehicle.arrival + scene.zone_passing_time, dst)]

    points = [src] + [scene.zones[zone_id] for zone_id in vehicle.path] + [dst]
    first = max(vehicle.arrival, vehicle.schedule[0] - 0.5 * scene.zone_passing_time)
    last = vehicle.schedule[-1] + 1.5 * scene.zone_passing_time
    times = [first] + [t + 0.5 * scene.zone_passing_time for t in vehicle.schedule] + [last]

    for i in range(1, len(times)):
        if times[i] <= times[i - 1]:
            times[i] = times[i - 1] + 0.1
    return list(zip(times, points))


def interpolate(timeline: list[tuple[float, Point]], current_time: float) -> Point | None:
    if current_time < timeline[0][0] or current_time > timeline[-1][0]:
        return None
    for (t0, p0), (t1, p1) in zip(timeline, timeline[1:]):
        if current_time <= t1:
            ratio = 0.0 if math.isclose(t0, t1) else (current_time - t0) / (t1 - t0)
            return p0[0] + (p1[0] - p0[0]) * ratio, p0[1] + (p1[1] - p0[1]) * ratio
    return timeline[-1][1]


def direction_at(timeline: list[tuple[float, Point]], current_time: float) -> float:
    for (t0, p0), (t1, p1) in zip(timeline, timeline[1:]):
        if t0 <= current_time <= t1:
            return math.atan2(p1[1] - p0[1], p1[0] - p0[0])
    p0, p1 = timeline[-2][1], timeline[-1][1]
    return math.atan2(p1[1] - p0[1], p1[0] - p0[0])


def active_zones(scene: Scene, current_time: float) -> set[int]:
    active: set[int] = set()
    for vehicle in scene.vehicles:
        for zone_id, start in zip(vehicle.path, vehicle.schedule):
            if start <= current_time <= start + scene.zone_passing_time:
                active.add(zone_id)
    return active


def draw_arrow(surface: pygame.Surface, start: tuple[int, int], end: tuple[int, int], color: Color) -> None:
    pygame.draw.line(surface, color, start, end, 3)
    dx, dy = end[0] - start[0], end[1] - start[1]
    angle = math.atan2(dy, dx)
    head = 12
    left = (end[0] - head * math.cos(angle - 0.45), end[1] - head * math.sin(angle - 0.45))
    right = (end[0] - head * math.cos(angle + 0.45), end[1] - head * math.sin(angle + 0.45))
    pygame.draw.polygon(surface, color, [end, left, right])


def draw_scene(
    surface: pygame.Surface,
    scene: Scene,
    transform: Transform,
    timelines: dict[int, list[tuple[float, Point]]],
    current_time: float,
    speed: float,
    paused: bool,
    fonts: dict[str, pygame.font.Font],
) -> None:
    surface.fill(BACKGROUND)
    active = active_zones(scene, current_time)

    for lane in scene.lanes:
        start = transform.world_to_screen(lane.start)
        end = transform.world_to_screen(lane.end)
        pygame.draw.line(surface, ROAD_EDGE, start, end, 34)
        pygame.draw.line(surface, ROAD, start, end, 28)
        draw_arrow(surface, start, end, LANE_MARK)

    for vehicle in scene.vehicles:
        timeline = timelines[vehicle.vehicle_id]
        screen_points = [transform.world_to_screen(point) for _, point in timeline]
        if len(screen_points) >= 2:
            pygame.draw.lines(surface, PATH_LINE, False, screen_points, 2)

    for i, zone in enumerate(scene.zones):
        center = transform.world_to_screen(zone)
        fill = ZONE_ACTIVE if i in active else ZONE
        pygame.draw.circle(surface, (95, 83, 61), center, 15)
        pygame.draw.circle(surface, fill, center, 12)
        label = fonts["small"].render(str(i), True, TEXT)
        surface.blit(label, label.get_rect(center=center))

    for vehicle in scene.vehicles:
        timeline = timelines[vehicle.vehicle_id]
        point = interpolate(timeline, current_time)
        if point is None:
            continue
        angle = direction_at(timeline, current_time)
        draw_vehicle(surface, transform.world_to_screen(point), angle, vehicle.color, vehicle.vehicle_id, fonts["tiny"])

    draw_hud(surface, scene, current_time, speed, paused, fonts)


def draw_vehicle(
    surface: pygame.Surface,
    center: tuple[int, int],
    angle: float,
    color: Color,
    vehicle_id: int,
    font: pygame.font.Font,
) -> None:
    length = 30
    width = 16
    # Convert from world-space angle to the inverted screen y-axis.
    screen_angle = -angle
    forward = (math.cos(screen_angle), math.sin(screen_angle))
    side = (-math.sin(screen_angle), math.cos(screen_angle))
    corners = []
    for fx, sx in [(1, 1), (1, -1), (-1, -1), (-1, 1)]:
        x = center[0] + forward[0] * length * 0.5 * fx + side[0] * width * 0.5 * sx
        y = center[1] + forward[1] * length * 0.5 * fx + side[1] * width * 0.5 * sx
        corners.append((x, y))
    pygame.draw.polygon(surface, (39, 43, 43), corners)
    inner = [
        (center[0] + (x - center[0]) * 0.82, center[1] + (y - center[1]) * 0.82)
        for x, y in corners
    ]
    pygame.draw.polygon(surface, color, inner)
    label = font.render(str(vehicle_id), True, (255, 255, 255))
    surface.blit(label, label.get_rect(center=center))


def draw_hud(
    surface: pygame.Surface,
    scene: Scene,
    current_time: float,
    speed: float,
    paused: bool,
    fonts: dict[str, pygame.font.Font],
) -> None:
    panel = pygame.Rect(18, 18, 312, 86)
    pygame.draw.rect(surface, PANEL, panel, border_radius=8)
    pygame.draw.rect(surface, PANEL_LINE, panel, width=1, border_radius=8)
    title = fonts["regular"].render("Pygame Intersection Visualizer", True, TEXT)
    status = "paused" if paused else "playing"
    meta = fonts["small"].render(
        f"t={current_time:6.1f} ticks   {speed:g} ticks/s   {status}",
        True,
        MUTED_TEXT,
    )
    count = fonts["small"].render(
        f"{len(scene.lanes)} lanes   {len(scene.zones)} zones   {len(scene.vehicles)} vehicles",
        True,
        MUTED_TEXT,
    )
    surface.blit(title, (panel.x + 14, panel.y + 12))
    surface.blit(meta, (panel.x + 14, panel.y + 40))
    surface.blit(count, (panel.x + 14, panel.y + 62))


def scene_bounds(scene: Scene) -> list[Point]:
    points: list[Point] = []
    for lane in scene.lanes:
        points.extend([lane.start, lane.end])
    points.extend(scene.zones)
    return points


def time_bounds(scene: Scene, timelines: dict[int, list[tuple[float, Point]]]) -> tuple[float, float]:
    if not timelines:
        return 0.0, 1.0
    starts = [timeline[0][0] for timeline in timelines.values()]
    ends = [timeline[-1][0] for timeline in timelines.values()]
    return min(starts), max(ends)


def run(args: argparse.Namespace) -> None:
    global pygame
    try:
        import pygame as pygame_module
    except ModuleNotFoundError as exc:
        raise SystemExit(
            "pygame is not installed. Run: pip install -r src/visualizer/requirements.txt"
        ) from exc
    pygame = pygame_module

    scene = load_scene(Path(args.intersection), Path(args.schedule))
    timelines = {v.vehicle_id: vehicle_timeline(scene, v) for v in scene.vehicles}
    start_time, end_time = time_bounds(scene, timelines)
    current_time = start_time
    paused = False

    pygame.init()
    try:
        screen = pygame.display.set_mode((args.width, args.height))
        pygame.display.set_caption("Intersection Visualizer")
        fonts = {
            "regular": pygame.font.SysFont("arial", 20, bold=True),
            "small": pygame.font.SysFont("arial", 15),
            "tiny": pygame.font.SysFont("arial", 11, bold=True),
        }
        clock = pygame.time.Clock()
        transform = Transform(scene_bounds(scene), (args.width, args.height), args.margin)

        running = True
        while running:
            dt = clock.tick(args.fps) / 1000.0
            for event in pygame.event.get():
                if event.type == pygame.QUIT:
                    running = False
                elif event.type == pygame.KEYDOWN:
                    if event.key in (pygame.K_ESCAPE, pygame.K_q):
                        running = False
                    elif event.key == pygame.K_SPACE:
                        paused = not paused
                    elif event.key == pygame.K_r:
                        current_time = start_time
                    elif event.key in (pygame.K_EQUALS, pygame.K_PLUS):
                        args.speed *= 1.25
                    elif event.key in (pygame.K_MINUS, pygame.K_UNDERSCORE):
                        args.speed = max(0.1, args.speed / 1.25)

            if not paused:
                current_time += dt * args.speed
                if current_time > end_time:
                    current_time = start_time if args.loop else end_time
                    paused = not args.loop

            draw_scene(screen, scene, transform, timelines, current_time, args.speed, paused, fonts)
            pygame.display.flip()
    finally:
        pygame.quit()


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(description="Animate Manager schedule output with Pygame.")
    parser.add_argument("intersection", help="intersection config JSON, e.g. config/crossroad.json")
    parser.add_argument("schedule", help="Manager output JSON containing vehicles with schedules")
    parser.add_argument("--width", type=int, default=960, help="window width")
    parser.add_argument("--height", type=int, default=720, help="window height")
    parser.add_argument("--margin", type=int, default=90, help="map margin in pixels")
    parser.add_argument("--fps", type=int, default=60, help="target frames per second")
    parser.add_argument("--speed", type=float, default=8.0, help="simulation ticks per real second")
    parser.add_argument("--loop", action="store_true", help="restart automatically after the final vehicle exits")
    return parser.parse_args()


if __name__ == "__main__":
    run(parse_args())

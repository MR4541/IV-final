import argparse
import json
import random
from itertools import combinations, product
from collections import namedtuple

Line = namedtuple('Line', ['start', 'end'])

def intersect(line1: Line, line2: Line) -> list[int] | None:
    x, y = zip(*([p for p in line1] + [p for p in line2]))
    diffs = lambda a: [[a[i] - a[j] for j in range(len(a))]
                       for i in range(len(a))]
    dx = diffs(x)
    dy = diffs(y)
    det = lambda m: m[0][0] * m[1][1] - m[0][1] * m[1][0]
    f = lambda a, b, c, d: det([[dx[a][b], dx[c][d]], [dy[a][b], dy[c][d]]])
    try:
        t = f(0, 2, 2, 3) / f(0, 1, 2, 3)
    except ZeroDivisionError:
        return None
    px = round(x[0] + t * dx[1][0], 1)
    py = round(y[0] + t * dy[1][0], 1)
    return [px, py]

def to_zones(lanes: list[Line]) -> tuple[list[list[int]], list[list[int]]]:
    zones = []
    roads = [[] for _ in lanes]
    for i, j in combinations(range(len(lanes)), 2):
        p = intersect(lanes[i], lanes[j])
        if not p:
            continue
        if p not in zones:
            zones.append(p)
        k = zones.index(p)
        if k not in roads[i]:
            roads[i].append(k)
        if k not in roads[j]:
            roads[j].append(k)
    for i in range(len(lanes)):
        diffs = [abs(lanes[i].end[j] - lanes[i].start[j]) for j in [0, 1]]
        k = diffs.index(max(diffs))
        length = lanes[i].end[k] - lanes[i].start[k]
        roads[i].sort(key=lambda x: (zones[x][k] - lanes[i].start[k]) / length)
    return zones, roads

def route(src: list[int], dst: list[int]) -> list[int]:
    s = set(src) & set(dst)
    if not s:
        return []
    x = list(s)[0]
    a = src.index(x)
    b = dst.index(x)
    return src[:a] + dst[b:]

def get_routes(roads: list[list[int]]) -> list[list[int]]:
    routes = []
    for i, j in product(range(len(roads)), range(len(roads))):
        x = roads[i] if i == j else route(roads[i], roads[j])
        if not x:
            continue
        routes.append((i, j, x))
    return routes

def main(args) -> None:
    with open(args.filename) as f:
        data = json.load(f)
    to_line = lambda x: Line(x['start'], x['end'])
    lanes = list(map(to_line, data['lanes']))
    zones, roads = to_zones(lanes)
    routes = get_routes(roads)
    n = int(args.rate * args.time)
    arrivals = random.choices(range(args.time), k=n)
    paths = random.choices(routes, k=n)
    scenario = {
        'zone_passing_time': args.zone_passing_time,
        'edge_waiting_time': args.edge_waiting_time,
        'zones': zones,
        'vehicles': [{
            'arrival': a,
            'src_lane': p[0],
            'dst_lane': p[1],
            'path': p[2]
        } for a, p in zip(arrivals, paths)]
    }
    with open(args.output, 'w') as f:
        json.dump(scenario, f)

if __name__ == '__main__':
    parser = argparse.ArgumentParser()
    parser.add_argument(
        'filename',
        help='the input lane configuration JSON file'
    )
    parser.add_argument(
        '-o',
        '--output',
        required=True,
        help='the output file name'
    )
    parser.add_argument(
        '-r',
        '--rate',
        type=float,
        required=True,
        help='the Poisson process rate (mean number of vehicles per tick)'
    )
    parser.add_argument(
        '-t',
        '--time',
        type=int,
        required=True,
        help='the earliest arrival time of the last vehicle (in ticks)'
    )
    parser.add_argument(
        '-p',
        '--zone-passing-time',
        type=int,
        default=10,
        help='zone passing time (in ticks)'
    )
    parser.add_argument(
        '-w',
        '--edge-waiting-time',
        type=int,
        nargs=3,
        default=[1, 2, 2],
        help='edge waiting time for type 1, 2, and 3 edges (in ticks)'
    )
    main(parser.parse_args())


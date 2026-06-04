# Data Exchange Formats between Components

論文裡面設定的 zone_passing_time 是 1s、type-1/2/3 edge 的 waiting_time 是 0.1/0.2/0.2s。
為了方便就用 0.1s 當 1 time unit，這樣所有跟時間有關的值都會是整數好欸。

## Intersections

An *intersection* is defined using a JSON file
containing the 2D coordinates of the start and the end of each lane,
assuming that all lanes are straight lines.

```json
{
  "lanes": [
    {
      "start": [x, y],
      "end": [x, y]
    },
    {
      "start": [x, y],
      "end": [x, y]
    },
    ...
  ]
}
```

Coodinate components `x` and `y` are integers.
The lanes may be referenced by their ID,
which is a non-negative integer implicitly assigned
by the 0-based order of appearance in the `lanes` array,
i.e., the first lane in `lanes` has ID `0` and the second one has ID `1`, etc.

## Scenarios

A *scenario* is defined using a JSON file
containing trajectories of multiple vehicles.
A *trajectory* is defined using a list of *conflict zones*,
(intersection between lanes)
that the vehicle should pass through.
These conflict zones are included in the scenario configuration
by their 2D coordinates.

```json
{
  "zone_passing_time": value,
  "edge_waiting_time": [type1, type2, type3],
  "zones": [[x, y], [x, y], ...],
  "vehicles": [
    {
      "arrival": value,
      "src_lane": lane_id,
      "dst_lane": lane_id,
      "path": [zone_id, zone_id, ..., zone_id]
    },
    ...
  ]
}
```

`zone_id` are implicitly assigned by the 0-based order of appearance
in the `zones` array.
An example is as follows:

```json
{
  "zone_passing_time": 10,
  "edge_waiting_time": [1, 2, 2],
  "zones": [[2.0, 2.0], [1.0, 2.0], [2.0, 1.0], [1.0, 1.0]],
  "vehicles": [
      {"arrival": 11, "src_lane": 2, "dst_lane": 2, "path": [2, 3]},
      {"arrival": 1, "src_lane": 3, "dst_lane": 2, "path": [3]},
      {"arrival": 6, "src_lane": 1, "dst_lane": 1, "path": [0, 2]},
      {"arrival": 12, "src_lane": 2, "dst_lane": 3, "path": [2, 3, 1]},
      {"arrival": 21, "src_lane": 1, "dst_lane": 0, "path": [0]}
  ]
}
```

## Intersection Manager

The task of an intersection manager is to take the JSON file of a scenario
and assign the time of entrance of each zone for each vehicle.
Using the example from [Scenarios](#scenarios),
the output JSON file from the manager may look like this:

```json
{
  "zone_passing_time": 10,
  "edge_waiting_time": [1, 2, 2],
  "zones": [[2.0, 2.0], [1.0, 2.0], [2.0, 1.0], [1.0, 1.0]],
  "vehicles": [
      {"arrival": 11, "src_lane": 2, "dst_lane": 2, "path": [2, 3], "schedule": [15, 20]},
      {"arrival": 1, "src_lane": 3, "dst_lane": 2, "path": [3], "schedule": [5]},
      {"arrival": 6, "src_lane": 1, "dst_lane": 1, "path": [0, 2], "schedule": [8, 13]},
      {"arrival": 12, "src_lane": 2, "dst_lane": 3, "path": [2, 3, 1], "schedule": [23, 34, 44]},
      {"arrival": 21, "src_lane": 1, "dst_lane": 0, "path": [0], "schedule": [32]}
  ]
}
```

## Visualizer

The visualizer should take in both an intersection configuration file
as defined in [Intersections](#intersections)
and the output JSON file of the manager
and display an animation showing the vehicles crossing the intersection
as scheduled.
The two input files, as previously defined, should contain all the information
necessary for creating the animation.


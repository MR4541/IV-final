論文裡面設定的 zone_passing_time 是 1s、type-1/2/3 edge 的 waiting_time 是 0.1/0.2/0.2s。
為了方便就用 0.1s 當 1 time unit，這樣所有跟時間有關的值都會是整數好欸。

## intersection.json

**待定**

Manager 需要：
- 一個整數表示「通過」一個 conflict zone 的時間 (vertex passing time)，論文是設 1s = 10 time units
- 三個整數表示 type-1/2/3 edge 的 edge waiting time，論文設 0.1/0.2/0.2s = 1/2/2 time units
由 Generator 把這些值從 intersection.json 中抓出來一起放到 JSON 裡面 Manager

## input of Manager

Schema:
```json
{
  "n_vehicles": "Number",
  "vehicles": [
    {
      "vid": "Number",
      "src_lane": "Number (source lane)",
      "dest_lane": "Number (destination lane)",
      "path": ["Number (經過的conflict zones)"],
      "arrival": "Number (到達路口時間)"
    }
  ],
  "zone_passing_time": "Number (從intersection.json讀)",
  "edge_waiting_time": ["Number (從intersection.json讀，type1,2,3edge各一個)"],
  "n_zones": "Number (從intersection.json讀)"
}
```

Example:
```json
{
    "n_vehicles": 3,
    "vehicles": [
        {"vid": 1, "src_lane": 1, "dest_lane": 2, "path": [1, 2, 3], "arrival": 10},
        {"vid": 2, "src_lane": 1, "dest_lane": 3, "path": [1, 2], "arrival": 20},
        {"vid": 3, "src_lane": 2, "dest_lane": 4, "path": [3, 2], "arrival": 0}
    ],
    "zone_passing_time": 10,
    "edge_waiting_time": [1, 2, 2],
    "n_zones": 4
}
```

## output of Manager

Schema:
```json
{
  "n_vehicles": "Number",
  "vehicles": [
    {
      "vid": "Number",
      "src_lane": "Number (起始車道)",
      "dest_lane": "Number (目標車道)",
      "path": ["Number (經過的conflict zones)"],
      "schedule": ["Number (排程後enter每個zone的時間)"],
      "arrival": "Number (到達路口時間)"
    }
  ]
}
```

Example:
```json
{
  "n_vehicles": 3,
  "vehicles": [
    {
      "arrival": 10,
      "dest_lane": 2,
      "path": [1, 2, 3],
      "schedule": [0, 0, 0],
      "src_lane": 1,
      "vid": 1
    },
    {
      "arrival": 20,
      "dest_lane": 3,
      "path": [1, 2],
      "schedule": [0, 0],
      "src_lane": 1,
      "vid": 2
    },
    {
      "arrival": 0,
      "dest_lane": 4,
      "path": [3, 2],
      "schedule": [0, 0],
      "src_lane": 2,
      "vid": 3
    }
  ]
}
```

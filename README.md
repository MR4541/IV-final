# Introduction to Intelligent Vehicles Final Project

## Project Structure

```
IV-final/
├── .gitignore
├── README.md
├── docs/
│   ├── api_spec.md             # schema of JSON files
│   └── ...                     # other documents...
├── config/                     # each JSON represents a type of intersection
│   ├── intersection_1.json
│   ├── intersection_2.json
│   └── ...
├── data/                       # dynamic files
│   └── ...                     # output from Generator, Manager, ...
├── src/                        # source codes
│   ├── generator/              # Traffic Generator (Python)
│   │   ├── requirements.txt
│   │   └── ...
│   ├── manager/                # Intersection Manager (C++)
│   │   ├── src/
│   │   ├── include/
│   │   └── ...
│   └── visualizer/             # Pygame Visualizer & Plotter (Python)
│       ├── requirements.txt
│       └── ...
└── scripts/                    # (if any)
    └── run_experiments.sh      # something like this
```

## Usage

### Generator

### Manager

compile:
```bash
cd /src/manager
make clean && make # it requires 15 - 30 sec
bin/manager <inputFile> <outputFile> <Algo=1|2|3|4>
```

- `inputFile`: a JSON of a scenario
- `outputFile`: a JSON including schedule
- `Algo`: the algorithm to use
1 - 3D-intersecton
2 - FCFS
3 - Priority
4 - Graph-based


### Visualizer

## Misc

- 有些ㄧ開始是空的資料夾，我在底下放 .gitkeep 讓 git 可以 track 到目錄

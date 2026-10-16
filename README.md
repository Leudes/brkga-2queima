# BRKGA for the Graph 2-Burning Problem

Research implementation of a **Biased Random-Key Genetic Algorithm (BRKGA)** for the **graph 2-burning problem**, developed as part of my undergraduate thesis in Computer Science at the Federal University of Ceará (UFC), Quixadá Campus, under the supervision of **Prof. Dr. Atílio Gomes Luiz**.

## About the problem

Graph 2-burning is a discrete diffusion process on a graph. Initially, every vertex is unburned. At each round:

1. every unburned vertex with at least two burned neighbors from the previous round becomes burned; and
2. one still-unburned vertex may be selected as a new burning source.

The objective is to find a sequence of sources that burns the entire graph in as few rounds as possible.

This repository investigates the problem from a **combinatorial optimization** perspective using BRKGA.

## Approach

A BRKGA chromosome is represented by a vector of random keys associated with the graph vertices. The decoder transforms the chromosome into a feasible burning sequence and evaluates it by the number of rounds required to burn the whole graph.

The repository keeps four experimental decoder versions for comparison:

- `2queima` — initial implementation;
- `2queimaV2` — second decoder variant;
- `2queimaV3` — variant using relative-degree information;
- `2queimaV4` — variant using a trigger-based repair heuristic.

The current V4 decoder prioritizes sources encoded by the chromosome and, when necessary, repairs incomplete solutions using vertices that can activate vertices already adjacent to one burned neighbor.

The BRKGA implementation uses the bundled `brkgaAPI` and supports multiple independent trials, elite exchange between populations, stopping by generation count, and stopping by stagnation.

## Repository structure

```text
.
├── brkga-2queima.cpp          # Initial BRKGA executable
├── brkga-2queimaV2.cpp        # BRKGA version 2
├── brkga-2queimaV3.cpp        # BRKGA version 3
├── brkga-2queimaV4.cpp        # BRKGA version 4
├── Decoder2Queima*.cpp/.h     # Problem-specific BRKGA decoders
├── Graph.cpp/.h               # Graph representation and parser
├── SimulacaoQueima.h          # 2-burning simulation
├── brkgaAPI/                  # BRKGA implementation
├── instancias/                # Graph instances
├── benchmark/                 # Experimental data/results
├── gerar_grafos.cpp           # Graph-instance generation utility
├── results*.csv               # Experimental outputs
└── Makefile
```

## Requirements

- C++17-compatible compiler
- GNU Make
- OpenMP support

The current Makefile uses `g++` with optimization and OpenMP enabled.

## Building

Clone the repository and compile all four experimental versions:

```bash
git clone https://github.com/Leudes/brkga-2queima.git
cd brkga-2queima
make
```

This generates:

```text
2queima
2queimaV2
2queimaV3
2queimaV4
```

To remove generated objects and executables:

```bash
make clean
```

## Input format

Graph instances are read as an **edge list**. Each non-comment line must contain two integer vertex identifiers:

```text
0 1
1 2
2 3
0 3
```

Lines beginning with `#` or `%` are ignored, as are self-loops. The graph is treated as undirected and duplicate edges are removed.

For the intended experiments, vertex identifiers should be contiguous and start at `0`.

## Running

The most recent BRKGA variant can be executed as:

```bash
./2queimaV4 <graph_file> [options]
```

Example:

```bash
./2queimaV4 instancias/example.txt --trials 15 --output results4.csv
```

Available V4 parameters:

```text
--p VALUE
--pe VALUE
--pm VALUE
--rhoe VALUE
--K VALUE
--MAXT VALUE
--X_INTVL VALUE
--X_NUMBER VALUE
--MAX_GENS VALUE
--MAX_STAGT VALUE
--trials VALUE
--output FILE
```

The executable records graph information, best fitness, elapsed time, and the best burning-source sequence found.

## Experimental work

The implementation has been used in preliminary experiments with different graph families, including:

- dense random graphs;
- sparse random graphs;
- grid graphs;
- paths;
- cycles.

The experimental workflow compares heuristic solutions with exact values or theoretical bounds whenever those are available.

## Related implementation

A separate implementation using **General Variable Neighborhood Search (GVNS)** is available at:

https://github.com/Leudes/gvns-2queima

## Academic context

This repository is part of the undergraduate thesis:

> **Meta-heurísticas para o Problema da 2-Queima em Grafos**

**Author:** Francisco Leudes Bezerra Neto  
**Institution:** Federal University of Ceará (UFC) — Quixadá Campus  
**Advisor:** Prof. Dr. Atílio Gomes Luiz

The research investigates BRKGA and GVNS as metaheuristic approaches for the graph 2-burning problem, with experimental comparison against known values, theoretical bounds, and exact methods when available.

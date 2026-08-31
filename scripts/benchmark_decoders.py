#!/usr/bin/env python3
"""Benchmark reproduzivel dos quatro decodificadores da 2-queima."""

from __future__ import annotations

import argparse
import csv
import json
import statistics
import subprocess
import tempfile
from collections import defaultdict, deque
from dataclasses import dataclass
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]

VERSIONS = {
    "V1_cromossomo": ROOT / "2queima",
    "V2_grau_estatico": ROOT / "2queimaV2",
    "V3_grau_relativo": ROOT / "2queimaV3",
    "V4_gatilho": ROOT / "2queimaV4",
}

INSTANCES = [
    ROOT / "benchmark/base01/cycle1-order25.txt",
    ROOT / "instancias/random_denso_n50_p30.txt",
    ROOT / "benchmark/base01/steam3.txt",
    ROOT / "benchmark/base01/cubic_100.txt",
    ROOT / "instancias/grade_10x10.txt",
    ROOT / "instancias/ciclo_n100.txt",
    ROOT / "benchmark/base01/dwt__162.txt",
    ROOT / "instancias/random_esparso_n200_d3.txt",
    ROOT / "benchmark/base01/cycle10-order250.txt",
    ROOT / "benchmark/base01/nos7.txt",
]

COMMON_PARAMETERS = {
    "p": "40",
    "pe": "0.20",
    "pm": "0.10",
    "rhoe": "0.70",
    "K": "2",
    "MAXT": "2",
    "X_INTVL": "20",
    "X_NUMBER": "1",
    "MAX_GENS": "60",
    "MAX_STAGT": "30",
}


@dataclass
class Graph:
    adjacency: list[list[int]]
    edges: int


def read_graph(path: Path) -> Graph:
    edge_set: set[tuple[int, int]] = set()
    maximum = -1
    for line in path.read_text(encoding="utf-8").splitlines():
        if not line or line[0] in "#%":
            continue
        fields = line.split()
        if len(fields) < 2:
            continue
        try:
            u, v = int(fields[0]), int(fields[1])
        except ValueError:
            continue
        if u == v:
            continue
        maximum = max(maximum, u, v)
        edge_set.add((min(u, v), max(u, v)))
    adjacency = [[] for _ in range(maximum + 1)]
    for u, v in edge_set:
        adjacency[u].append(v)
        adjacency[v].append(u)
    return Graph(adjacency, len(edge_set))


def sequence_fitness(graph: Graph, sequence: list[int]) -> int:
    n = len(graph.adjacency)
    burned = [False] * n
    burned_neighbors = [0] * n
    spread_queue: list[int] = []
    next_spread_queue: list[int] = []
    total_burned = 0
    sequence_index = 0
    rounds = 0

    def burn(vertex: int) -> bool:
        nonlocal total_burned
        if burned[vertex]:
            return False
        burned[vertex] = True
        total_burned += 1
        for neighbor in graph.adjacency[vertex]:
            if not burned[neighbor]:
                burned_neighbors[neighbor] += 1
                if burned_neighbors[neighbor] == 2:
                    next_spread_queue.append(neighbor)
        return True

    while total_burned < n:
        rounds += 1
        changed = False
        for vertex in spread_queue:
            changed = burn(vertex) or changed
        spread_queue.clear()
        if total_burned == n:
            break
        if sequence_index < len(sequence):
            selected = sequence[sequence_index]
            sequence_index += 1
            if not 0 <= selected < n or burned[selected]:
                raise ValueError(f"sequencia direta invalida no vertice {selected}")
            changed = burn(selected) or changed
        spread_queue, next_spread_queue = next_spread_queue, spread_queue
        if not changed and not spread_queue:
            # O V1 permite uma sequencia insuficiente e penaliza os vertices
            # restantes somente quando toda a propagacao tambem termina.
            return rounds + (n - total_burned) * 100
    return rounds


def parse_program_output(path: Path) -> list[tuple[int, float, list[int]]]:
    lines = path.read_text(encoding="utf-8").splitlines()
    results: list[tuple[int, float, list[int]]] = []
    index = 1
    while index < len(lines):
        fields = next(csv.reader([lines[index]]))
        if len(fields) != 6:
            raise ValueError(f"linha de resultado inesperada: {lines[index]}")
        fitness = int(float(fields[4]))
        elapsed_us = float(fields[5])
        index += 1
        if index >= len(lines) or not lines[index].startswith("sequence,"):
            raise ValueError("sequencia ausente no resultado")
        inside = lines[index].split("[", 1)[1].split("]", 1)[0]
        sequence = [int(value) for value in inside.split()]
        results.append((fitness, elapsed_us, sequence))
        index += 1
    return results


def run_benchmark(
    trials: int,
    output_dir: Path,
    parameters: dict[str, str],
    file_prefix: str,
) -> list[dict[str, object]]:
    for executable in VERSIONS.values():
        if not executable.is_file():
            raise FileNotFoundError(f"execute make antes do benchmark: {executable}")
    for instance in INSTANCES:
        if not instance.is_file():
            raise FileNotFoundError(instance)

    raw_rows: list[dict[str, object]] = []
    graphs = {instance: read_graph(instance) for instance in INSTANCES}
    with tempfile.TemporaryDirectory(prefix="2queima-benchmark-") as directory:
        temporary = Path(directory)
        total = len(VERSIONS) * len(INSTANCES)
        completed = 0
        for version, executable in VERSIONS.items():
            for instance in INSTANCES:
                result_file = temporary / f"{version}__{instance.stem}.csv"
                command = [str(executable), str(instance)]
                for name, value in parameters.items():
                    command.extend((f"--{name}", value))
                command.extend(("--trials", str(trials), "--output", str(result_file)))
                subprocess.run(command, cwd=ROOT, check=True)
                parsed = parse_program_output(result_file)
                if len(parsed) != trials:
                    raise ValueError(f"{version}/{instance.name}: {len(parsed)} trials")
                graph = graphs[instance]
                for trial, (fitness, elapsed_us, sequence) in enumerate(parsed):
                    independent = sequence_fitness(graph, sequence)
                    if independent != fitness:
                        raise ValueError(
                            f"fitness inconsistente em {version}/{instance.name}/"
                            f"{trial}: programa={fitness}, simulacao={independent}"
                        )
                    raw_rows.append({
                        "version": version,
                        "instance": instance.name,
                        "vertices": len(graph.adjacency),
                        "edges": graph.edges,
                        "trial": trial,
                        "fitness": fitness,
                        "elapsed_us": elapsed_us,
                        "direct_burns": len(sequence),
                    })
                completed += 1
                print(f"[{completed:02d}/{total}] {version}: {instance.name}", flush=True)

    output_dir.mkdir(parents=True, exist_ok=True)
    raw_path = output_dir / f"{file_prefix}_raw.csv"
    with raw_path.open("w", encoding="utf-8", newline="") as stream:
        writer = csv.DictWriter(stream, fieldnames=raw_rows[0].keys())
        writer.writeheader()
        writer.writerows(raw_rows)
    return raw_rows


def summarize(
    rows: list[dict[str, object]],
    output_dir: Path,
    trials: int,
    parameters: dict[str, str],
    file_prefix: str,
) -> None:
    grouped: dict[tuple[str, str], list[dict[str, object]]] = defaultdict(list)
    for row in rows:
        grouped[(str(row["instance"]), str(row["version"]))].append(row)

    summary_rows: list[dict[str, object]] = []
    for (instance, version), values in sorted(grouped.items()):
        fitness = [float(row["fitness"]) for row in values]
        times = [float(row["elapsed_us"]) / 1000.0 for row in values]
        summary_rows.append({
            "instance": instance,
            "vertices": values[0]["vertices"],
            "edges": values[0]["edges"],
            "version": version,
            "fitness_mean": statistics.fmean(fitness),
            "fitness_best": min(fitness),
            "fitness_stdev": statistics.stdev(fitness) if len(fitness) > 1 else 0.0,
            "time_ms_mean": statistics.fmean(times),
            "time_ms_stdev": statistics.stdev(times) if len(times) > 1 else 0.0,
        })

    summary_path = output_dir / f"{file_prefix}_summary.csv"
    with summary_path.open("w", encoding="utf-8", newline="") as stream:
        writer = csv.DictWriter(stream, fieldnames=summary_rows[0].keys())
        writer.writeheader()
        writer.writerows(summary_rows)

    by_version: dict[str, list[dict[str, object]]] = defaultdict(list)
    by_instance: dict[str, list[dict[str, object]]] = defaultdict(list)
    for row in summary_rows:
        by_version[str(row["version"])].append(row)
        by_instance[str(row["instance"])].append(row)
    wins = defaultdict(int)
    for values in by_instance.values():
        best = min(float(row["fitness_mean"]) for row in values)
        for row in values:
            if float(row["fitness_mean"]) == best:
                wins[str(row["version"])] += 1

    aggregates = []
    for version in VERSIONS:
        values = by_version[version]
        relative_gaps = []
        for row in values:
            best_for_instance = min(
                float(candidate["fitness_mean"])
                for candidate in by_instance[str(row["instance"])]
            )
            relative_gaps.append(
                100.0 * (float(row["fitness_mean"]) - best_for_instance) /
                best_for_instance
            )
        aggregates.append({
            "version": version,
            "fitness_mean_across_instances": statistics.fmean(
                float(row["fitness_mean"]) for row in values
            ),
            "mean_gap_to_best_percent": statistics.fmean(relative_gaps),
            "time_ms_mean": statistics.fmean(float(row["time_ms_mean"]) for row in values),
            "wins_or_ties": wins[version],
        })
    (output_dir / f"{file_prefix}_aggregate.json").write_text(
        json.dumps({
            "trials_per_instance": trials,
            "parameters": parameters,
            "aggregate": aggregates,
        }, ensure_ascii=False, indent=2) + "\n",
        encoding="utf-8",
    )

    report = [
        "# Benchmark dos quatro decodificadores",
        "",
        f"Foram executadas {trials} repetições por versão e instância, com sementes "
        f"determinísticas de 0 a {trials - 1}. Todas as versões receberam os mesmos parâmetros.",
        "",
        "Parâmetros: " + ", ".join(
            f"{nome}={valor}" for nome, valor in parameters.items()
        ) + ".",
        "",
        "| Versão | Fitness médio | Gap médio | Tempo médio (ms) | Vitórias/empates |",
        "|---|---:|---:|---:|---:|",
    ]
    for row in aggregates:
        report.append(
            f"| {row['version']} | {row['fitness_mean_across_instances']:.3f} | "
            f"{row['mean_gap_to_best_percent']:.2f}% | "
            f"{row['time_ms_mean']:.3f} | {row['wins_or_ties']} |"
        )
    report.extend((
        "",
        "## Fitness médio por instância",
        "",
        "| Instância | V1 | V2 | V3 | V4 |",
        "|---|---:|---:|---:|---:|",
    ))
    for instance_path in INSTANCES:
        values = {
            str(row["version"]): float(row["fitness_mean"])
            for row in by_instance[instance_path.name]
        }
        best = min(values.values())
        cells = []
        for version in VERSIONS:
            formatted = f"{values[version]:.2f}"
            cells.append(f"**{formatted}**" if values[version] == best else formatted)
        report.append(f"| {instance_path.name} | " + " | ".join(cells) + " |")
    report.extend((
        "",
        "O fitness é o número de rodadas e deve ser minimizado. Os tempos medem somente "
        "a execução do BRKGA, em milissegundos. As sequências foram verificadas por um "
        "simulador independente do limiar 2.",
        "",
    ))
    (output_dir / f"{file_prefix}_report.md").write_text(
        "\n".join(report), encoding="utf-8"
    )


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--trials", type=int, default=3)
    parser.add_argument(
        "--output", type=Path, default=ROOT / "benchmark/results/quick_decoders"
    )
    parser.add_argument("--prefix", default="quick_decoders")
    parser.add_argument("--p", type=int, default=int(COMMON_PARAMETERS["p"]))
    parser.add_argument(
        "--max-gens", type=int, default=int(COMMON_PARAMETERS["MAX_GENS"])
    )
    parser.add_argument(
        "--max-stagt", type=int, default=int(COMMON_PARAMETERS["MAX_STAGT"])
    )
    parser.add_argument(
        "--x-intvl", type=int, default=int(COMMON_PARAMETERS["X_INTVL"])
    )
    parser.add_argument(
        "--x-number", type=int, default=int(COMMON_PARAMETERS["X_NUMBER"])
    )
    args = parser.parse_args()
    if args.trials <= 0:
        raise SystemExit("--trials deve ser positivo")
    if min(args.p, args.max_gens, args.max_stagt, args.x_intvl) <= 0:
        raise SystemExit("os parâmetros inteiros devem ser positivos")
    if args.x_number < 0:
        raise SystemExit("--x-number não pode ser negativo")

    parameters = COMMON_PARAMETERS.copy()
    parameters.update({
        "p": str(args.p),
        "X_INTVL": str(args.x_intvl),
        "X_NUMBER": str(args.x_number),
        "MAX_GENS": str(args.max_gens),
        "MAX_STAGT": str(args.max_stagt),
    })

    rows = run_benchmark(args.trials, args.output, parameters, args.prefix)
    summarize(
        rows,
        args.output,
        args.trials,
        parameters,
        args.prefix,
    )
    print(f"Resultados: {args.output}")


if __name__ == "__main__":
    main()

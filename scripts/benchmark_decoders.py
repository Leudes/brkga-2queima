#!/usr/bin/env python3
"""Benchmark reproduzivel dos decodificadores da 2-queima."""

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
    "V3_reparo_grau_relativo": ROOT / "2queimaV3",
    "V4_reparo_gatilho": ROOT / "2queimaV4",
    "V5_reparo_combinado": ROOT / "2queimaV5",
    "V6_permutacao_pura": ROOT / "2queimaV6",
}

DEFAULT_VERSIONS = [
    "V1_cromossomo",
    "V2_grau_estatico",
    "V3_reparo_grau_relativo",
    "V4_reparo_gatilho",
    "V5_reparo_combinado",
    "V6_permutacao_pura",
]

PASTA_BASE_FINAL = ROOT / "graph_dataset/splits/final"

INSTANCES = [
    PASTA_BASE_FINAL / "dimacs10__karate.txt",
    PASTA_BASE_FINAL / "harwell_boeing__steam3.txt",
    PASTA_BASE_FINAL / "random__gilbert_n200_d3_r1.txt",
    PASTA_BASE_FINAL / "harwell_boeing__mbeaflw.txt",
    PASTA_BASE_FINAL / "snap__as19990101.txt",
    PASTA_BASE_FINAL / "random__gilbert_n1000_d12_r1.txt",
    PASTA_BASE_FINAL / "harwell_boeing__lshp1270.txt",
    PASTA_BASE_FINAL / "random__gilbert_n2000_d24_r2.txt",
    PASTA_BASE_FINAL / "harwell_boeing__cegb3024.txt",
    PASTA_BASE_FINAL / "snap__as19980824.txt",
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

# Parametros encontrados pelo irace. A V6 ainda nao possui calibracao
# propria e, por isso, usa provisoriamente a configuracao da V2.
CALIBRATED_PARAMETERS = {
    "V1_cromossomo": {
        "p": "578", "pe": "0.2788", "pm": "0.0743",
        "rhoe": "0.6237", "K": "2", "MAXT": "4",
        "X_INTVL": "125", "X_NUMBER": "2",
        "MAX_GENS": "583", "MAX_STAGT": "477",
    },
    "V2_grau_estatico": {
        "p": "518", "pe": "0.2463", "pm": "0.2171",
        "rhoe": "0.6522", "K": "2", "MAXT": "4",
        "X_INTVL": "91", "X_NUMBER": "2",
        "MAX_GENS": "651", "MAX_STAGT": "451",
    },
    "V3_reparo_grau_relativo": {
        "p": "572", "pe": "0.2485", "pm": "0.1638",
        "rhoe": "0.6334", "K": "2", "MAXT": "4",
        "X_INTVL": "86", "X_NUMBER": "2",
        "MAX_GENS": "344", "MAX_STAGT": "102",
    },
    "V4_reparo_gatilho": {
        "p": "407", "pe": "0.2899", "pm": "0.0692",
        "rhoe": "0.6514", "K": "2", "MAXT": "4",
        "X_INTVL": "137", "X_NUMBER": "2",
        "MAX_GENS": "762", "MAX_STAGT": "411",
    },
    "V5_reparo_combinado": {
        "p": "554", "pe": "0.2689", "pm": "0.2702",
        "rhoe": "0.6252", "K": "2", "MAXT": "4",
        "X_INTVL": "37", "X_NUMBER": "2",
        "MAX_GENS": "652", "MAX_STAGT": "486",
    },
    "V6_permutacao_pura": {
        "p": "518", "pe": "0.2463", "pm": "0.2171",
        "rhoe": "0.6522", "K": "2", "MAXT": "4",
        "X_INTVL": "91", "X_NUMBER": "2",
        "MAX_GENS": "651", "MAX_STAGT": "451",
    },
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
    parameters_by_version: dict[str, dict[str, str]],
    file_prefix: str,
    versions: dict[str, Path],
) -> list[dict[str, object]]:
    for executable in versions.values():
        if not executable.is_file():
            raise FileNotFoundError(f"execute make antes do benchmark: {executable}")
    for instance in INSTANCES:
        if not instance.is_file():
            raise FileNotFoundError(instance)

    raw_rows: list[dict[str, object]] = []
    graphs = {instance: read_graph(instance) for instance in INSTANCES}
    with tempfile.TemporaryDirectory(prefix="2queima-benchmark-") as directory:
        temporary = Path(directory)
        total = len(versions) * len(INSTANCES)
        completed = 0
        for version, executable in versions.items():
            parameters = parameters_by_version[version]
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
        writer = csv.DictWriter(
            stream,
            fieldnames=raw_rows[0].keys(),
            lineterminator="\n",
        )
        writer.writeheader()
        writer.writerows(raw_rows)
    return raw_rows


def summarize(
    rows: list[dict[str, object]],
    output_dir: Path,
    trials: int,
    parameters_by_version: dict[str, dict[str, str]],
    file_prefix: str,
    versions: dict[str, Path],
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
        writer = csv.DictWriter(
            stream,
            fieldnames=summary_rows[0].keys(),
            lineterminator="\n",
        )
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
    for version in versions:
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
            "parameters_by_version": parameters_by_version,
            "aggregate": aggregates,
        }, ensure_ascii=False, indent=2) + "\n",
        encoding="utf-8",
    )

    distinct_parameters = {
        tuple(parameters.items())
        for parameters in parameters_by_version.values()
    }
    same_parameters = len(distinct_parameters) == 1

    report = [
        f"# Benchmark de {len(versions)} decodificadores",
        "",
        f"Foram executadas {trials} repetições por versão e instância, com sementes "
        f"determinísticas de 0 a {trials - 1}.",
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

    if same_parameters:
        parameters = next(iter(parameters_by_version.values()))
        report[4:4] = [
            "Todas as versões receberam os mesmos parâmetros: " + ", ".join(
                f"{name}={value}" for name, value in parameters.items()
            ) + ".",
            "",
        ]
    else:
        report.extend((
            "",
            "## Parâmetros calibrados",
            "",
            "| Versão | p | pe | pm | rhoe | K | MAXT | X_INTVL | X_NUMBER | MAX_GENS | MAX_STAGT |",
            "|---|---:|---:|---:|---:|---:|---:|---:|---:|---:|---:|",
        ))
        for version in versions:
            parameters = parameters_by_version[version]
            report.append(
                f"| {version} | " + " | ".join(
                    parameters[name]
                    for name in (
                        "p", "pe", "pm", "rhoe", "K", "MAXT",
                        "X_INTVL", "X_NUMBER", "MAX_GENS", "MAX_STAGT",
                    )
                ) + " |"
            )

    report.extend((
        "",
        "## Fitness médio por instância",
        "",
        "| Instância | " + " | ".join(versions) + " |",
        "|---|" + "---:|" * len(versions),
    ))
    for instance_path in INSTANCES:
        values = {
            str(row["version"]): float(row["fitness_mean"])
            for row in by_instance[instance_path.name]
        }
        best = min(values.values())
        cells = []
        for version in versions:
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
        "--output", type=Path, default=ROOT / "tests/resultados/quick_decoders"
    )
    parser.add_argument("--prefix", default="quick_decoders")
    parser.add_argument(
        "--versions",
        nargs="+",
        choices=VERSIONS,
        default=DEFAULT_VERSIONS,
        help="versões que participarão do benchmark",
    )
    parser.add_argument(
        "--calibrated",
        action="store_true",
        help="usa a configuracao calibrada pelo irace de cada versao",
    )
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

    selected_versions = {
        name: VERSIONS[name]
        for name in args.versions
    }

    if args.calibrated:
        parameters_by_version = {
            name: CALIBRATED_PARAMETERS[name].copy()
            for name in args.versions
        }
    else:
        parameters = COMMON_PARAMETERS.copy()
        parameters.update({
            "p": str(args.p),
            "X_INTVL": str(args.x_intvl),
            "X_NUMBER": str(args.x_number),
            "MAX_GENS": str(args.max_gens),
            "MAX_STAGT": str(args.max_stagt),
        })
        parameters_by_version = {
            name: parameters.copy()
            for name in args.versions
        }

    rows = run_benchmark(
        args.trials,
        args.output,
        parameters_by_version,
        args.prefix,
        selected_versions,
    )
    summarize(
        rows,
        args.output,
        args.trials,
        parameters_by_version,
        args.prefix,
        selected_versions,
    )
    print(f"Resultados: {args.output}")


if __name__ == "__main__":
    main()

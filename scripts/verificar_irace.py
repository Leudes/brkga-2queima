#!/usr/bin/env python3
"""Verifica os cinco target-runners sem iniciar a calibracao do irace."""

from __future__ import annotations

import argparse
import math
import subprocess
import time
from pathlib import Path


RAIZ = Path(__file__).resolve().parents[1]
PASTA_INSTANCIAS = RAIZ / "graph_dataset/splits/irace"

RUNNERS = {
    "v1": RAIZ / "tuning/target-runner-v1",
    "v2": RAIZ / "tuning/target-runner-v2",
    "v3": RAIZ / "tuning/target-runner-v3",
    "v4": RAIZ / "tuning/target-runner-v4",
    "v5": RAIZ / "tuning/target-runner-v5",
}

PARAMETROS_RAPIDOS = [
    "--p", "10",
    "--pe", "0.20",
    "--pm", "0.10",
    "--rhoe", "0.70",
    "--MAX_GENS", "2",
    "--MAX_STAGT", "1",
    "--X_INTVL", "1",
]


def verificar_runner(
    nome: str,
    runner: Path,
    instancias: list[Path],
    tempo_limite: float,
) -> None:
    tempos: list[float] = []

    for indice, instancia in enumerate(instancias, start=1):
        comando = [
            str(runner),
            "1",
            str(indice),
            str(1000 + indice),
            str(instancia),
            *PARAMETROS_RAPIDOS,
        ]

        inicio = time.perf_counter()
        resultado = subprocess.run(
            comando,
            text=True,
            capture_output=True,
            timeout=tempo_limite,
            check=False,
        )
        tempos.append(time.perf_counter() - inicio)

        if resultado.returncode != 0:
            raise RuntimeError(
                f"{nome}/{instancia.name} terminou com codigo "
                f"{resultado.returncode}: {resultado.stderr.strip()}"
            )

        campos = resultado.stdout.split()
        if len(campos) != 1:
            raise RuntimeError(
                f"{nome}/{instancia.name} deveria imprimir somente um numero, "
                f"mas imprimiu: {resultado.stdout!r}"
            )

        try:
            custo = float(campos[0])
        except ValueError as erro:
            raise RuntimeError(
                f"{nome}/{instancia.name} retornou um custo invalido: "
                f"{campos[0]!r}"
            ) from erro

        if not math.isfinite(custo):
            raise RuntimeError(
                f"{nome}/{instancia.name} retornou um custo nao finito"
            )

        if indice % 10 == 0 or indice == len(instancias):
            print(f"{nome}: {indice}/{len(instancias)} instancias verificadas")

    print(
        f"{nome}: OK; tempo total={sum(tempos):.2f}s; "
        f"media={sum(tempos) / len(tempos):.3f}s; "
        f"maior={max(tempos):.3f}s"
    )


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument(
        "--versao",
        choices=("v1", "v2", "v3", "v4", "v5", "todas"),
        default="todas",
    )
    parser.add_argument("--tempo-limite", type=float, default=120.0)
    args = parser.parse_args()

    instancias = sorted(PASTA_INSTANCIAS.glob("*.txt"))
    if len(instancias) != 75:
        raise SystemExit(
            f"Esperadas 75 instancias em {PASTA_INSTANCIAS}, "
            f"mas foram encontradas {len(instancias)}"
        )

    selecionados = RUNNERS.items()
    if args.versao != "todas":
        selecionados = [(args.versao, RUNNERS[args.versao])]

    for nome, runner in selecionados:
        verificar_runner(nome, runner, instancias, args.tempo_limite)


if __name__ == "__main__":
    main()

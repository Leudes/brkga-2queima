#!/usr/bin/env python3
"""Mede os seis BRKGAs nas instancias mais pesadas e projeta o experimento."""

from __future__ import annotations

import argparse
import csv
import subprocess
import tempfile
import time
from pathlib import Path

try:
    from benchmark_decoders import (
        CALIBRATED_PARAMETERS,
        DEFAULT_VERSIONS,
        ROOT,
        VERSIONS,
        parse_program_output,
        read_graph,
        sequence_fitness,
    )
except ModuleNotFoundError:
    from scripts.benchmark_decoders import (
        CALIBRATED_PARAMETERS,
        DEFAULT_VERSIONS,
        ROOT,
        VERSIONS,
        parse_program_output,
        read_graph,
        sequence_fitness,
    )


INSTANCIAS = [
    # Maior numero de vertices e maior densidade entre as instancias de n=4000.
    ROOT / "graph_dataset/splits/final/random__gilbert_n4000_d24_r1.txt",
    # Maior numero de arestas de toda a base final.
    ROOT / "graph_dataset/splits/final/harwell_boeing__psmigr_3.txt",
]
SAIDA_PADRAO = ROOT / "tests/resultados/estimativa_pessimista"
QUANTIDADE_FINAL = 175
REPETICOES_FINAIS = 15

CAMPOS = [
    "versao",
    "instancia",
    "vertices",
    "arestas",
    "semente",
    "fitness",
    "tempo_algoritmo_s",
    "tempo_total_s",
    "estado",
    "erro",
]


def montar_comando(versao: str, instancia: Path, saida: Path) -> list[str]:
    comando = [str(VERSIONS[versao]), str(instancia)]
    for nome, valor in CALIBRATED_PARAMETERS[versao].items():
        comando.extend((f"--{nome}", valor))
    comando.extend(("--trials", "1", "--seed", "0", "--output", str(saida)))
    return comando


def ler_csv(caminho: Path) -> list[dict[str, str]]:
    if not caminho.exists():
        return []
    with caminho.open(newline="", encoding="utf-8") as arquivo:
        return list(csv.DictReader(arquivo))


def salvar_linha(caminho: Path, linha: dict[str, object]) -> None:
    novo = not caminho.exists()
    with caminho.open("a", newline="", encoding="utf-8") as arquivo:
        escritor = csv.DictWriter(arquivo, fieldnames=CAMPOS, lineterminator="\n")
        if novo:
            escritor.writeheader()
        escritor.writerow(linha)
        arquivo.flush()


def executar(
    versao: str, instancia: Path, limite: float | None
) -> dict[str, object]:
    grafo = read_graph(instancia)
    inicio = time.perf_counter()
    estado = "ok"
    erro = ""
    fitness: int | str = ""
    tempo_algoritmo: float | str = ""

    with tempfile.TemporaryDirectory(prefix="brkga-estimativa-") as temporaria:
        saida = Path(temporaria) / "resultado.csv"
        try:
            subprocess.run(
                montar_comando(versao, instancia, saida),
                cwd=ROOT,
                check=True,
                timeout=limite,
                capture_output=True,
                text=True,
            )
            resultados = parse_program_output(saida)
            if len(resultados) != 1:
                raise ValueError(f"esperado um resultado, recebidos {len(resultados)}")
            fitness, tempo_us, sequencia = resultados[0]
            tempo_algoritmo = tempo_us / 1_000_000.0
            conferido = sequence_fitness(grafo, sequencia)
            if conferido != fitness:
                raise ValueError(
                    f"fitness do programa={fitness}, simulador={conferido}"
                )
        except subprocess.TimeoutExpired:
            estado = "limite_excedido"
            erro = "tempo maximo excedido"
        except subprocess.CalledProcessError as excecao:
            estado = "erro_programa"
            erro = (excecao.stderr or excecao.stdout or str(excecao))[-500:]
        except Exception as excecao:
            estado = "erro_validacao"
            erro = str(excecao)[:500]

    return {
        "versao": versao,
        "instancia": instancia.name,
        "vertices": len(grafo.adjacency),
        "arestas": grafo.edges,
        "semente": 0,
        "fitness": fitness,
        "tempo_algoritmo_s": tempo_algoritmo,
        "tempo_total_s": f"{time.perf_counter() - inicio:.6f}",
        "estado": estado,
        "erro": erro,
    }


def salvar_estimativa(linhas: list[dict[str, str]], caminho: Path) -> None:
    multiplicador = QUANTIDADE_FINAL * REPETICOES_FINAIS
    campos = [
        "versao",
        "instancia_referencia",
        "tempo_medido_s",
        "execucoes_projetadas",
        "projecao_horas",
        "projecao_dias",
    ]
    estimativas = []
    total_segundos = 0.0
    for versao in DEFAULT_VERSIONS:
        candidatas = []
        for instancia in INSTANCIAS:
            resultados = [
                atual
                for atual in linhas
                if atual["versao"] == versao
                and atual["instancia"] == instancia.name
                and atual["estado"] == "ok"
            ]
            if resultados:
                candidatas.append(
                    max(resultados, key=lambda atual: float(atual["tempo_total_s"]))
                )
        if len(candidatas) != len(INSTANCIAS):
            continue
        linha = max(candidatas, key=lambda atual: float(atual["tempo_total_s"]))
        tempo = float(linha["tempo_total_s"])
        projecao = tempo * multiplicador
        total_segundos += projecao
        estimativas.append(
            {
                "versao": versao,
                "instancia_referencia": linha["instancia"],
                "tempo_medido_s": f"{tempo:.6f}",
                "execucoes_projetadas": multiplicador,
                "projecao_horas": f"{projecao / 3600.0:.6f}",
                "projecao_dias": f"{projecao / 86400.0:.6f}",
            }
        )

    if len(estimativas) == len(DEFAULT_VERSIONS):
        estimativas.append(
            {
                "versao": "TOTAL_6_VERSOES",
                "instancia_referencia": "maior tempo de cada versao",
                "tempo_medido_s": f"{sum(float(l['tempo_medido_s']) for l in estimativas):.6f}",
                "execucoes_projetadas": multiplicador * len(DEFAULT_VERSIONS),
                "projecao_horas": f"{total_segundos / 3600.0:.6f}",
                "projecao_dias": f"{total_segundos / 86400.0:.6f}",
            }
        )

    with caminho.open("w", newline="", encoding="utf-8") as arquivo:
        escritor = csv.DictWriter(arquivo, fieldnames=campos, lineterminator="\n")
        escritor.writeheader()
        escritor.writerows(estimativas)

    print("\nProjecao pessimista: maior tempo de cada versao x 175 x 15")
    for linha in estimativas:
        print(
            f"  {linha['versao']:24} "
            f"{float(linha['projecao_horas']):8.2f} h  "
            f"({float(linha['projecao_dias']):6.2f} dias)  "
            f"referencia={linha['instancia_referencia']}"
        )


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument(
        "--limite-segundos",
        type=float,
        default=1800.0,
        help="limite por versao; 0 desativa",
    )
    parser.add_argument("--saida", type=Path, default=SAIDA_PADRAO)
    args = parser.parse_args()

    if args.limite_segundos < 0:
        raise SystemExit("--limite-segundos nao pode ser negativo")
    for instancia in INSTANCIAS:
        if not instancia.is_file():
            raise FileNotFoundError(instancia)
    for versao in DEFAULT_VERSIONS:
        if not VERSIONS[versao].is_file():
            raise FileNotFoundError(f"execute make: {VERSIONS[versao]}")

    print("Instancias usadas na estimativa pessimista:")
    for instancia in INSTANCIAS:
        grafo = read_graph(instancia)
        print(
            f"  {instancia.name}: "
            f"{len(grafo.adjacency)} vertices, {grafo.edges} arestas"
        )
    pasta_saida = args.saida.resolve()
    pasta_saida.mkdir(parents=True, exist_ok=True)
    caminho_bruto = pasta_saida / "resultados.csv"
    anteriores = ler_csv(caminho_bruto)
    concluidas = {
        (linha["versao"], linha["instancia"])
        for linha in anteriores
    }
    limite = None if args.limite_segundos == 0 else args.limite_segundos

    total = len(DEFAULT_VERSIONS) * len(INSTANCIAS)
    indice = 0
    for instancia in INSTANCIAS:
        for versao in DEFAULT_VERSIONS:
            indice += 1
            chave = (versao, instancia.name)
            if chave in concluidas:
                print(f"[{indice}/{total}] ja concluida: {versao} / {instancia.name}")
                continue
            print(
                f"[{indice}/{total}] executando {versao} / {instancia.name}...",
                flush=True,
            )
            linha = executar(versao, instancia, limite)
            salvar_linha(caminho_bruto, linha)
            anteriores.append({chave: str(valor) for chave, valor in linha.items()})
            print(
                f"      estado={linha['estado']} fitness={linha['fitness']} "
                f"tempo={linha['tempo_total_s']}s",
                flush=True,
            )

    salvar_estimativa(ler_csv(caminho_bruto), pasta_saida / "estimativa.csv")
    print(f"\nResultados salvos em: {pasta_saida}")


if __name__ == "__main__":
    main()

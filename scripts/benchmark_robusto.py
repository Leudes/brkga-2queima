#!/usr/bin/env python3
"""Benchmark robusto e retomavel dos seis BRKGAs calibrados."""

from __future__ import annotations

import argparse
import csv
import math
import statistics
import subprocess
import tempfile
import time
from collections import defaultdict
from dataclasses import dataclass
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


PASTA_BASE_FINAL = ROOT / "graph_dataset/splits/final"
PASTA_CLASSICOS = ROOT / "graph_dataset/generated/classic_exact"
SAIDA_PADRAO = ROOT / "tests/resultados/robusto_15_instancias"

# Amostra manual estratificada por origem, tamanho e densidade.
# Proporcao: 4 Harwell-Boeing, 3 aleatorios, 2 SNAP e 1 DIMACS.
AMOSTRA_FINAL = [
    ("dimacs10__karate.txt", "social pequeno"),
    ("harwell_boeing__steam3.txt", "matriz pequena, densidade media"),
    ("random__gilbert_n200_d3_r1.txt", "aleatorio pequeno e esparso"),
    ("harwell_boeing__mbeaflw.txt", "matriz media e muito densa"),
    ("snap__as19990101.txt", "rede real media e esparsa"),
    ("random__gilbert_n1000_d12_r1.txt", "aleatorio medio"),
    ("harwell_boeing__lshp1270.txt", "matriz media-grande e esparsa"),
    ("random__gilbert_n2000_d24_r2.txt", "aleatorio grande e denso"),
    ("harwell_boeing__cegb3024.txt", "matriz grande e densa"),
    ("snap__as19980824.txt", "rede real muito grande e esparsa"),
]


@dataclass(frozen=True)
class Instancia:
    caminho: Path
    grupo: str
    estrutura: str
    valor_exato: int | None = None


CAMPOS = [
    "grupo",
    "estrutura",
    "valor_exato",
    "versao",
    "instancia",
    "vertices",
    "arestas",
    "repeticao",
    "semente",
    "fitness",
    "gap_exato_percentual",
    "tempo_algoritmo_s",
    "tempo_total_s",
    "fontes_diretas",
    "estado",
    "erro",
]


def escrever_grafo(caminho: Path, descricao: str, arestas) -> None:
    caminho.parent.mkdir(parents=True, exist_ok=True)
    with caminho.open("w", encoding="utf-8") as arquivo:
        arquivo.write(f"# {descricao}\n")
        for origem, destino in arestas:
            arquivo.write(f"{origem} {destino}\n")


def gerar_instancias_classicas() -> list[Instancia]:
    ciclo = PASTA_CLASSICOS / "classico__ciclo_n250.txt"
    escrever_grafo(
        ciclo,
        "Ciclo C_250; b2 exato = ceil(250/2) + 1 = 126",
        ((vertice, (vertice + 1) % 250) for vertice in range(250)),
    )

    caminho = PASTA_CLASSICOS / "classico__caminho_n251.txt"
    escrever_grafo(
        caminho,
        "Caminho P_251; b2 exato = ceil(251/2) + 1 = 127",
        ((vertice, vertice + 1) for vertice in range(250)),
    )

    completo = PASTA_CLASSICOS / "classico__completo_n200.txt"
    escrever_grafo(
        completo,
        "Completo K_200; b2 exato = 3",
        (
            (origem, destino)
            for origem in range(200)
            for destino in range(origem + 1, 200)
        ),
    )

    bipartido = (
        PASTA_CLASSICOS / "classico__bipartido_completo_100x100.txt"
    )
    escrever_grafo(
        bipartido,
        "Bipartido completo K_100,100; b2 exato = 4",
        (
            (origem, destino)
            for origem in range(100)
            for destino in range(100, 200)
        ),
    )

    estrela = PASTA_CLASSICOS / "classico__estrela_n200.txt"
    escrever_grafo(
        estrela,
        "Estrela K_1,199; b2 exato = 199",
        ((0, folha) for folha in range(1, 200)),
    )

    return [
        Instancia(ciclo, "classico", "ciclo C_250", 126),
        Instancia(caminho, "classico", "caminho P_251", 127),
        Instancia(completo, "classico", "completo K_200", 3),
        Instancia(
            bipartido,
            "classico",
            "bipartido completo K_100,100",
            4,
        ),
        Instancia(estrela, "classico", "estrela K_1,199", 199),
    ]


def carregar_instancias() -> list[Instancia]:
    finais = [
        Instancia(PASTA_BASE_FINAL / nome, "base_final", estrutura)
        for nome, estrutura in AMOSTRA_FINAL
    ]
    instancias = finais + gerar_instancias_classicas()
    ausentes = [item.caminho for item in instancias if not item.caminho.is_file()]
    if ausentes:
        raise FileNotFoundError(f"instancias ausentes: {ausentes}")
    return instancias


def montar_comando(
    versao: str,
    instancia: Path,
    semente: int,
    saida: Path,
) -> list[str]:
    comando = [str(VERSIONS[versao]), str(instancia)]
    for nome, valor in CALIBRATED_PARAMETERS[versao].items():
        comando.extend((f"--{nome}", valor))
    comando.extend(
        (
            "--trials",
            "1",
            "--seed",
            str(semente),
            "--output",
            str(saida),
        )
    )
    return comando


def ler_execucoes(caminho: Path) -> list[dict[str, str]]:
    if not caminho.exists():
        return []
    with caminho.open(newline="", encoding="utf-8") as arquivo:
        return list(csv.DictReader(arquivo))


def salvar_execucao(caminho: Path, linha: dict[str, object]) -> None:
    novo = not caminho.exists()
    with caminho.open("a", newline="", encoding="utf-8") as arquivo:
        escritor = csv.DictWriter(
            arquivo,
            fieldnames=CAMPOS,
            lineterminator="\n",
        )
        if novo:
            escritor.writeheader()
        escritor.writerow(linha)
        arquivo.flush()


def executar_uma_vez(
    versao: str,
    instancia: Instancia,
    repeticao: int,
    semente: int,
    limite_segundos: float | None,
) -> dict[str, object]:
    grafo = read_graph(instancia.caminho)
    inicio = time.perf_counter()
    estado = "ok"
    mensagem = ""
    fitness: int | str = ""
    tempo_algoritmo: float | str = ""
    fontes: int | str = ""
    gap_exato: float | str = ""

    with tempfile.TemporaryDirectory(prefix="brkga-robusto-") as temporaria:
        saida = Path(temporaria) / "resultado.csv"
        try:
            processo = subprocess.run(
                montar_comando(versao, instancia.caminho, semente, saida),
                cwd=ROOT,
                check=True,
                timeout=limite_segundos,
                capture_output=True,
                text=True,
            )
            resultados = parse_program_output(saida)
            if len(resultados) != 1:
                raise ValueError(
                    f"esperado um resultado, recebidos {len(resultados)}"
                )
            fitness, tempo_us, sequencia = resultados[0]
            tempo_algoritmo = tempo_us / 1_000_000.0
            fontes = len(sequencia)
            conferido = sequence_fitness(grafo, sequencia)
            if conferido != fitness:
                raise ValueError(
                    f"fitness do programa={fitness}, simulador={conferido}"
                )
            if instancia.valor_exato is not None:
                if fitness < instancia.valor_exato:
                    raise ValueError(
                        f"fitness {fitness} menor que o valor exato "
                        f"{instancia.valor_exato}"
                    )
                gap_exato = (
                    100.0
                    * (fitness - instancia.valor_exato)
                    / instancia.valor_exato
                )
        except subprocess.TimeoutExpired:
            estado = "limite_excedido"
            mensagem = "tempo maximo da execucao excedido"
        except subprocess.CalledProcessError as erro:
            estado = "erro_programa"
            mensagem = (erro.stderr or erro.stdout or str(erro))[-500:]
        except Exception as erro:  # Mantem o benchmark noturno em execucao.
            estado = "erro_validacao"
            mensagem = str(erro)[:500]

    return {
        "grupo": instancia.grupo,
        "estrutura": instancia.estrutura,
        "valor_exato": (
            instancia.valor_exato if instancia.valor_exato is not None else ""
        ),
        "versao": versao,
        "instancia": instancia.caminho.name,
        "vertices": len(grafo.adjacency),
        "arestas": grafo.edges,
        "repeticao": repeticao,
        "semente": semente,
        "fitness": fitness,
        "gap_exato_percentual": gap_exato,
        "tempo_algoritmo_s": tempo_algoritmo,
        "tempo_total_s": f"{time.perf_counter() - inicio:.6f}",
        "fontes_diretas": fontes,
        "estado": estado,
        "erro": mensagem,
    }


def salvar_manifesto(instancias: list[Instancia], caminho: Path) -> None:
    campos = [
        "grupo",
        "instancia",
        "estrutura",
        "vertices",
        "arestas",
        "grau_medio",
        "valor_exato",
    ]
    with caminho.open("w", newline="", encoding="utf-8") as arquivo:
        escritor = csv.DictWriter(arquivo, fieldnames=campos, lineterminator="\n")
        escritor.writeheader()
        for instancia in instancias:
            grafo = read_graph(instancia.caminho)
            numero_vertices = len(grafo.adjacency)
            escritor.writerow(
                {
                    "grupo": instancia.grupo,
                    "instancia": instancia.caminho.name,
                    "estrutura": instancia.estrutura,
                    "vertices": numero_vertices,
                    "arestas": grafo.edges,
                    "grau_medio": f"{2.0 * grafo.edges / numero_vertices:.6f}",
                    "valor_exato": (
                        instancia.valor_exato
                        if instancia.valor_exato is not None
                        else ""
                    ),
                }
            )


def resumir(caminho_bruto: Path, pasta_saida: Path) -> None:
    linhas = [
        linha
        for linha in ler_execucoes(caminho_bruto)
        if linha["estado"] == "ok"
    ]
    if not linhas:
        return

    agrupado: dict[tuple[str, str], list[dict[str, str]]] = defaultdict(list)
    for linha in linhas:
        agrupado[(linha["versao"], linha["instancia"])].append(linha)

    por_instancia = []
    for (versao, instancia), valores in sorted(agrupado.items()):
        fitness = [int(valor["fitness"]) for valor in valores]
        tempos = [float(valor["tempo_algoritmo_s"]) for valor in valores]
        valor_exato = valores[0]["valor_exato"]
        por_instancia.append(
            {
                "versao": versao,
                "instancia": instancia,
                "grupo": valores[0]["grupo"],
                "valor_exato": valor_exato,
                "execucoes": len(valores),
                "melhor_fitness": min(fitness),
                "media_fitness": f"{statistics.fmean(fitness):.6f}",
                "desvio_fitness": (
                    f"{statistics.pstdev(fitness):.6f}"
                    if len(fitness) > 1
                    else "0.000000"
                ),
                "media_tempo_s": f"{statistics.fmean(tempos):.6f}",
                "atingiu_exato": (
                    "sim"
                    if valor_exato and min(fitness) == int(valor_exato)
                    else ("nao" if valor_exato else "")
                ),
            }
        )

    with (pasta_saida / "resumo_por_instancia.csv").open(
        "w", newline="", encoding="utf-8"
    ) as arquivo:
        escritor = csv.DictWriter(
            arquivo,
            fieldnames=list(por_instancia[0].keys()),
            lineterminator="\n",
        )
        escritor.writeheader()
        escritor.writerows(por_instancia)

    medias_por_instancia = defaultdict(dict)
    for linha in por_instancia:
        medias_por_instancia[linha["instancia"]][linha["versao"]] = float(
            linha["media_fitness"]
        )

    gerais = []
    for versao in DEFAULT_VERSIONS:
        valores = [linha for linha in por_instancia if linha["versao"] == versao]
        if not valores:
            continue
        gaps = []
        vitorias = 0
        for linha in valores:
            melhor = min(medias_por_instancia[linha["instancia"]].values())
            media = float(linha["media_fitness"])
            gaps.append(100.0 * (media - melhor) / melhor)
            if math.isclose(media, melhor):
                vitorias += 1
        gerais.append(
            {
                "versao": versao,
                "instancias_concluidas": len(valores),
                "gap_medio_para_melhor_percentual": f"{statistics.fmean(gaps):.6f}",
                "tempo_medio_por_execucao_s": f"{statistics.fmean(float(v['media_tempo_s']) for v in valores):.6f}",
                "vitorias_ou_empates": vitorias,
                "tempo_total_h": f"{sum(float(l['tempo_algoritmo_s']) for l in linhas if l['versao'] == versao) / 3600.0:.6f}",
            }
        )

    with (pasta_saida / "resumo_geral.csv").open(
        "w", newline="", encoding="utf-8"
    ) as arquivo:
        escritor = csv.DictWriter(
            arquivo,
            fieldnames=list(gerais[0].keys()),
            lineterminator="\n",
        )
        escritor.writeheader()
        escritor.writerows(gerais)


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--repeticoes", type=int, default=15)
    parser.add_argument("--semente-inicial", type=int, default=0)
    parser.add_argument(
        "--limite-segundos",
        type=float,
        default=1800.0,
        help="limite por execucao; 0 desativa",
    )
    parser.add_argument("--saida", type=Path, default=SAIDA_PADRAO)
    parser.add_argument(
        "--listar",
        action="store_true",
        help="gera e lista as instancias sem executar o benchmark",
    )
    args = parser.parse_args()

    if args.repeticoes <= 0 or args.semente_inicial < 0:
        raise SystemExit("repeticoes e semente devem ser validas")
    if args.limite_segundos < 0:
        raise SystemExit("--limite-segundos nao pode ser negativo")
    for versao in DEFAULT_VERSIONS:
        if not VERSIONS[versao].is_file():
            raise FileNotFoundError(
                f"executavel ausente: {VERSIONS[versao]}; execute make"
            )

    instancias = carregar_instancias()
    pasta_saida = args.saida.resolve()
    pasta_saida.mkdir(parents=True, exist_ok=True)
    salvar_manifesto(instancias, pasta_saida / "instancias.csv")

    print("Instancias selecionadas:")
    for instancia in instancias:
        grafo = read_graph(instancia.caminho)
        exato = (
            f", exato={instancia.valor_exato}"
            if instancia.valor_exato is not None
            else ""
        )
        print(
            f"  {instancia.grupo:10} n={len(grafo.adjacency):4d} "
            f"m={grafo.edges:7d}: {instancia.caminho.name}{exato}"
        )
    if args.listar:
        return

    caminho_bruto = pasta_saida / "resultados_brutos.csv"
    anteriores = ler_execucoes(caminho_bruto)
    concluidas = {
        (linha["versao"], linha["instancia"], int(linha["repeticao"]))
        for linha in anteriores
    }
    total = len(DEFAULT_VERSIONS) * len(instancias) * args.repeticoes
    feitas = len(concluidas)
    limite = None if args.limite_segundos == 0 else args.limite_segundos

    print(f"\nExecucoes: {total}; concluidas: {feitas}; pendentes: {total - feitas}")
    print("Estimativa inicial neste computador: cerca de 14 h (faixa de 12 a 18 h).")
    inicio_geral = time.perf_counter()

    try:
        # Repeticao externa deixa resultados balanceados entre todas as versoes
        # caso a execucao seja interrompida e retomada depois.
        for repeticao in range(args.repeticoes):
            semente = args.semente_inicial + repeticao
            for instancia in instancias:
                for versao in DEFAULT_VERSIONS:
                    chave = (versao, instancia.caminho.name, repeticao)
                    if chave in concluidas:
                        continue

                    inicio = time.perf_counter()
                    linha = executar_uma_vez(
                        versao,
                        instancia,
                        repeticao,
                        semente,
                        limite,
                    )
                    salvar_execucao(caminho_bruto, linha)
                    concluidas.add(chave)
                    feitas += 1

                    duracao = time.perf_counter() - inicio
                    decorrido = time.perf_counter() - inicio_geral
                    realizadas_agora = max(1, feitas - len(anteriores))
                    media_observada = decorrido / realizadas_agora
                    eta_horas = media_observada * (total - feitas) / 3600.0
                    print(
                        f"[{feitas:04d}/{total}] {versao} "
                        f"{instancia.caminho.name} rep={repeticao} "
                        f"estado={linha['estado']} fitness={linha['fitness']} "
                        f"tempo={duracao:.1f}s ETA_simples={eta_horas:.1f}h",
                        flush=True,
                    )
    except KeyboardInterrupt:
        print("\nInterrompido; os resultados concluidos foram preservados.")
    finally:
        resumir(caminho_bruto, pasta_saida)

    print(f"\nResultados salvos em: {pasta_saida}")


if __name__ == "__main__":
    main()

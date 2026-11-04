#!/usr/bin/env python3
"""Teste piloto dos BRKGAs com os parametros encontrados pelo irace."""

from __future__ import annotations

import argparse
import csv
import statistics
import subprocess
import tempfile
import time
from collections import defaultdict
from pathlib import Path


RAIZ = Path(__file__).resolve().parents[1]
PASTA_INSTANCIAS = RAIZ / "graph_dataset/splits/final"

VERSOES = {
    "V1": RAIZ / "2queima",
    "V2": RAIZ / "2queimaV2",
    "V3": RAIZ / "2queimaV3",
    "V4": RAIZ / "2queimaV4",
    "V5": RAIZ / "2queimaV5",
    "V6": RAIZ / "2queimaV6",
}

PARAMETROS_CALIBRADOS = {
    "V1": (578, 0.2788, 0.0743, 0.6237, 583, 477, 125),
    "V2": (518, 0.2463, 0.2171, 0.6522, 651, 451, 91),
    "V3": (572, 0.2485, 0.1638, 0.6334, 344, 102, 86),
    "V4": (407, 0.2899, 0.0692, 0.6514, 762, 411, 137),
    "V5": (554, 0.2689, 0.2702, 0.6252, 652, 486, 37),
    # Provisorio: a V6 usa os parametros da V2, a versao mais semelhante.
    "V6": (518, 0.2463, 0.2171, 0.6522, 651, 451, 91),
}

# Configuracao usada nos benchmarks anteriores. Ela e executada rapidamente e
# permite medir, nas mesmas instancias, o ganho obtido depois da calibracao.
PARAMETROS_ANTERIORES = {
    versao: (40, 0.20, 0.10, 0.70, 60, 30, 20)
    for versao in VERSOES
}

PARAMETROS_FIXOS = {
    "calibrada": (2, 4, 2),
    "anterior": (2, 2, 1),
}

CAMPOS = [
    "configuracao", "versao", "instancia", "vertices", "arestas",
    "repeticao", "semente", "fitness", "tempo_algoritmo_s",
    "tempo_total_s", "estado",
]


def ler_tamanho_grafo(caminho: Path) -> tuple[int, int]:
    maior_vertice = -1
    arestas = 0
    with caminho.open(encoding="utf-8") as arquivo:
        for linha in arquivo:
            linha = linha.strip()
            if not linha or linha[0] in "#%":
                continue
            partes = linha.split()
            if len(partes) < 2:
                continue
            try:
                u, v = int(partes[0]), int(partes[1])
            except ValueError:
                continue
            if u == v:
                continue
            maior_vertice = max(maior_vertice, u, v)
            arestas += 1
    return maior_vertice + 1, arestas


def selecionar_amostra(quantidade: int) -> list[tuple[Path, int, int]]:
    instancias = []
    for caminho in sorted(PASTA_INSTANCIAS.glob("*.txt")):
        vertices, arestas = ler_tamanho_grafo(caminho)
        instancias.append((caminho, vertices, arestas))

    if not instancias:
        raise SystemExit(f"Nenhuma instancia encontrada em {PASTA_INSTANCIAS}")
    if quantidade <= 0 or quantidade > len(instancias):
        raise SystemExit(
            f"--instancias deve estar entre 1 e {len(instancias)}"
        )

    instancias.sort(key=lambda item: (item[1], item[2], item[0].name))
    if quantidade == 1:
        return [instancias[len(instancias) // 2]]

    indices = {
        round(i * (len(instancias) - 1) / (quantidade - 1))
        for i in range(quantidade)
    }
    return [instancias[indice] for indice in sorted(indices)]


def carregar_execucoes(caminho: Path) -> tuple[list[dict[str, str]], set[tuple[str, ...]]]:
    if not caminho.exists():
        return [], set()
    with caminho.open(encoding="utf-8", newline="") as arquivo:
        linhas = list(csv.DictReader(arquivo))
    concluidas = {
        (linha["configuracao"], linha["versao"], linha["instancia"], linha["repeticao"])
        for linha in linhas
    }
    return linhas, concluidas


def salvar_linha(caminho: Path, linha: dict[str, object]) -> None:
    novo = not caminho.exists()
    caminho.parent.mkdir(parents=True, exist_ok=True)
    with caminho.open("a", encoding="utf-8", newline="") as arquivo:
        escritor = csv.DictWriter(arquivo, fieldnames=CAMPOS, lineterminator="\n")
        if novo:
            escritor.writeheader()
        escritor.writerow(linha)


def montar_comando(
    executavel: Path,
    instancia: Path,
    parametros: tuple[float, ...],
    configuracao: str,
    semente: int,
    saida: Path,
) -> list[str]:
    p, pe, pm, rhoe, max_gens, max_stagt, x_intvl = parametros
    populacoes, threads, trocas = PARAMETROS_FIXOS[configuracao]
    return [
        str(executavel), str(instancia),
        "--p", str(p), "--pe", str(pe), "--pm", str(pm),
        "--rhoe", str(rhoe), "--K", str(populacoes),
        "--MAXT", str(threads), "--X_NUMBER", str(trocas),
        "--MAX_GENS", str(max_gens),
        "--MAX_STAGT", str(max_stagt), "--X_INTVL", str(x_intvl),
        "--trials", "1", "--seed", str(semente), "--output", str(saida),
    ]


def ler_resultado(caminho: Path) -> tuple[int, float]:
    with caminho.open(encoding="utf-8", newline="") as arquivo:
        linhas = list(csv.reader(arquivo))
    if len(linhas) < 2 or len(linhas[1]) != 6:
        raise RuntimeError(f"Saida inesperada em {caminho}")
    return int(float(linhas[1][4])), float(linhas[1][5]) / 1_000_000.0


def executar(
    configuracao: str,
    parametros_por_versao: dict[str, tuple[float, ...]],
    versoes: list[str],
    amostra: list[tuple[Path, int, int]],
    repeticoes: int,
    limite: float | None,
    caminho_bruto: Path,
    concluidas: set[tuple[str, ...]],
) -> None:
    total = len(versoes) * len(amostra) * repeticoes
    numero = 0
    for versao in versoes:
        executavel = VERSOES[versao]
        for instancia, vertices, arestas in amostra:
            for repeticao in range(repeticoes):
                numero += 1
                chave = (configuracao, versao, instancia.name, str(repeticao))
                if chave in concluidas:
                    print(f"[{numero:03d}/{total}] ja feito: {configuracao} {versao} {instancia.name}")
                    continue

                semente = 20261022 + repeticao
                print(
                    f"[{numero:03d}/{total}] {configuracao} {versao}: "
                    f"{instancia.name} ({vertices} vertices)",
                    flush=True,
                )
                with tempfile.NamedTemporaryFile(suffix=".csv", delete=False) as temporario:
                    saida = Path(temporario.name)
                saida.unlink(missing_ok=True)
                inicio = time.perf_counter()
                estado = "ok"
                fitness: int | str = ""
                tempo_algoritmo: float | str = ""
                try:
                    subprocess.run(
                        montar_comando(
                            executavel, instancia,
                            parametros_por_versao[versao], configuracao,
                            semente, saida,
                        ),
                        cwd=RAIZ,
                        check=True,
                        timeout=limite,
                    )
                    fitness, tempo_algoritmo = ler_resultado(saida)
                except subprocess.TimeoutExpired:
                    estado = "limite_excedido"
                finally:
                    tempo_total = time.perf_counter() - inicio
                    saida.unlink(missing_ok=True)

                linha = {
                    "configuracao": configuracao,
                    "versao": versao,
                    "instancia": instancia.name,
                    "vertices": vertices,
                    "arestas": arestas,
                    "repeticao": repeticao,
                    "semente": semente,
                    "fitness": fitness,
                    "tempo_algoritmo_s": tempo_algoritmo,
                    "tempo_total_s": f"{tempo_total:.6f}",
                    "estado": estado,
                }
                salvar_linha(caminho_bruto, linha)
                concluidas.add(chave)


def gerar_resumo(
    caminho_bruto: Path,
    caminho_resumo: Path,
    repeticoes_finais: int,
    total_instancias: int,
) -> None:
    linhas, _ = carregar_execucoes(caminho_bruto)
    validas = [linha for linha in linhas if linha["estado"] == "ok"]
    grupos: dict[tuple[str, str, str], list[dict[str, str]]] = defaultdict(list)
    for linha in validas:
        grupos[(linha["configuracao"], linha["versao"], linha["instancia"])].append(linha)

    medias: dict[tuple[str, str, str], tuple[float, float]] = {}
    for chave, valores in grupos.items():
        medias[chave] = (
            statistics.fmean(float(valor["fitness"]) for valor in valores),
            statistics.fmean(float(valor["tempo_total_s"]) for valor in valores),
        )

    linhas_relatorio = [
        "# Teste piloto dos parametros calibrados",
        "",
        f"Projecao para {total_instancias} instancias e {repeticoes_finais} repeticoes.",
        "",
        "| Versao | Tempo medio calibrado | Projecao da versao | Melhora media no fitness |",
        "|---|---:|---:|---:|",
    ]
    for versao in VERSOES:
        tempos = [
            tempo for (configuracao, atual, _), (_, tempo) in medias.items()
            if configuracao == "calibrada" and atual == versao
        ]
        melhorias = []
        for (configuracao, atual, instancia), (fitness, _) in medias.items():
            if configuracao != "calibrada" or atual != versao:
                continue
            anterior = medias.get(("anterior", versao, instancia))
            if anterior and anterior[0] != 0:
                melhorias.append(100.0 * (anterior[0] - fitness) / anterior[0])

        if tempos:
            media_tempo = statistics.fmean(tempos)
            projecao_horas = media_tempo * total_instancias * repeticoes_finais / 3600.0
            texto_tempo = f"{media_tempo:.2f} s"
            texto_projecao = f"{projecao_horas:.2f} h"
        else:
            texto_tempo = texto_projecao = "sem dados"
        texto_melhoria = (
            f"{statistics.fmean(melhorias):+.2f}%" if melhorias else "sem comparacao"
        )
        linhas_relatorio.append(
            f"| {versao} | {texto_tempo} | {texto_projecao} | {texto_melhoria} |"
        )

    linhas_relatorio.extend([
        "",
        "Melhora positiva significa fitness menor com os parametros calibrados.",
        "A projecao considera execucao sequencial e e apenas uma estimativa; "
        "densidade e estrutura do grafo tambem influenciam o tempo.",
    ])
    limites = [linha for linha in linhas if linha["estado"] == "limite_excedido"]
    if limites:
        linhas_relatorio.extend([
            "",
            f"Atencao: {len(limites)} execucoes excederam o limite. A projecao "
            "acima fica subestimada enquanto houver execucoes incompletas.",
        ])
    linhas_relatorio.append("")
    caminho_resumo.write_text("\n".join(linhas_relatorio), encoding="utf-8")
    print("\n" + "\n".join(linhas_relatorio))


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--instancias", type=int, default=10)
    parser.add_argument("--repeticoes", type=int, default=1)
    parser.add_argument("--repeticoes-finais", type=int, default=30)
    parser.add_argument("--versoes", nargs="+", choices=VERSOES, default=list(VERSOES))
    parser.add_argument("--sem-comparacao", action="store_true")
    parser.add_argument(
        "--limite-segundos", type=float, default=1800.0,
        help="limite de cada execucao; use 0 para nao limitar",
    )
    parser.add_argument(
        "--saida", type=Path,
        default=RAIZ / "tests/resultados/piloto_calibrado",
    )
    parser.add_argument("--listar-amostra", action="store_true")
    args = parser.parse_args()

    if args.repeticoes <= 0 or args.repeticoes_finais <= 0:
        raise SystemExit("As quantidades de repeticoes devem ser positivas")

    amostra = selecionar_amostra(args.instancias)
    print("Amostra selecionada:")
    for caminho, vertices, arestas in amostra:
        print(f"  {vertices:4d} vertices, {arestas:7d} arestas: {caminho.name}")
    if args.listar_amostra:
        return

    for versao in args.versoes:
        if not VERSOES[versao].is_file():
            raise SystemExit(f"Executavel ausente: {VERSOES[versao]}. Rode make primeiro.")

    args.saida.mkdir(parents=True, exist_ok=True)
    caminho_bruto = args.saida / "resultados_brutos.csv"
    _, concluidas = carregar_execucoes(caminho_bruto)
    limite = None if args.limite_segundos == 0 else args.limite_segundos

    if not args.sem_comparacao:
        executar(
            "anterior", PARAMETROS_ANTERIORES, args.versoes, amostra,
            args.repeticoes, limite, caminho_bruto, concluidas,
        )
    executar(
        "calibrada", PARAMETROS_CALIBRADOS, args.versoes, amostra,
        args.repeticoes, limite, caminho_bruto, concluidas,
    )
    gerar_resumo(
        caminho_bruto, args.saida / "resumo.md", args.repeticoes_finais,
        len(list(PASTA_INSTANCIAS.glob("*.txt"))),
    )
    print(f"\nArquivos salvos em: {args.saida}")


if __name__ == "__main__":
    main()

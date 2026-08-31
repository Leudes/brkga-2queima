#!/usr/bin/env python3
"""Coleta, limpa e separa instancias de grafos para o TCC.

As saidas sao listas de arestas simples, nao direcionadas e zero-based,
compativeis com Graph.cpp deste repositorio. O script usa apenas a biblioteca
padrao e ``requests``.
"""

from __future__ import annotations

import argparse
import csv
import gzip
import hashlib
import html
import io
import json
import math
import os
import random
import re
import shutil
import sys
import tarfile
import time
from collections import defaultdict, deque
from dataclasses import asdict, dataclass
from pathlib import Path
from typing import Iterable, Iterator, Sequence

import requests
from requests.adapters import HTTPAdapter
from urllib3.util.retry import Retry


SUITESPARSE_GROUP = "https://sparse.tamu.edu/{group}?per_page=All"
SUITESPARSE_MM = (
    "https://suitesparse-collection-website.herokuapp.com/MM/{group}/{name}.tar.gz"
)
SNAP_ROOT = "https://snap.stanford.edu/data"

DEFAULT_WEIGHTS = {
    "harwell_boeing": 100,
    "random": 80,
    "snap": 50,
    "dimacs10": 20,
}

# Matrizes que o catalogo oficial do DIMACS10 identifica como instancias da
# colecao, mas armazena em outros grupos para evitar duplicacao de dados.
DIMACS10_CROSSWALK = [
    ("Newman", "adjnoun"),
    ("Arenas", "celegans_metabolic"),
    ("Newman", "celegansneural"),
    ("Newman", "dolphins"),
    ("Arenas", "email"),
    ("Newman", "football"),
    ("Arenas", "jazz"),
    # O texto historico do DIMACS10 aponta Arenas/karate, mas o espelho atual
    # da SuiteSparse publica o mesmo arquivo em Newman/karate.
    ("Newman", "karate"),
    ("Newman", "lesmis"),
    ("Newman", "netscience"),
    ("Newman", "polblogs"),
    ("Newman", "polbooks"),
    ("Hamm", "add20"),
]

SNAP_FILES = [
    ("email-Eu-core", "email-Eu-core.txt.gz"),
    ("email-Eu-core-temporal", "email-Eu-core-temporal.txt.gz"),
    ("CollegeMsg", "CollegeMsg.txt.gz"),
    ("bitcoin-alpha", "soc-sign-bitcoinalpha.csv.gz"),
    ("email-Eu-dept1", "email-Eu-core-temporal-Dept1.txt.gz"),
    ("email-Eu-dept2", "email-Eu-core-temporal-Dept2.txt.gz"),
    ("email-Eu-dept3", "email-Eu-core-temporal-Dept3.txt.gz"),
    ("email-Eu-dept4", "email-Eu-core-temporal-Dept4.txt.gz"),
]


@dataclass(frozen=True)
class Candidate:
    source: str
    family: str
    name: str
    url: str
    file_format: str
    declared_n: int | None = None
    declared_entries: int | None = None
    kind: str = ""


@dataclass
class RawGraph:
    edges: list[tuple[int, int]]
    vertices: set[int]
    declared_n: int | None = None


@dataclass
class GraphRecord:
    source: str
    family: str
    original_name: str
    source_url: str
    file: str
    split: str
    raw_vertices: int
    raw_edges: int
    vertices: int
    edges: int
    loops_removed: int
    duplicates_removed: int
    components_before: int
    vertices_dropped: int
    density: float
    size_bin: str
    density_bin: str
    graph_sha256: str


@dataclass
class PreparedGraph:
    candidate: Candidate
    edges: list[tuple[int, int]]
    record: GraphRecord


class HttpClient:
    def __init__(self, cache_dir: Path, timeout: int = 120) -> None:
        self.cache_dir = cache_dir
        self.timeout = timeout
        self.cache_dir.mkdir(parents=True, exist_ok=True)
        retry = Retry(
            total=4,
            backoff_factor=1.0,
            status_forcelist=(429, 500, 502, 503, 504),
            allowed_methods=("GET",),
        )
        self.session = requests.Session()
        self.session.headers["User-Agent"] = "2-queima-tcc-dataset-builder/1.0"
        self.session.mount("https://", HTTPAdapter(max_retries=retry))

    def text(self, url: str) -> str:
        response = self.session.get(url, timeout=self.timeout)
        response.raise_for_status()
        return response.text

    def download(self, url: str, label: str) -> Path:
        suffix = "".join(Path(url.split("?", 1)[0]).suffixes) or ".dat"
        safe = re.sub(r"[^A-Za-z0-9_.-]+", "_", label).strip("._")
        digest = hashlib.sha256(url.encode()).hexdigest()[:12]
        target = self.cache_dir / f"{safe}-{digest}{suffix}"
        if target.exists() and target.stat().st_size:
            return target
        partial = target.with_suffix(target.suffix + ".part")
        with self.session.get(url, stream=True, timeout=self.timeout) as response:
            response.raise_for_status()
            with partial.open("wb") as output:
                for chunk in response.iter_content(1024 * 1024):
                    if chunk:
                        output.write(chunk)
        partial.replace(target)
        return target


def _cell(row: str, class_name: str) -> str | None:
    match = re.search(
        rf"<td class=['\"]{re.escape(class_name)}[^'\"]*['\"]>(.*?)</td>",
        row,
        re.DOTALL,
    )
    if not match:
        return None
    return html.unescape(re.sub(r"<[^>]+>", "", match.group(1))).strip()


def discover_suitesparse(
    client: HttpClient, group: str, source: str, min_vertices: int, max_vertices: int,
    max_entries: int,
) -> list[Candidate]:
    page = client.text(SUITESPARSE_GROUP.format(group=group))
    found: list[Candidate] = []
    for row in re.findall(r"<tr>(.*?)</tr>", page, re.DOTALL):
        name = _cell(row, "column-name")
        rows = _cell(row, "column-num_rows")
        cols = _cell(row, "column-num_cols")
        entries = _cell(row, "column-nonzeros")
        kind = _cell(row, "column-kind d-none d-md-table-cell") or ""
        url_match = re.search(r'href=["\']([^"\']+/MM/[^"\']+\.tar\.gz)', row)
        if not all((name, rows, cols, entries, url_match)):
            continue
        nrows = int(rows.replace(",", ""))
        ncols = int(cols.replace(",", ""))
        nnz = int(entries.replace(",", ""))
        if nrows != ncols or not min_vertices <= nrows <= max_vertices:
            continue
        if nnz > max_entries or "duplicate" in kind.lower():
            continue
        found.append(Candidate(
            source=source,
            family=group,
            name=name,
            url=html.unescape(url_match.group(1)),
            file_format="matrix_market_tar",
            declared_n=nrows,
            declared_entries=nnz,
            kind=kind,
        ))
    return found


def dimacs10_candidates(
    client: HttpClient, min_vertices: int, max_vertices: int, max_entries: int,
) -> list[Candidate]:
    direct = discover_suitesparse(
        client, "DIMACS10", "dimacs10", min_vertices, max_vertices, max_entries
    )
    candidates = direct
    for group, name in DIMACS10_CROSSWALK:
        candidates.append(Candidate(
            source="dimacs10",
            family=f"DIMACS10 via {group}",
            name=name,
            url=SUITESPARSE_MM.format(group=group, name=name),
            file_format="matrix_market_tar",
            kind="DIMACS10 cross-reference",
        ))
    unique: dict[str, Candidate] = {candidate.url: candidate for candidate in candidates}
    return list(unique.values())


def parse_matrix_market(data: bytes) -> RawGraph:
    stream = io.TextIOWrapper(io.BytesIO(data), encoding="utf-8", errors="replace")
    header = stream.readline().strip().lower()
    if not header.startswith("%%matrixmarket matrix coordinate"):
        raise ValueError("apenas Matrix Market no formato coordinate e suportado")
    dimensions = ""
    for line in stream:
        if not line.lstrip().startswith("%") and line.strip():
            dimensions = line
            break
    parts = dimensions.split()
    if len(parts) < 3:
        raise ValueError("dimensoes ausentes no Matrix Market")
    rows, cols, _ = map(int, parts[:3])
    if rows != cols:
        raise ValueError(f"matriz retangular ({rows}x{cols})")
    edges: list[tuple[int, int]] = []
    for line in stream:
        if not line.strip() or line.lstrip().startswith("%"):
            continue
        fields = line.split()
        if len(fields) >= 2:
            edges.append((int(fields[0]) - 1, int(fields[1]) - 1))
    return RawGraph(edges=edges, vertices=set(range(rows)), declared_n=rows)


def parse_edge_list(data: bytes, compressed: bool = False) -> RawGraph:
    if compressed:
        data = gzip.decompress(data)
    text = data.decode("utf-8", errors="replace")
    raw_pairs: list[tuple[str, str]] = []
    declared_n: int | None = None
    for line in text.splitlines():
        stripped = line.strip()
        if not stripped or stripped.startswith(("#", "%")):
            continue
        fields = re.split(r"[\s,;]+", stripped)
        if fields[0].lower() == "p" and len(fields) >= 4:
            try:
                declared_n = int(fields[-2])
            except ValueError:
                pass
            continue
        if fields[0].lower() in {"e", "a"}:
            fields = fields[1:]
        if len(fields) < 2:
            continue
        try:
            int(fields[0])
            int(fields[1])
        except ValueError:
            continue
        raw_pairs.append((fields[0], fields[1]))

    labels = sorted({x for pair in raw_pairs for x in pair}, key=lambda value: int(value))
    label_to_id = {label: index for index, label in enumerate(labels)}
    edges = [(label_to_id[u], label_to_id[v]) for u, v in raw_pairs]
    vertices = set(range(len(labels)))
    # Cabecalhos DIMACS podem declarar vertices isolados. Eles sao representados
    # aqui e tratados pela politica de componentes durante a limpeza.
    if declared_n and declared_n > len(vertices):
        vertices.update(range(declared_n))
    return RawGraph(edges=edges, vertices=vertices, declared_n=declared_n)


def read_matrix_tar(path: Path) -> RawGraph:
    with tarfile.open(path, "r:*") as archive:
        members = [m for m in archive.getmembers() if m.isfile() and m.name.endswith(".mtx")]
        if not members:
            raise ValueError("arquivo SuiteSparse sem .mtx")
        member = min(members, key=lambda item: len(item.name))
        extracted = archive.extractfile(member)
        if extracted is None:
            raise ValueError("nao foi possivel ler o .mtx")
        return parse_matrix_market(extracted.read())


def clean_graph(
    raw: RawGraph, min_vertices: int, max_vertices: int, component_policy: str,
) -> tuple[list[tuple[int, int]], dict[str, int | float | str]]:
    loops = 0
    duplicates = 0
    unique: set[tuple[int, int]] = set()
    vertices = set(raw.vertices)
    for u, v in raw.edges:
        vertices.update((u, v))
        if u == v:
            loops += 1
            continue
        edge = (u, v) if u < v else (v, u)
        if edge in unique:
            duplicates += 1
        else:
            unique.add(edge)

    adjacency: dict[int, list[int]] = {vertex: [] for vertex in vertices}
    for u, v in unique:
        adjacency[u].append(v)
        adjacency[v].append(u)
    components: list[list[int]] = []
    unseen = set(vertices)
    while unseen:
        start = min(unseen)
        unseen.remove(start)
        component: list[int] = []
        queue = deque([start])
        while queue:
            u = queue.popleft()
            component.append(u)
            for v in adjacency[u]:
                if v in unseen:
                    unseen.remove(v)
                    queue.append(v)
        components.append(component)

    if not components:
        raise ValueError("grafo vazio")
    if component_policy == "largest":
        kept = set(max(components, key=lambda c: (len(c), -min(c))))
    else:
        kept = vertices
    mapping = {old: new for new, old in enumerate(sorted(kept))}
    cleaned = sorted(
        (mapping[u], mapping[v]) for u, v in unique if u in kept and v in kept
    )
    n = len(kept)
    if not min_vertices <= n <= max_vertices:
        raise ValueError(f"numero de vertices apos limpeza fora do limite: {n}")
    if not cleaned:
        raise ValueError("grafo sem arestas apos limpeza")
    density = 2.0 * len(cleaned) / (n * (n - 1)) if n > 1 else 0.0
    return cleaned, {
        "raw_vertices": len(vertices),
        "raw_edges": len(raw.edges),
        "vertices": n,
        "edges": len(cleaned),
        "loops_removed": loops,
        "duplicates_removed": duplicates,
        "components_before": len(components),
        "vertices_dropped": len(vertices) - n,
        "density": density,
    }


def size_bin(n: int) -> str:
    if n < 100:
        return "01_10-99"
    if n < 500:
        return "02_100-499"
    if n < 2000:
        return "03_500-1999"
    return "04_2000-4000"


def density_bin(density: float) -> str:
    if density < 0.005:
        return "01_very_sparse"
    if density < 0.05:
        return "02_sparse"
    return "03_dense"


def graph_digest(n: int, edges: Sequence[tuple[int, int]]) -> str:
    digest = hashlib.sha256(f"n={n}\n".encode())
    for u, v in edges:
        digest.update(f"{u} {v}\n".encode())
    return digest.hexdigest()


def _safe_name(name: str) -> str:
    return re.sub(r"[^A-Za-z0-9_.-]+", "_", name).strip("._") or "graph"


def prepare(
    candidate: Candidate,
    raw: RawGraph,
    min_vertices: int,
    max_vertices: int,
    component_policy: str,
) -> PreparedGraph:
    edges, stats = clean_graph(raw, min_vertices, max_vertices, component_policy)
    n = int(stats["vertices"])
    digest = graph_digest(n, edges)
    record = GraphRecord(
        source=candidate.source,
        family=candidate.family,
        original_name=candidate.name,
        source_url=candidate.url,
        file="",
        split="",
        raw_vertices=int(stats["raw_vertices"]),
        raw_edges=int(stats["raw_edges"]),
        vertices=n,
        edges=int(stats["edges"]),
        loops_removed=int(stats["loops_removed"]),
        duplicates_removed=int(stats["duplicates_removed"]),
        components_before=int(stats["components_before"]),
        vertices_dropped=int(stats["vertices_dropped"]),
        density=float(stats["density"]),
        size_bin=size_bin(n),
        density_bin=density_bin(float(stats["density"])),
        graph_sha256=digest,
    )
    return PreparedGraph(candidate, edges, record)


def read_candidate(client: HttpClient, candidate: Candidate) -> RawGraph:
    path = client.download(candidate.url, f"{candidate.family}-{candidate.name}")
    if candidate.file_format == "matrix_market_tar":
        return read_matrix_tar(path)
    data = path.read_bytes()
    if candidate.file_format == "edge_gzip":
        return parse_edge_list(data, compressed=True)
    return parse_edge_list(data)


def evenly_spaced(items: Sequence[str], count: int) -> list[str]:
    if count >= len(items):
        return list(items)
    if count <= 1:
        return [items[len(items) // 2]] if count else []
    indexes = {round(i * (len(items) - 1) / (count - 1)) for i in range(count)}
    return [items[index] for index in sorted(indexes)]


def collect_snap_raw(
    client: HttpClient, wanted: int, reserve: int, min_vertices: int,
    max_vertices: int, seed: int,
) -> Iterator[tuple[Candidate, RawGraph]]:
    for name, filename in SNAP_FILES:
        candidate = Candidate(
            source="snap", family="SNAP", name=name,
            url=f"{SNAP_ROOT}/{filename}", file_format="edge_gzip",
        )
        try:
            yield candidate, read_candidate(client, candidate)
        except requests.HTTPError as error:
            print(f"[aviso] SNAP indisponivel: {filename}: {error}", file=sys.stderr)

    archive_url = f"{SNAP_ROOT}/as-733.tar.gz"
    archive_path = client.download(archive_url, "SNAP-as-733")
    with tarfile.open(archive_path, "r:*") as archive:
        # Reservoir sampling conserva uma amostra temporalmente espalhada sem
        # carregar os 733 grafos simultaneamente. A iteracao sequencial tambem
        # evita descompactar o tar.gz desde o inicio para cada membro.
        sample_size = wanted + reserve
        rng = random.Random(seed)
        reservoir: list[tuple[Candidate, RawGraph]] = []
        eligible = 0
        for member in archive:
            if not member.isfile() or not member.name.lower().endswith(".txt"):
                continue
            stream = archive.extractfile(member)
            if stream is None:
                continue
            member_name = member.name
            name = Path(member_name).stem
            candidate = Candidate(
                source="snap", family="SNAP/as-733", name=name,
                url=f"{archive_url}#{member_name}", file_format="edge_list",
            )
            raw = parse_edge_list(stream.read())
            active_vertices = len(raw.vertices)
            if not min_vertices <= active_vertices <= max_vertices:
                continue
            eligible += 1
            item = (candidate, raw)
            if len(reservoir) < sample_size:
                reservoir.append(item)
            else:
                replacement = rng.randrange(eligible)
                if replacement < sample_size:
                    reservoir[replacement] = item
        yield from sorted(reservoir, key=lambda item: item[0].name)


def fast_gnp_edges(n: int, probability: float, rng: random.Random) -> set[tuple[int, int]]:
    """Batagelj-Brandes: gera G(n,p) esperado em O(n+m), sem testar todo par."""
    edges: set[tuple[int, int]] = set()
    if probability <= 0.0:
        return edges
    if probability >= 1.0:
        return {(u, v) for v in range(1, n) for u in range(v)}
    log_q = math.log1p(-probability)
    v, w = 1, -1
    while v < n:
        r = max(rng.random(), sys.float_info.min)
        w = w + 1 + int(math.log(r) / log_q)
        while w >= v and v < n:
            w -= v
            v += 1
        if v < n:
            edges.add((w, v))
    return edges


def connected_gilbert(n: int, probability: float, seed: int) -> RawGraph:
    rng = random.Random(seed)
    edges = fast_gnp_edges(n, probability, rng)
    order = list(range(n))
    rng.shuffle(order)
    for index in range(1, n):
        parent = order[rng.randrange(index)]
        child = order[index]
        edges.add((min(parent, child), max(parent, child)))
    return RawGraph(edges=sorted(edges), vertices=set(range(n)), declared_n=n)


def random_candidates(seed: int) -> Iterator[tuple[Candidate, RawGraph]]:
    serial = 0
    for n in (50, 100, 200, 500, 1000, 2000, 3000, 4000):
        for average_degree in (3, 6, 12, 24):
            for replicate in range(2):
                probability = min(1.0, average_degree / max(1, n - 1))
                graph_seed = seed + serial * 104729
                name = f"gilbert_n{n}_d{average_degree}_r{replicate + 1}"
                candidate = Candidate(
                    source="random", family="Gilbert conectado", name=name,
                    url=f"generated://gilbert?n={n}&p={probability:.12g}&seed={graph_seed}",
                    file_format="generated", declared_n=n,
                    kind="G(n,p) acrescido de arvore geradora aleatoria",
                )
                yield candidate, connected_gilbert(n, probability, graph_seed)
                serial += 1
    for n in (50, 100, 200, 500):
        for probability in (0.02, 0.05, 0.10, 0.30):
            graph_seed = seed + serial * 104729
            name = f"gilbert_n{n}_p{int(probability * 100):02d}"
            candidate = Candidate(
                source="random", family="Gilbert conectado", name=name,
                url=f"generated://gilbert?n={n}&p={probability}&seed={graph_seed}",
                file_format="generated", declared_n=n,
                kind="G(n,p) acrescido de arvore geradora aleatoria",
            )
            yield candidate, connected_gilbert(n, probability, graph_seed)
            serial += 1


def metadata_diverse_order(candidates: Sequence[Candidate], seed: int) -> list[Candidate]:
    buckets: dict[tuple[str, str], list[Candidate]] = defaultdict(list)
    rng = random.Random(seed)
    for candidate in candidates:
        n = candidate.declared_n or 0
        buckets[(size_bin(max(10, n)), candidate.kind or candidate.family)].append(candidate)
    for values in buckets.values():
        rng.shuffle(values)
    keys = sorted(buckets)
    result: list[Candidate] = []
    while keys:
        next_keys: list[tuple[str, str]] = []
        for key in keys:
            values = buckets[key]
            if values:
                result.append(values.pop())
            if values:
                next_keys.append(key)
        keys = next_keys
    return result


def diverse_select(graphs: Sequence[PreparedGraph], count: int, seed: int) -> list[PreparedGraph]:
    buckets: dict[tuple[str, str, str], list[PreparedGraph]] = defaultdict(list)
    rng = random.Random(seed)
    for graph in graphs:
        key = (graph.record.size_bin, graph.record.density_bin, graph.record.family)
        buckets[key].append(graph)
    for values in buckets.values():
        rng.shuffle(values)
    keys = list(buckets)
    rng.shuffle(keys)
    selected: list[PreparedGraph] = []
    while keys and len(selected) < count:
        remaining: list[tuple[str, str, str]] = []
        for key in keys:
            if len(selected) >= count:
                break
            selected.append(buckets[key].pop())
            if buckets[key]:
                remaining.append(key)
        keys = remaining
    return selected


def scaled_targets(total: int) -> dict[str, int]:
    weight_sum = sum(DEFAULT_WEIGHTS.values())
    raw = {key: total * value / weight_sum for key, value in DEFAULT_WEIGHTS.items()}
    result = {key: int(value) for key, value in raw.items()}
    for key in sorted(raw, key=lambda item: (raw[item] - result[item], item), reverse=True):
        if sum(result.values()) == total:
            break
        result[key] += 1
    return result


def parse_targets(value: str | None, total: int) -> dict[str, int]:
    if not value:
        return scaled_targets(total)
    result = {key: 0 for key in DEFAULT_WEIGHTS}
    for item in value.split(","):
        name, raw_count = item.split("=", 1)
        if name not in result:
            raise ValueError(f"fonte desconhecida: {name}")
        result[name] = int(raw_count)
    if sum(result.values()) != total:
        raise ValueError("a soma de --source-targets deve ser igual a --target")
    return result


def split_stratified(
    graphs: Sequence[PreparedGraph], test_count: int, seed: int,
) -> tuple[list[PreparedGraph], list[PreparedGraph]]:
    strata: dict[tuple[str, str, str], list[PreparedGraph]] = defaultdict(list)
    rng = random.Random(seed)
    for graph in graphs:
        key = (graph.record.source, graph.record.size_bin, graph.record.density_bin)
        strata[key].append(graph)
    for values in strata.values():
        rng.shuffle(values)
    ratio = test_count / len(graphs)
    allocations = {key: min(len(values), int(len(values) * ratio)) for key, values in strata.items()}
    remaining = test_count - sum(allocations.values())
    ranked = sorted(
        strata,
        key=lambda key: (
            len(strata[key]) * ratio - allocations[key], len(strata[key]), key
        ),
        reverse=True,
    )
    while remaining:
        changed = False
        for key in ranked:
            if remaining == 0:
                break
            if allocations[key] < len(strata[key]):
                allocations[key] += 1
                remaining -= 1
                changed = True
        if not changed:
            raise RuntimeError("nao foi possivel completar a divisao estratificada")
    test: list[PreparedGraph] = []
    final: list[PreparedGraph] = []
    for key in sorted(strata):
        cut = allocations[key]
        test.extend(strata[key][:cut])
        final.extend(strata[key][cut:])
    return test, final


def write_graph(path: Path, edges: Sequence[tuple[int, int]]) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    with path.open("w", encoding="utf-8", newline="\n") as output:
        for u, v in edges:
            output.write(f"{u} {v}\n")


def link_or_copy(source: Path, target: Path) -> None:
    target.parent.mkdir(parents=True, exist_ok=True)
    try:
        os.link(source, target)
    except OSError:
        shutil.copy2(source, target)


def write_outputs(output: Path, graphs: Sequence[PreparedGraph], test_count: int, seed: int) -> None:
    test, final = split_stratified(graphs, test_count, seed)
    membership = {graph.record.graph_sha256: "irace" for graph in test}
    membership.update({graph.record.graph_sha256: "final" for graph in final})
    used_names: set[str] = set()
    for index, graph in enumerate(sorted(graphs, key=lambda g: (g.record.source, g.record.original_name)), 1):
        base = f"{graph.record.source}__{_safe_name(graph.record.original_name)}"
        filename = f"{base}.txt"
        if filename in used_names:
            filename = f"{base}__{graph.record.graph_sha256[:8]}.txt"
        used_names.add(filename)
        clean_path = output / "clean" / graph.record.source / filename
        write_graph(clean_path, graph.edges)
        split = membership[graph.record.graph_sha256]
        split_path = output / "splits" / split / filename
        link_or_copy(clean_path, split_path)
        graph.record.file = str(clean_path.relative_to(output))
        graph.record.split = split

    records = [asdict(graph.record) for graph in graphs]
    fields = list(GraphRecord.__dataclass_fields__)
    with (output / "manifest.csv").open("w", encoding="utf-8", newline="") as stream:
        writer = csv.DictWriter(stream, fieldnames=fields)
        writer.writeheader()
        writer.writerows(records)
    (output / "manifest.json").write_text(
        json.dumps(records, ensure_ascii=False, indent=2) + "\n", encoding="utf-8"
    )
    by_source = defaultdict(int)
    by_split = defaultdict(int)
    by_size = defaultdict(int)
    for graph in graphs:
        by_source[graph.record.source] += 1
        by_split[graph.record.split] += 1
        by_size[graph.record.size_bin] += 1
    summary = {
        "total": len(graphs),
        "by_source": dict(sorted(by_source.items())),
        "by_split": dict(sorted(by_split.items())),
        "by_size": dict(sorted(by_size.items())),
        "seed": seed,
        "format": "uma aresta nao direcionada por linha: u v; vertices zero-based",
    }
    (output / "summary.json").write_text(
        json.dumps(summary, ensure_ascii=False, indent=2) + "\n", encoding="utf-8"
    )


def build(args: argparse.Namespace) -> None:
    output = args.output.resolve()
    if output.exists() and any(output.iterdir()):
        if not args.overwrite:
            raise SystemExit(f"saida nao esta vazia: {output}; use --overwrite conscientemente")
        shutil.rmtree(output)
    output.mkdir(parents=True, exist_ok=True)
    client = HttpClient(output / "raw", args.timeout)
    targets = parse_targets(args.source_targets, args.target)
    reserve = max(5, math.ceil(args.target * 0.08))
    pools: dict[str, list[PreparedGraph]] = defaultdict(list)
    rejected: list[dict[str, str]] = []
    digests: set[str] = set()

    def accept(candidate: Candidate, raw: RawGraph) -> None:
        try:
            graph = prepare(
                candidate, raw, args.min_vertices, args.max_vertices, args.component_policy
            )
            if graph.record.edges > args.max_edges:
                raise ValueError(f"arestas acima do limite: {graph.record.edges}")
            if graph.record.graph_sha256 in digests:
                raise ValueError("duplicado estrutural de outro grafo")
            digests.add(graph.record.graph_sha256)
            pools[candidate.source].append(graph)
        except Exception as error:  # registra um item ruim sem perder a coleta inteira
            rejected.append({
                "source": candidate.source,
                "name": candidate.name,
                "url": candidate.url,
                "reason": str(error),
            })

    if targets["dimacs10"]:
        print("[1/4] Coletando catalogo DIMACS10...", flush=True)
        for candidate in dimacs10_candidates(
            client, args.min_vertices, args.max_vertices, args.max_edges * 2
        ):
            try:
                accept(candidate, read_candidate(client, candidate))
            except Exception as error:
                rejected.append({"source": candidate.source, "name": candidate.name,
                                 "url": candidate.url, "reason": str(error)})

    if targets["snap"]:
        print("[2/4] Coletando redes SNAP...", flush=True)
        for candidate, raw in collect_snap_raw(
            client, targets["snap"], reserve, args.min_vertices,
            args.max_vertices, args.seed,
        ):
            accept(candidate, raw)

    if targets["harwell_boeing"]:
        print("[3/4] Coletando catalogo Harwell-Boeing/SuiteSparse...", flush=True)
        catalog = discover_suitesparse(
            client, "HB", "harwell_boeing", args.min_vertices,
            args.max_vertices, args.max_edges * 2,
        )
        for candidate in metadata_diverse_order(catalog, args.seed):
            if len(pools["harwell_boeing"]) >= targets["harwell_boeing"] + reserve:
                break
            try:
                accept(candidate, read_candidate(client, candidate))
            except Exception as error:
                rejected.append({"source": candidate.source, "name": candidate.name,
                                 "url": candidate.url, "reason": str(error)})

    if targets["random"]:
        print("[4/4] Gerando grafos aleatorios...", flush=True)
        for candidate, raw in random_candidates(args.seed):
            accept(candidate, raw)

    selected: list[PreparedGraph] = []
    leftovers: list[PreparedGraph] = []
    for offset, source in enumerate(DEFAULT_WEIGHTS):
        chosen = diverse_select(pools[source], min(targets[source], len(pools[source])), args.seed + offset)
        selected.extend(chosen)
        chosen_ids = {id(graph) for graph in chosen}
        leftovers.extend(graph for graph in pools[source] if id(graph) not in chosen_ids)
    deficit = args.target - len(selected)
    if deficit:
        extras = diverse_select(leftovers, deficit, args.seed + 999)
        selected.extend(extras)
    if len(selected) != args.target:
        counts = {source: len(pool) for source, pool in pools.items()}
        raise RuntimeError(
            f"foram obtidos {len(selected)}/{args.target} grafos; disponiveis por fonte: {counts}"
        )

    test_count = round(args.target * args.irace_fraction)
    write_outputs(output, selected, test_count, args.seed)
    with (output / "rejections.csv").open("w", encoding="utf-8", newline="") as stream:
        writer = csv.DictWriter(stream, fieldnames=("source", "name", "url", "reason"))
        writer.writeheader()
        writer.writerows(rejected)
    print(json.dumps(json.loads((output / "summary.json").read_text()), indent=2))


def argument_parser() -> argparse.ArgumentParser:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--output", type=Path, default=Path("graph_dataset"))
    parser.add_argument("--target", type=int, default=250)
    parser.add_argument("--source-targets", help=(
        "cotas separadas por virgula; padrao para 250: "
        "harwell_boeing=100,random=80,snap=50,dimacs10=20"
    ))
    parser.add_argument("--min-vertices", type=int, default=10)
    parser.add_argument("--max-vertices", type=int, default=4000)
    parser.add_argument("--max-edges", type=int, default=2_000_000)
    parser.add_argument("--irace-fraction", type=float, default=0.30)
    parser.add_argument("--component-policy", choices=("largest", "all"), default="largest")
    parser.add_argument("--seed", type=int, default=42)
    parser.add_argument("--timeout", type=int, default=120)
    parser.add_argument("--overwrite", action="store_true")
    return parser


def main() -> None:
    args = argument_parser().parse_args()
    if args.target <= 0:
        raise SystemExit("--target deve ser positivo")
    if not 0.0 <= args.irace_fraction <= 1.0:
        raise SystemExit("--irace-fraction deve estar entre 0 e 1")
    if args.min_vertices < 2 or args.max_vertices < args.min_vertices:
        raise SystemExit("limites de vertices invalidos")
    started = time.monotonic()
    build(args)
    print(f"Concluido em {time.monotonic() - started:.1f}s")


if __name__ == "__main__":
    main()

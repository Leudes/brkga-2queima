import importlib.util
import tempfile
import unittest
from pathlib import Path


SCRIPT = Path(__file__).parents[1] / "scripts" / "build_graph_dataset.py"
SPEC = importlib.util.spec_from_file_location("build_graph_dataset", SCRIPT)
dataset = importlib.util.module_from_spec(SPEC)
assert SPEC.loader is not None
import sys
sys.modules[SPEC.name] = dataset
SPEC.loader.exec_module(dataset)


class DatasetBuilderTests(unittest.TestCase):
    def test_matrix_market_and_cleaning(self):
        raw = dataset.parse_matrix_market(b"""%%MatrixMarket matrix coordinate real symmetric
% test
5 5 6
1 1 2
1 2 1
2 1 1
2 3 1
4 5 1
5 4 1
""")
        edges, stats = dataset.clean_graph(raw, 2, 10, "largest")
        self.assertEqual(edges, [(0, 1), (1, 2)])
        self.assertEqual(stats["loops_removed"], 1)
        self.assertEqual(stats["duplicates_removed"], 2)
        self.assertEqual(stats["components_before"], 2)
        self.assertEqual(stats["vertices_dropped"], 2)

    def test_edge_list_maps_sparse_labels_and_removes_direction(self):
        raw = dataset.parse_edge_list(b"10 20 1\n20 10 -1\n20 30 123\n")
        edges, stats = dataset.clean_graph(raw, 2, 10, "largest")
        self.assertEqual(edges, [(0, 1), (1, 2)])
        self.assertEqual(stats["duplicates_removed"], 1)

    def test_random_graph_is_connected_and_reproducible(self):
        first = dataset.connected_gilbert(100, 0.02, 42)
        second = dataset.connected_gilbert(100, 0.02, 42)
        self.assertEqual(first.edges, second.edges)
        edges, stats = dataset.clean_graph(first, 10, 100, "largest")
        self.assertEqual(stats["vertices"], 100)
        self.assertGreaterEqual(len(edges), 99)
        self.assertEqual(stats["components_before"], 1)

    def test_split_has_exact_size_and_no_overlap(self):
        graphs = []
        for index in range(20):
            candidate = dataset.Candidate("random", "test", str(index), "generated://", "generated")
            raw = dataset.connected_gilbert(20 + index, 0.1, index)
            graphs.append(dataset.prepare(candidate, raw, 10, 100, "largest"))
        test, final = dataset.split_stratified(graphs, 6, 42)
        self.assertEqual((len(test), len(final)), (6, 14))
        self.assertFalse({id(x) for x in test} & {id(x) for x in final})

    def test_evenly_spaced(self):
        self.assertEqual(dataset.evenly_spaced(list("abcdef"), 3), ["a", "c", "f"])

    def test_write_format_is_accepted_plain_edge_list(self):
        with tempfile.TemporaryDirectory() as directory:
            path = Path(directory) / "g.txt"
            dataset.write_graph(path, [(0, 1), (1, 2)])
            self.assertEqual(path.read_text(), "0 1\n1 2\n")


if __name__ == "__main__":
    unittest.main()

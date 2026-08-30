// gerar_grafos.cpp
//
// Gera um conjunto misto de 10 grafos pequenos (n <= 200) e salva cada um como
// lista de arestas (um par "u v" por linha), no formato lido por Graph.cpp.
//
// O conjunto contempla tres tipos de instancia, para produzir uma tabela de
// resultados variada e capaz de discriminar o comportamento do BRKGA:
//
//   (1) Aleatorios densos  -> G(n, p=0.3). A propagacao por 2 vizinhos e
//       trivial e b2 fica proximo do minimo teorico (b2 = 4).
//   (2) Aleatorios esparsos -> G(n, p) com grau medio baixo (p ~ c/n). A
//       propagacao indireta e dificil, gerando valores de b2 maiores e variados.
//   (3) Estruturados -> caminhos, ciclos e grades, cujos valores de 2-queima
//       sao conhecidos ou bem comportados, permitindo comparacao com a teoria.
//       Para caminhos e ciclos: b2(P_n) = b2(C_n) = ceil(n/2) + 1.
//
// Todos os grafos aleatorios sao garantidamente conexos (arvore geradora antes
// do sorteio das demais arestas), evitando vertices isolados, que nunca queimam
// por queima indireta.
//
// Compilar:  g++ -O3 -std=c++17 -o gerar_grafos gerar_grafos.cpp
// Executar:  ./gerar_grafos

#include <iostream>
#include <fstream>
#include <vector>
#include <set>
#include <utility>
#include <random>
#include <numeric>
#include <algorithm>
#include <string>
#include <iomanip>
#include <cmath>
#include <sys/stat.h>

// Pasta de saida
static const std::string OUT_DIR = "instancias";

using Aresta = std::pair<int,int>;
using ConjArestas = std::set<Aresta>;

static inline Aresta aresta(int u, int v) {
    return {std::min(u, v), std::max(u, v)};
}

// ---------------------------------------------------------------------------
// Geradores
// ---------------------------------------------------------------------------

// Conecta o grafo adicionando uma arvore geradora aleatoria sobre n vertices.
void adicionar_arvore_geradora(int n, ConjArestas &arestas, std::mt19937 &rng) {
    std::vector<int> vertices(n);
    std::iota(vertices.begin(), vertices.end(), 0);
    std::shuffle(vertices.begin(), vertices.end(), rng);
    for (int i = 1; i < n; ++i) {
        std::uniform_int_distribution<int> ant(0, i - 1);
        int u = vertices[i];
        int v = vertices[ant(rng)];
        arestas.insert(aresta(u, v));
    }
}

// G(n, p) conexo (modelo de Erdos-Renyi).
ConjArestas gerar_aleatorio(int n, double p, std::mt19937 &rng) {
    ConjArestas arestas;
    adicionar_arvore_geradora(n, arestas, rng);

    std::uniform_real_distribution<double> moeda(0.0, 1.0);
    for (int u = 0; u < n; ++u) {
        for (int v = u + 1; v < n; ++v) {
            if (arestas.find(aresta(u, v)) == arestas.end() && moeda(rng) < p) {
                arestas.insert(aresta(u, v));
            }
        }
    }
    return arestas;
}

// G(n, p) esparso com grau medio alvo "grau_medio" (p = grau_medio / (n-1)).
ConjArestas gerar_esparso(int n, double grau_medio, std::mt19937 &rng) {
    double p = grau_medio / static_cast<double>(n - 1);
    return gerar_aleatorio(n, p, rng);
}

// Caminho P_n: 0-1-2-...-(n-1).
ConjArestas gerar_caminho(int n) {
    ConjArestas arestas;
    for (int i = 0; i + 1 < n; ++i) {
        arestas.insert(aresta(i, i + 1));
    }
    return arestas;
}

// Ciclo C_n: caminho fechado.
ConjArestas gerar_ciclo(int n) {
    ConjArestas arestas = gerar_caminho(n);
    arestas.insert(aresta(n - 1, 0));
    return arestas;
}

// Grade a x b (grid graph). Vertice (i,j) tem indice i*b + j.
ConjArestas gerar_grade(int a, int b) {
    ConjArestas arestas;
    auto idx = [b](int i, int j) { return i * b + j; };
    for (int i = 0; i < a; ++i) {
        for (int j = 0; j < b; ++j) {
            if (j + 1 < b) arestas.insert(aresta(idx(i, j), idx(i, j + 1)));
            if (i + 1 < a) arestas.insert(aresta(idx(i, j), idx(i + 1, j)));
        }
    }
    return arestas;
}

// ---------------------------------------------------------------------------
// Escrita
// ---------------------------------------------------------------------------

void salvar(const std::string &nome, int n, const ConjArestas &arestas) {
    std::string caminho = OUT_DIR + "/" + nome;
    std::ofstream out(caminho);
    if (!out) {
        std::cerr << "Erro ao abrir " << caminho << " para escrita\n";
        std::exit(1);
    }
    for (const auto &e : arestas) {
        out << e.first << " " << e.second << "\n";
    }
    out.close();

    int m = static_cast<int>(arestas.size());
    double densidade = (2.0 * m) / (static_cast<double>(n) * (n - 1));
    std::cout << std::left << std::setw(26) << nome
              << std::right << std::setw(5) << n
              << std::setw(8) << m
              << std::setw(12) << std::fixed << std::setprecision(4) << densidade
              << "\n";
}

// ---------------------------------------------------------------------------
// Main
// ---------------------------------------------------------------------------

int main() {
    // Reprodutibilidade: muda esta semente se quiser outros grafos aleatorios
    std::mt19937 rng(42);

    mkdir(OUT_DIR.c_str(), 0755);

    std::cout << "Gerando conjunto misto de 10 grafos em '" << OUT_DIR << "/'\n\n";
    std::cout << std::left << std::setw(26) << "arquivo"
              << std::right << std::setw(5) << "n"
              << std::setw(8) << "m"
              << std::setw(12) << "densidade" << "\n";
    std::cout << std::string(51, '-') << "\n";

    // (1) Aleatorios densos: G(n, 0.3)
    std::cout << "\n-- aleatorios densos (p = 0.3) --\n";
    salvar("random_denso_n50_p30.txt",  50,  gerar_aleatorio(50,  0.3, rng));
    salvar("random_denso_n100_p30.txt", 100, gerar_aleatorio(100, 0.3, rng));
    salvar("random_denso_n200_p30.txt", 200, gerar_aleatorio(200, 0.3, rng));

    // (2) Aleatorios esparsos: grau medio baixo (proximo do limiar de percolacao)
    std::cout << "\n-- aleatorios esparsos (grau medio baixo) --\n";
    salvar("random_esparso_n50_d3.txt",  50,  gerar_esparso(50,  3.0, rng));
    salvar("random_esparso_n100_d3.txt", 100, gerar_esparso(100, 3.0, rng));
    salvar("random_esparso_n200_d3.txt", 200, gerar_esparso(200, 3.0, rng));

    // (3) Estruturados: caminho, ciclo e grades
    std::cout << "\n-- estruturados --\n";
    salvar("caminho_n100.txt", 100, gerar_caminho(100));   // b2 = ceil(100/2)+1 = 51
    salvar("ciclo_n100.txt",   100, gerar_ciclo(100));     // b2 = ceil(100/2)+1 = 51
    salvar("grade_10x10.txt",  100, gerar_grade(10, 10));  // 100 vertices
    salvar("grade_10x20.txt",  200, gerar_grade(10, 20));  // 200 vertices

    std::cout << "\nPronto. 10 arquivos salvos em '" << OUT_DIR << "/'.\n";
    return 0;
}
#include "Decoder2QueimaV4.h"
#include "Graph.h"

#include <cassert>
#include <iostream>
#include <stdexcept>
#include <vector>

void conferir_caso(
    Graph& grafo,
    const std::vector<double>& cromossomo,
    int fitness_esperado,
    const std::vector<int>& sequencia_esperada) {

    Decodificador2Queima decoder(grafo);

    assert(decoder.decode(cromossomo) == fitness_esperado);
    assert(
        decoder.obter_sequencia_queima(cromossomo) ==
        sequencia_esperada);
}

Graph criar_ciclo(int quantidade_vertices) {
    Graph grafo;

    for (int vertice = 0; vertice < quantidade_vertices; ++vertice) {
        grafo.addEdge(
            vertice,
            (vertice + 1) % quantidade_vertices);
    }

    return grafo;
}

Graph criar_grade_tres_por_tres() {
    Graph grafo;

    for (int linha = 0; linha < 3; ++linha) {
        for (int coluna = 0; coluna < 3; ++coluna) {
            int vertice = linha * 3 + coluna;

            if (linha + 1 < 3) {
                grafo.addEdge(vertice, vertice + 3);
            }

            if (coluna + 1 < 3) {
                grafo.addEdge(vertice, vertice + 1);
            }
        }
    }

    return grafo;
}

Graph criar_arvore() {
    Graph grafo;

    grafo.addEdge(0, 1);
    grafo.addEdge(0, 2);
    grafo.addEdge(0, 3);
    grafo.addEdge(1, 4);
    grafo.addEdge(1, 5);
    grafo.addEdge(2, 6);
    grafo.addEdge(2, 7);
    grafo.addEdge(3, 8);
    grafo.addEdge(3, 9);

    return grafo;
}

Graph criar_caso_gatilho_com_propagacao_programada() {
    Graph grafo;

    grafo.addEdge(0, 2);
    grafo.addEdge(1, 5);
    grafo.addEdge(2, 5);
    grafo.addEdge(3, 4);
    grafo.addEdge(3, 5);
    grafo.addEdge(4, 5);

    return grafo;
}

int main() {
    Graph ciclo = criar_ciclo(8);

    conferir_caso(
        ciclo,
        {0.49, 0.41, 0.33, 0.25, 0.17, 0.09, 0.05, 0.01},
        5,
        {0, 2, 4, 6});

    // Chaves iguais verificam se o desempate continua igual ao da V4
    // anterior, mesmo depois da inclusao da fila de prioridade.
    conferir_caso(
        ciclo,
        std::vector<double>(8, 0.25),
        5,
        {0, 2, 6, 4});

    Graph grade = criar_grade_tres_por_tres();

    conferir_caso(
        grade,
        {0.12, 0.48, 0.31, 0.07, 0.44, 0.28, 0.19, 0.36, 0.03},
        5,
        {1, 5, 6, 0});

    conferir_caso(
        grade,
        std::vector<double>(9, 0.25),
        5,
        {0, 4, 8, 2});

    Graph arvore = criar_arvore();

    conferir_caso(
        arvore,
        {0.21, 0.18, 0.42, 0.39, 0.06, 0.33, 0.15, 0.48, 0.09, 0.27},
        7,
        {7, 0, 5, 9, 6, 8, 4});

    // Depois que as fontes selecionadas acabam, o gatilho continua sendo
    // consultado mesmo quando ja existe propagacao para a proxima rodada.
    // A regra antiga escolheria {4, 1, 3, 0} e terminaria em 5 rodadas.
    Graph caso_gatilho = criar_caso_gatilho_com_propagacao_programada();

    conferir_caso(
        caso_gatilho,
        {0.3393, 0.4687, 0.2191, 0.4591, 0.4841, 0.4680},
        4,
        {4, 1, 0});

    bool cromossomo_invalido_rejeitado = false;

    try {
        Decodificador2Queima decoder(ciclo);
        decoder.decode({0.1, 0.2});
    } catch (const std::invalid_argument&) {
        cromossomo_invalido_rejeitado = true;
    }

    assert(cromossomo_invalido_rejeitado);

    std::cout << "Teste do decoder V4: OK\n";
    return 0;
}

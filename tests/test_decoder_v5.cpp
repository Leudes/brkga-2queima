#include "Decoder2QueimaV5.h"
#include "Graph.h"

#include <cassert>
#include <iostream>
#include <stdexcept>
#include <vector>

Graph criar_caminho() {
    Graph grafo;
    grafo.addEdge(0, 1);
    grafo.addEdge(1, 2);
    grafo.addEdge(2, 3);
    grafo.addEdge(3, 4);
    return grafo;
}

int main() {
    Graph grafo = criar_caminho();
    Decodificador2Queima decoder(grafo);

    // Como todas as chaves estao abaixo de 0,5, este caso entra diretamente
    // no reparo combinado e testa as atualizacoes incrementais da fila.
    std::vector<double> cromossomo = {0.10, 0.40, 0.20, 0.30, 0.05};
    assert(decoder.decode(cromossomo) == 4);
    assert(
        decoder.obter_sequencia_queima(cromossomo) ==
        std::vector<int>({1, 3, 0, 4}));

    bool cromossomo_invalido_rejeitado = false;
    try {
        decoder.decode({0.1, 0.2});
    } catch (const std::invalid_argument&) {
        cromossomo_invalido_rejeitado = true;
    }
    assert(cromossomo_invalido_rejeitado);

    std::cout << "Teste do decoder V5: OK\n";
    return 0;
}

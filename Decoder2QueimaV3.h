#ifndef DECODER_2QUEIMA_V3_H
#define DECODER_2QUEIMA_V3_H

#include "Graph.h"
#include <vector>

// Decoder de grau relativo: em cada rodada, a queima direta prioriza o
// vertice com mais vizinhos ainda nao queimados. A chave aleatoria do BRKGA
// desempata vertices com o mesmo grau relativo.
class Decodificador2Queima {
public:
    explicit Decodificador2Queima(const Graph& grafo_recebido)
        : grafo{grafo_recebido} {}
    ~Decodificador2Queima() = default;

    double decode(const std::vector<double>& cromossomo) const;
    std::vector<int> obter_sequencia_queima(
        const std::vector<double>& cromossomo) const;

private:
    const Graph& grafo;
};

#endif

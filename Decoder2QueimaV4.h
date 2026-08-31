#ifndef DECODER_2QUEIMA_V4_H
#define DECODER_2QUEIMA_V4_H

#include "Graph.h"
#include <vector>

// V4: quando a propagacao vai parar, prioriza a queima direta que faz mais
// vertices atingirem imediatamente o limiar de dois vizinhos queimados.
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

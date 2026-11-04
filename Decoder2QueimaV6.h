#ifndef DECODER_2QUEIMA_V6_H
#define DECODER_2QUEIMA_V6_H

#include "Graph.h"

#include <vector>

// V6: o cromossomo representa somente uma permutacao de todos os vertices.
// Em cada rodada, o primeiro vertice ainda nao queimado dessa ordem vira fonte.
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

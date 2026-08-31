#ifndef DECODER_2QUEIMA_V2_H
#define DECODER_2QUEIMA_V2_H

#include "Graph.h"
#include <vector>

// V2: usa as chaves do cromossomo e completa uma sequencia insuficiente com
// os vertices de maior grau original.
class Decodificador2Queima {
public:
    explicit Decodificador2Queima(const Graph& grafo_recebido);
    ~Decodificador2Queima() = default;

    double decode(const std::vector<double>& cromossomo) const;
    std::vector<int> obter_sequencia_queima(
        const std::vector<double>& cromossomo) const;

private:
    const Graph& grafo;
    std::vector<int> vertices_ordenados_por_grau;
};

#endif

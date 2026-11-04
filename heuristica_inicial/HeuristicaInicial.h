#ifndef HEURISTICA_INICIAL_H
#define HEURISTICA_INICIAL_H

#include "Graph.h"

#include <cstddef>
#include <vector>

struct SolucaoInicial {
    int numero_de_rodadas = 0;
    std::vector<int> fontes_de_queima;
};

SolucaoInicial construir_por_grau_relativo(const Graph& grafo);

SolucaoInicial construir_por_grau_relativo_e_gatilho(const Graph& grafo);

// Converte a sequencia de fontes para o formato de chaves aleatorias usado
// pelos cinco decodificadores do BRKGA.
std::vector<double> criar_cromossomo_da_solucao(
    std::size_t numero_vertices,
    const std::vector<int>& fontes_de_queima);

// Recalcula o valor de uma sequencia sem usar a heuristica que a produziu.
// Essa funcao tambem podera ser usada para conferir solucoes do futuro GVNS.
int avaliar_solucao(
    const Graph& grafo,
    const std::vector<int>& fontes_de_queima);

#endif

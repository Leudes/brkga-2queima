#include "Decoder2QueimaV2.h"
#include "SimulacaoQueima.h"

#include <algorithm>
#include <numeric>

namespace {

struct MemoriaAuxiliar {
    EstadoQueima estado;
    std::vector<int> ordem_cromossomo;
};

// Cada thread possui sua própria memória para que as avaliações paralelas do
// BRKGA não misturem seus dados.
thread_local MemoriaAuxiliar memoria;

int proximo_vertice_de_reparo(
    const std::vector<int>& vertices_por_grau,
    const std::vector<uint8_t>& queimado,
    size_t& proxima_posicao) {

    while (proxima_posicao < vertices_por_grau.size() &&
           queimado[vertices_por_grau[proxima_posicao]]) {
        proxima_posicao++;
    }

    if (proxima_posicao == vertices_por_grau.size()) {
        return -1;
    }

    int escolhido = vertices_por_grau[proxima_posicao];
    proxima_posicao++;
    return escolhido;
}

double simular(
    const Graph& grafo,
    const std::vector<int>& vertices_por_grau,
    const std::vector<double>& cromossomo,
    std::vector<int>* sequencia_queima) {

    validar_cromossomo(grafo, cromossomo);
    memoria.estado.reiniciar(grafo.getOrder());

    preparar_ordem_do_cromossomo(
        memoria.ordem_cromossomo,
        cromossomo,
        grafo.getOrder(),
        true);

    if (sequencia_queima != nullptr) {
        sequencia_queima->clear();
        sequencia_queima->reserve(grafo.getOrder());
    }

    size_t posicao_cromossomo = 0;
    size_t posicao_reparo = 0;
    int rodadas = 0;

    while (!memoria.estado.todos_queimados()) {
        rodadas++;
        propagar_fogo(grafo, memoria.estado);

        if (memoria.estado.todos_queimados()) {
            break;
        }

        int escolhido = proximo_vertice_nao_queimado(
            memoria.ordem_cromossomo,
            posicao_cromossomo,
            memoria.estado.queimado);

        if (escolhido == -1) {
            escolhido = proximo_vertice_de_reparo(
                vertices_por_grau,
                memoria.estado.queimado,
                posicao_reparo);
        }

        if (escolhido == -1) {
            break;
        }

        queimar_vertice(grafo, escolhido, memoria.estado);

        if (sequencia_queima != nullptr) {
            sequencia_queima->push_back(escolhido);
        }

        terminar_rodada(memoria.estado);
    }

    return rodadas;
}

}  // namespace

Decodificador2Queima::Decodificador2Queima(const Graph& grafo_recebido)
    : grafo{grafo_recebido} {
    vertices_ordenados_por_grau.resize(grafo.getOrder());

    std::iota(
        vertices_ordenados_por_grau.begin(),
        vertices_ordenados_por_grau.end(),
        0);

    std::sort(
        vertices_ordenados_por_grau.begin(),
        vertices_ordenados_por_grau.end(),
        [&](int primeiro, int segundo) {
            return grafo.getVertexDegree(primeiro) >
                grafo.getVertexDegree(segundo);
        });
}

double Decodificador2Queima::decode(
    const std::vector<double>& cromossomo) const {
    return simular(
        grafo,
        vertices_ordenados_por_grau,
        cromossomo,
        nullptr);
}

std::vector<int> Decodificador2Queima::obter_sequencia_queima(
    const std::vector<double>& cromossomo) const {
    std::vector<int> sequencia;
    simular(
        grafo,
        vertices_ordenados_por_grau,
        cromossomo,
        &sequencia);
    return sequencia;
}

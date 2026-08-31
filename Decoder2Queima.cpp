#include "Decoder2Queima.h"
#include "SimulacaoQueima.h"

namespace {

struct MemoriaAuxiliar {
    EstadoQueima estado;
    std::vector<int> ordem_cromossomo;
};

// Cada thread possui sua própria memória para evitar novas alocações durante
// as muitas avaliações feitas pelo BRKGA.
thread_local MemoriaAuxiliar memoria;

double simular(
    const Graph& grafo,
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

    size_t proxima_posicao = 0;
    int rodadas = 0;

    while (!memoria.estado.todos_queimados()) {
        rodadas++;

        bool houve_mudanca = propagar_fogo(grafo, memoria.estado);

        if (memoria.estado.todos_queimados()) {
            break;
        }

        int escolhido = proximo_vertice_nao_queimado(
            memoria.ordem_cromossomo,
            proxima_posicao,
            memoria.estado.queimado);

        if (escolhido != -1) {
            queimar_vertice(grafo, escolhido, memoria.estado);
            houve_mudanca = true;

            if (sequencia_queima != nullptr) {
                sequencia_queima->push_back(escolhido);
            }
        }

        terminar_rodada(memoria.estado);

        if (!houve_mudanca &&
            memoria.estado.fila_propagacao_atual.empty()) {
            break;
        }
    }

    if (!memoria.estado.todos_queimados()) {
        int restantes = static_cast<int>(grafo.getOrder()) -
            memoria.estado.total_queimados;
        return rodadas + restantes * 100.0;
    }

    return rodadas;
}

}  // namespace

double Decodificador2Queima::decode(
    const std::vector<double>& cromossomo) const {
    return simular(grafo, cromossomo, nullptr);
}

std::vector<int> Decodificador2Queima::obter_sequencia_queima(
    const std::vector<double>& cromossomo) const {
    std::vector<int> sequencia;
    simular(grafo, cromossomo, &sequencia);
    return sequencia;
}

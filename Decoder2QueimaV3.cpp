#include "Decoder2QueimaV3.h"
#include "SimulacaoQueima.h"

namespace {

struct MemoriaAuxiliar {
    EstadoQueima estado;
    std::vector<int> grau_relativo;
    std::vector<int> vertices_nao_queimados;
    std::vector<int> posicao_na_lista;

    void reiniciar(const Graph& grafo) {
        estado.reiniciar(grafo.getOrder());
        grau_relativo.resize(grafo.getOrder());
        vertices_nao_queimados.resize(grafo.getOrder());
        posicao_na_lista.resize(grafo.getOrder());

        for (size_t vertice = 0;
             vertice < grafo.getOrder();
             vertice++) {
            grau_relativo[vertice] = static_cast<int>(
                grafo.getVertexDegree(vertice));
            vertices_nao_queimados[vertice] =
                static_cast<int>(vertice);
            posicao_na_lista[vertice] = static_cast<int>(vertice);
        }
    }

    // Remove em O(1), colocando o último elemento na posição liberada.
    void remover_dos_nao_queimados(int vertice) {
        int posicao = posicao_na_lista[vertice];
        int ultimo = vertices_nao_queimados.back();

        vertices_nao_queimados[posicao] = ultimo;
        posicao_na_lista[ultimo] = posicao;
        vertices_nao_queimados.pop_back();
        posicao_na_lista[vertice] = -1;
    }
};

// Cada thread possui sua própria memória para que as avaliações paralelas do
// BRKGA não misturem seus dados.
thread_local MemoriaAuxiliar memoria;

void queimar_e_atualizar_grau(
    const Graph& grafo,
    int vertice) {

    if (memoria.estado.queimado[vertice]) {
        return;
    }

    memoria.estado.queimado[vertice] = 1;
    memoria.estado.total_queimados++;
    memoria.grau_relativo[vertice] = 0;
    memoria.remover_dos_nao_queimados(vertice);

    for (size_t vizinho : grafo.getNeighbors(vertice)) {
        if (memoria.estado.queimado[vizinho]) {
            continue;
        }

        memoria.grau_relativo[vizinho]--;
        memoria.estado.quantidade_vizinhos_queimados[vizinho]++;

        if (memoria.estado.quantidade_vizinhos_queimados[vizinho] == 2) {
            memoria.estado.fila_propagacao_seguinte.push_back(
                static_cast<int>(vizinho));
        }
    }
}

void propagar_e_atualizar_grau(const Graph& grafo) {
    for (int vertice : memoria.estado.fila_propagacao_atual) {
        queimar_e_atualizar_grau(grafo, vertice);
    }

    memoria.estado.fila_propagacao_atual.clear();
}

int escolher_por_grau_relativo(
    const std::vector<double>& cromossomo) {

    int escolhido = -1;

    for (int candidato : memoria.vertices_nao_queimados) {

        if (escolhido == -1 ||
            memoria.grau_relativo[candidato] >
                memoria.grau_relativo[escolhido] ||
            (memoria.grau_relativo[candidato] ==
                 memoria.grau_relativo[escolhido] &&
             cromossomo[candidato] > cromossomo[escolhido])) {
            escolhido = candidato;
        }
    }

    return escolhido;
}

double simular(
    const Graph& grafo,
    const std::vector<double>& cromossomo,
    std::vector<int>* sequencia_queima) {

    validar_cromossomo(grafo, cromossomo);
    memoria.reiniciar(grafo);

    if (sequencia_queima != nullptr) {
        sequencia_queima->clear();
        sequencia_queima->reserve(grafo.getOrder());
    }

    int rodadas = 0;

    while (!memoria.estado.todos_queimados()) {
        rodadas++;

        propagar_e_atualizar_grau(grafo);

        if (memoria.estado.todos_queimados()) {
            break;
        }

        int escolhido = escolher_por_grau_relativo(cromossomo);

        if (escolhido == -1) {
            break;
        }

        queimar_e_atualizar_grau(grafo, escolhido);

        if (sequencia_queima != nullptr) {
            sequencia_queima->push_back(escolhido);
        }

        terminar_rodada(memoria.estado);
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

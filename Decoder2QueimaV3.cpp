#include "Decoder2QueimaV3.h"
#include "SimulacaoQueima.h"

namespace {

struct MemoriaAuxiliar {
    EstadoQueima estado;
    std::vector<int> ordem_cromossomo;
    std::vector<int> grau_relativo;
    std::vector<int> vertices_nao_queimados;
    std::vector<int> posicao_na_lista;
    bool reparo_iniciado = false;

    void reiniciar(
        const Graph& grafo,
        const std::vector<double>& cromossomo) {

        size_t numero_vertices = grafo.getOrder();
        estado.reiniciar(numero_vertices);

        preparar_ordem_do_cromossomo(
            ordem_cromossomo,
            cromossomo,
            numero_vertices,
            true);

        grau_relativo.resize(numero_vertices);
        vertices_nao_queimados.clear();
        vertices_nao_queimados.reserve(numero_vertices);
        posicao_na_lista.resize(numero_vertices);
        reparo_iniciado = false;
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
    MemoriaAuxiliar& dados,
    const Graph& grafo,
    int vertice) {

    if (dados.estado.queimado[vertice]) {
        return;
    }

    dados.estado.queimado[vertice] = 1;
    dados.estado.total_queimados++;
    if (dados.reparo_iniciado) {
        dados.grau_relativo[vertice] = 0;
        dados.remover_dos_nao_queimados(vertice);
    }

    for (size_t vizinho : grafo.getNeighbors(vertice)) {
        if (dados.estado.queimado[vizinho]) {
            continue;
        }

        if (dados.reparo_iniciado) {
            dados.grau_relativo[vizinho]--;
        }
        dados.estado.quantidade_vizinhos_queimados[vizinho]++;

        if (dados.estado.quantidade_vizinhos_queimados[vizinho] == 2) {
            dados.estado.fila_propagacao_seguinte.push_back(
                static_cast<int>(vizinho));
        }
    }
}

void propagar_e_atualizar_grau(
    MemoriaAuxiliar& dados,
    const Graph& grafo) {
    for (int vertice : dados.estado.fila_propagacao_atual) {
        queimar_e_atualizar_grau(dados, grafo, vertice);
    }

    dados.estado.fila_propagacao_atual.clear();
}

int escolher_reparo_por_grau_relativo(
    const Graph& grafo,
    MemoriaAuxiliar& dados,
    const std::vector<double>& cromossomo) {

    if (!dados.reparo_iniciado) {
        dados.vertices_nao_queimados.clear();
        std::fill(
            dados.posicao_na_lista.begin(),
            dados.posicao_na_lista.end(),
            -1);

        for (size_t vertice = 0; vertice < grafo.getOrder(); ++vertice) {
            if (dados.estado.queimado[vertice]) {
                dados.grau_relativo[vertice] = 0;
                continue;
            }

            dados.posicao_na_lista[vertice] = static_cast<int>(
                dados.vertices_nao_queimados.size());
            dados.vertices_nao_queimados.push_back(
                static_cast<int>(vertice));
            dados.grau_relativo[vertice] = static_cast<int>(
                grafo.getVertexDegree(vertice)) -
                dados.estado.quantidade_vizinhos_queimados[vertice];
        }

        dados.reparo_iniciado = true;
    }

    int escolhido = -1;

    for (int candidato : dados.vertices_nao_queimados) {

        if (escolhido == -1 ||
            dados.grau_relativo[candidato] >
                dados.grau_relativo[escolhido] ||
            (dados.grau_relativo[candidato] ==
                 dados.grau_relativo[escolhido] &&
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
    MemoriaAuxiliar& dados = memoria;
    dados.reiniciar(grafo, cromossomo);

    if (sequencia_queima != nullptr) {
        sequencia_queima->clear();
        sequencia_queima->reserve(grafo.getOrder());
    }

    size_t proxima_posicao = 0;
    int rodadas = 0;

    while (!dados.estado.todos_queimados()) {
        rodadas++;

        propagar_e_atualizar_grau(dados, grafo);

        if (dados.estado.todos_queimados()) {
            break;
        }

        int escolhido = proximo_vertice_nao_queimado(
            dados.ordem_cromossomo,
            proxima_posicao,
            dados.estado.queimado);

        if (escolhido == -1) {
            escolhido = escolher_reparo_por_grau_relativo(
                grafo, dados, cromossomo);
        }

        if (escolhido == -1) {
            break;
        }

        queimar_e_atualizar_grau(dados, grafo, escolhido);

        if (sequencia_queima != nullptr) {
            sequencia_queima->push_back(escolhido);
        }

        terminar_rodada(dados.estado);
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

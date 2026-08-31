#include "Decoder2QueimaV4.h"
#include "SimulacaoQueima.h"

namespace {

struct MemoriaAuxiliar {
    EstadoQueima estado;
    std::vector<int> ordem_cromossomo;
    std::vector<int> vertices_com_um_queimado;
    std::vector<int> fronteira_filtrada;
    std::vector<int> quantidade_gatilhos;
    std::vector<int> candidatos_tocados;

    void reiniciar(
        const Graph& grafo,
        const std::vector<double>& cromossomo) {

        size_t numero_vertices = grafo.getOrder();
        estado.reiniciar(numero_vertices);

        preparar_ordem_do_cromossomo(
            ordem_cromossomo,
            cromossomo,
            numero_vertices,
            false);

        vertices_com_um_queimado.clear();
        fronteira_filtrada.clear();
        candidatos_tocados.clear();

        vertices_com_um_queimado.reserve(numero_vertices);
        fronteira_filtrada.reserve(numero_vertices);
        candidatos_tocados.reserve(numero_vertices);

        if (quantidade_gatilhos.size() != numero_vertices) {
            quantidade_gatilhos.assign(numero_vertices, 0);
        }
    }
};

// Cada thread possui sua própria memória para que as avaliações paralelas do
// BRKGA não misturem seus dados.
thread_local MemoriaAuxiliar memoria;

int escolher_vertice_de_gatilho(
    const Graph& grafo,
    const std::vector<double>& cromossomo) {

    if (!memoria.estado.fila_propagacao_seguinte.empty() ||
        memoria.vertices_com_um_queimado.empty()) {
        return -1;
    }

    memoria.fronteira_filtrada.clear();
    memoria.candidatos_tocados.clear();

    for (int vertice : memoria.vertices_com_um_queimado) {
        if (memoria.estado.queimado[vertice] ||
            memoria.estado.quantidade_vizinhos_queimados[vertice] != 1) {
            continue;
        }

        memoria.fronteira_filtrada.push_back(vertice);

        for (size_t candidato : grafo.getNeighbors(vertice)) {
            if (memoria.estado.queimado[candidato]) {
                continue;
            }

            if (memoria.quantidade_gatilhos[candidato] == 0) {
                memoria.candidatos_tocados.push_back(
                    static_cast<int>(candidato));
            }

            memoria.quantidade_gatilhos[candidato]++;
        }
    }

    memoria.vertices_com_um_queimado.swap(
        memoria.fronteira_filtrada);

    int escolhido = -1;
    double melhor_pontuacao = -1.0;

    for (int candidato : memoria.candidatos_tocados) {
        double pontuacao = memoria.quantidade_gatilhos[candidato] +
            cromossomo[candidato];

        if (pontuacao > melhor_pontuacao) {
            melhor_pontuacao = pontuacao;
            escolhido = candidato;
        }

        memoria.quantidade_gatilhos[candidato] = 0;
    }

    return escolhido;
}

double simular(
    const Graph& grafo,
    const std::vector<double>& cromossomo,
    std::vector<int>* sequencia_queima) {

    validar_cromossomo(grafo, cromossomo);
    memoria.reiniciar(grafo, cromossomo);

    if (sequencia_queima != nullptr) {
        sequencia_queima->clear();
        sequencia_queima->reserve(grafo.getOrder());
    }

    size_t proxima_posicao = 0;
    int rodadas = 0;

    while (!memoria.estado.todos_queimados()) {
        rodadas++;

        propagar_fogo(
            grafo,
            memoria.estado,
            &memoria.vertices_com_um_queimado);

        if (memoria.estado.todos_queimados()) {
            break;
        }

        int escolhido = escolher_vertice_de_gatilho(
            grafo,
            cromossomo);

        if (escolhido == -1) {
            escolhido = proximo_vertice_nao_queimado(
                memoria.ordem_cromossomo,
                proxima_posicao,
                memoria.estado.queimado);
        }

        if (escolhido == -1) {
            break;
        }

        queimar_vertice(
            grafo,
            escolhido,
            memoria.estado,
            &memoria.vertices_com_um_queimado);

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

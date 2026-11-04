#include "HeuristicaInicial.h"
#include "SimulacaoQueima.h"

#include <algorithm>
#include <cstdint>
#include <stdexcept>
#include <vector>

namespace {

class EstadoDaQueima {
private:
    const Graph& grafo;
    bool manter_gatilhos;

public:
    EstadoQueima estado;
    std::vector<int> grau_relativo;
    std::vector<int> numero_de_gatilhos;

    EstadoDaQueima(const Graph& novo_grafo, bool usar_gatilhos)
        : grafo(novo_grafo),
          manter_gatilhos(usar_gatilhos),
          grau_relativo(grafo.getOrder(), 0),
          numero_de_gatilhos(grafo.getOrder(), 0) {

        estado.reiniciar(grafo.getOrder());

        for (std::size_t vertice = 0;
             vertice < grafo.getOrder();
             vertice++) {
            grau_relativo[vertice] = static_cast<int>(
                grafo.getVertexDegree(vertice));
        }
    }

    bool todos_queimados() const {
        return estado.todos_queimados();
    }

    void alterar_gatilhos_criados_por(int vertice, int alteracao) {
        if (!manter_gatilhos) {
            return;
        }

        // Um vertice com exatamente um vizinho queimado funciona como um
        // gatilho para cada um de seus vizinhos que ainda nao queimou.
        for (std::size_t vizinho : grafo.getNeighbors(vertice)) {
            if (!estado.queimado[vizinho]) {
                numero_de_gatilhos[vizinho] += alteracao;
            }
        }
    }

    bool queimar(int vertice) {
        if (estado.queimado[vertice]) {
            return false;
        }

        // Se o proprio vertice era um gatilho, ele deixa de contar quando
        // queima. Seus vizinhos ainda nao queimados sao atualizados aqui.
        if (estado.quantidade_vizinhos_queimados[vertice] == 1) {
            alterar_gatilhos_criados_por(vertice, -1);
        }

        grau_relativo[vertice] = 0;
        numero_de_gatilhos[vertice] = 0;

        for (std::size_t vizinho : grafo.getNeighbors(vertice)) {
            if (estado.queimado[vizinho]) {
                continue;
            }

            grau_relativo[vizinho]--;

            int quantidade_anterior =
                estado.quantidade_vizinhos_queimados[vizinho];

            if (quantidade_anterior == 0) {
                // O vizinho passou a ter exatamente um queimado ao lado.
                alterar_gatilhos_criados_por(
                    static_cast<int>(vizinho), 1);
            } else if (quantidade_anterior == 1) {
                // Ao chegar a dois, ele deixa de ser gatilho e entra na fila
                // para queimar por propagacao na proxima rodada.
                alterar_gatilhos_criados_por(
                    static_cast<int>(vizinho), -1);
            }
        }

        return queimar_vertice(grafo, vertice, estado);
    }

    void propagar() {
        for (int vertice : estado.fila_propagacao_atual) {
            queimar(vertice);
        }

        estado.fila_propagacao_atual.clear();
    }

    void terminar_rodada() {
        ::terminar_rodada(estado);
    }
};

int escolher_por_grau_relativo(const EstadoDaQueima& estado) {
    int escolhido = -1;

    for (std::size_t posicao = 0;
         posicao < estado.estado.queimado.size();
         posicao++) {
        int candidato = static_cast<int>(posicao);

        if (estado.estado.queimado[candidato]) {
            continue;
        }

        if (escolhido == -1 ||
            estado.grau_relativo[candidato] >
                estado.grau_relativo[escolhido]) {
            escolhido = candidato;
        }
    }

    return escolhido;
}

int escolher_por_grau_relativo_e_gatilho(const EstadoDaQueima& estado) {
    int escolhido = -1;
    int melhor_pontuacao = -1;

    for (std::size_t posicao = 0;
         posicao < estado.estado.queimado.size();
         posicao++) {
        int candidato = static_cast<int>(posicao);

        if (estado.estado.queimado[candidato]) {
            continue;
        }

        int pontuacao = std::max(
            estado.grau_relativo[candidato],
            estado.numero_de_gatilhos[candidato]);

        bool pontuacao_maior = pontuacao > melhor_pontuacao;
        bool mesma_pontuacao = pontuacao == melhor_pontuacao;
        bool mais_gatilhos = escolhido != -1 &&
            estado.numero_de_gatilhos[candidato] >
                estado.numero_de_gatilhos[escolhido];
        bool mesmos_gatilhos = escolhido != -1 &&
            estado.numero_de_gatilhos[candidato] ==
                estado.numero_de_gatilhos[escolhido];
        bool maior_grau = escolhido != -1 &&
            estado.grau_relativo[candidato] >
                estado.grau_relativo[escolhido];

        if (escolhido == -1 || pontuacao_maior ||
            (mesma_pontuacao && mais_gatilhos) ||
            (mesma_pontuacao && mesmos_gatilhos && maior_grau)) {
            escolhido = candidato;
            melhor_pontuacao = pontuacao;
        }
    }

    return escolhido;
}

SolucaoInicial construir(const Graph& grafo, bool combinar_com_gatilho) {
    EstadoDaQueima estado(grafo, combinar_com_gatilho);
    SolucaoInicial solucao;
    solucao.fontes_de_queima.reserve(grafo.getOrder());

    while (!estado.todos_queimados()) {
        solucao.numero_de_rodadas++;
        estado.propagar();

        if (estado.todos_queimados()) {
            break;
        }

        int escolhido;

        if (combinar_com_gatilho) {
            escolhido = escolher_por_grau_relativo_e_gatilho(estado);
        } else {
            escolhido = escolher_por_grau_relativo(estado);
        }

        if (escolhido == -1) {
            throw std::runtime_error("Nao foi possivel completar a solucao");
        }

        estado.queimar(escolhido);
        solucao.fontes_de_queima.push_back(escolhido);
        estado.terminar_rodada();
    }

    return solucao;
}

}  // namespace

SolucaoInicial construir_por_grau_relativo(const Graph& grafo) {
    return construir(grafo, false);
}

SolucaoInicial construir_por_grau_relativo_e_gatilho(const Graph& grafo) {
    return construir(grafo, true);
}

std::vector<double> criar_cromossomo_da_solucao(
    std::size_t numero_vertices,
    const std::vector<int>& fontes_de_queima) {

    std::vector<double> cromossomo(numero_vertices, 0.25);
    std::vector<std::uint8_t> ja_inserido(numero_vertices, 0);
    double divisor = 2.0 * (fontes_de_queima.size() + 1.0);

    for (std::size_t posicao = 0;
         posicao < fontes_de_queima.size();
         posicao++) {
        int vertice = fontes_de_queima[posicao];

        if (vertice < 0 ||
            vertice >= static_cast<int>(numero_vertices) ||
            ja_inserido[vertice]) {
            throw std::invalid_argument(
                "A sequencia possui uma fonte invalida");
        }

        ja_inserido[vertice] = 1;
        cromossomo[vertice] =
            1.0 - static_cast<double>(posicao + 1) / divisor;
    }

    return cromossomo;
}

int avaliar_solucao(
    const Graph& grafo,
    const std::vector<int>& fontes_de_queima) {

    EstadoDaQueima estado(grafo, false);
    std::size_t proxima_fonte = 0;
    int numero_de_rodadas = 0;

    while (!estado.todos_queimados()) {
        numero_de_rodadas++;
        estado.propagar();

        if (estado.todos_queimados()) {
            break;
        }

        if (proxima_fonte == fontes_de_queima.size()) {
            throw std::invalid_argument("A sequencia nao queima todo o grafo");
        }

        int vertice = fontes_de_queima[proxima_fonte];
        proxima_fonte++;

        if (vertice < 0 ||
            vertice >= static_cast<int>(grafo.getOrder()) ||
            estado.estado.queimado[vertice]) {
            throw std::invalid_argument("A sequencia possui uma fonte invalida");
        }

        estado.queimar(vertice);
        estado.terminar_rodada();
    }

    if (proxima_fonte != fontes_de_queima.size()) {
        throw std::invalid_argument("A sequencia possui fontes desnecessarias");
    }

    return numero_de_rodadas;
}

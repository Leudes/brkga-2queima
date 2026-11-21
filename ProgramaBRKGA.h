#ifndef PROGRAMA_BRKGA_H
#define PROGRAMA_BRKGA_H

#include "Graph.h"
#include "brkgaAPI/BRKGA.h"
#include "brkgaAPI/MTRand.h"

#include <chrono>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

struct ParametrosBRKGA {
    std::string caminho_do_grafo;
    std::string arquivo_de_saida = "results.csv";
    std::string arquivo_de_convergencia;

    unsigned tamanho_populacao = 100;
    double fracao_elite = 0.20;
    double fracao_mutantes = 0.10;
    double probabilidade_elite = 0.70;
    unsigned populacoes = 2;
    unsigned threads = 2;
    unsigned intervalo_troca = 100;
    unsigned elites_trocados = 2;
    unsigned limite_geracoes = 1000;
    unsigned limite_sem_melhoria = 400;
    double tempo_limite_segundos = 300.0;
    unsigned tentativas = 1;
    unsigned long semente = 0;
    bool modo_irace = false;
};

struct PontoConvergenciaBRKGA {
    double tempo_segundos = 0.0;
    unsigned geracao = 0;
    int melhor_valor = 0;
    std::string evento;
};

struct ResultadoBRKGA {
    std::string algoritmo;
    std::string nome_do_grafo;
    std::size_t numero_de_vertices = 0;
    std::size_t numero_de_arestas = 0;
    float densidade = 0.0f;
    unsigned tentativa = 0;
    unsigned long semente = 0;
    int melhor_valor = 0;
    double tempo_segundos = 0.0;
    double tempo_ate_melhor_segundos = 0.0;
    double tempo_limite_segundos = 0.0;
    unsigned geracoes = 0;
    unsigned melhorias = 0;
    unsigned geracoes_sem_melhoria = 0;
    std::string motivo_da_parada;
    std::vector<int> melhor_sequencia;
    std::vector<PontoConvergenciaBRKGA> convergencia;
};

namespace programa_brkga {

using Relogio = std::chrono::steady_clock;

inline double segundos_desde(
    const Relogio::time_point& inicio,
    const Relogio::time_point& instante) {

    return std::chrono::duration<double>(instante - inicio).count();
}

inline void mostrar_ajuda(const char* programa) {
    std::cerr
        << "Uso: " << programa << " <grafo> [opcoes]\n"
        << "Opcoes:\n"
        << "  --p VALOR\n"
        << "  --pe VALOR\n"
        << "  --pm VALOR\n"
        << "  --rhoe VALOR\n"
        << "  --K VALOR\n"
        << "  --MAXT VALOR\n"
        << "  --X_INTVL VALOR\n"
        << "  --X_NUMBER VALOR\n"
        << "  --MAX_GENS VALOR\n"
        << "  --MAX_STAGT VALOR\n"
        << "  --tempo SEGUNDOS\n"
        << "  --trials VALOR\n"
        << "  --seed VALOR\n"
        << "  --output ARQUIVO\n"
        << "  --convergence ARQUIVO\n"
        << "  --irace\n";
}

inline ParametrosBRKGA ler_argumentos(
    int argc,
    char* argv[],
    ParametrosBRKGA parametros) {

    if (argc < 2) {
        mostrar_ajuda(argv[0]);
        throw std::invalid_argument("informe o arquivo do grafo");
    }
    parametros.caminho_do_grafo = argv[1];

    for (int posicao = 2; posicao < argc; posicao++) {
        std::string argumento = argv[posicao];
        auto proximo_valor = [&]() -> std::string {
            if (posicao + 1 >= argc) {
                throw std::invalid_argument(
                    "falta o valor de " + argumento);
            }
            posicao++;
            return argv[posicao];
        };

        if (argumento == "--p") {
            parametros.tamanho_populacao = std::stoul(proximo_valor());
        } else if (argumento == "--pe") {
            parametros.fracao_elite = std::stod(proximo_valor());
        } else if (argumento == "--pm") {
            parametros.fracao_mutantes = std::stod(proximo_valor());
        } else if (argumento == "--rhoe") {
            parametros.probabilidade_elite = std::stod(proximo_valor());
        } else if (argumento == "--K") {
            parametros.populacoes = std::stoul(proximo_valor());
        } else if (argumento == "--MAXT") {
            parametros.threads = std::stoul(proximo_valor());
        } else if (argumento == "--X_INTVL") {
            parametros.intervalo_troca = std::stoul(proximo_valor());
        } else if (argumento == "--X_NUMBER") {
            parametros.elites_trocados = std::stoul(proximo_valor());
        } else if (argumento == "--MAX_GENS") {
            parametros.limite_geracoes = std::stoul(proximo_valor());
        } else if (argumento == "--MAX_STAGT") {
            parametros.limite_sem_melhoria = std::stoul(proximo_valor());
        } else if (argumento == "--tempo") {
            parametros.tempo_limite_segundos = std::stod(proximo_valor());
        } else if (argumento == "--trials") {
            parametros.tentativas = std::stoul(proximo_valor());
        } else if (argumento == "--seed") {
            parametros.semente = std::stoul(proximo_valor());
        } else if (argumento == "--output") {
            parametros.arquivo_de_saida = proximo_valor();
        } else if (argumento == "--convergence" ||
                   argumento == "--convergencia") {
            parametros.arquivo_de_convergencia = proximo_valor();
        } else if (argumento == "--irace") {
            parametros.modo_irace = true;
        } else if (argumento == "--help" || argumento == "-h") {
            mostrar_ajuda(argv[0]);
            std::exit(0);
        } else {
            throw std::invalid_argument(
                "argumento desconhecido: " + argumento);
        }
    }

    if (parametros.tamanho_populacao == 0 ||
        parametros.populacoes == 0 ||
        parametros.threads == 0 ||
        parametros.intervalo_troca == 0 ||
        parametros.limite_geracoes == 0 ||
        parametros.limite_sem_melhoria == 0 ||
        parametros.tempo_limite_segundos <= 0.0 ||
        parametros.tentativas == 0) {
        throw std::invalid_argument(
            "os limites e tamanhos devem ser maiores que zero");
    }
    if (parametros.modo_irace && parametros.tentativas != 1) {
        throw std::invalid_argument(
            "o modo --irace exige --trials 1");
    }
    return parametros;
}

inline void garantir_cabecalho(
    const std::string& nome_do_arquivo,
    const std::string& cabecalho) {

    if (std::filesystem::exists(nome_do_arquivo)) {
        std::ifstream leitura(nome_do_arquivo);
        std::string cabecalho_existente;
        std::getline(leitura, cabecalho_existente);
        if (cabecalho_existente.empty()) {
            std::ofstream arquivo(nome_do_arquivo);
            arquivo << cabecalho << '\n';
            return;
        }
        if (cabecalho_existente != cabecalho) {
            throw std::runtime_error(
                "o arquivo " + nome_do_arquivo +
                " usa um formato antigo; escolha outro nome ou mova o arquivo");
        }
        return;
    }

    std::ofstream arquivo(nome_do_arquivo);
    if (!arquivo) {
        throw std::runtime_error(
            "nao foi possivel criar " + nome_do_arquivo);
    }
    arquivo << cabecalho << '\n';
}

inline const std::string& cabecalho_resultados() {
    static const std::string cabecalho =
        "graph_name,graph_order,graph_size,density,fitness_value,"
        "elapsed_time(microsecond),algorithm,trial,seed,"
        "elapsed_time_seconds,time_to_best_seconds,time_limit_seconds,"
        "budget_fraction_to_best,generations,improvements,"
        "stagnant_generations,stop_reason,sequence";
    return cabecalho;
}

inline const std::string& cabecalho_convergencia() {
    static const std::string cabecalho =
        "algorithm,graph_name,trial,seed,time_seconds,progress_type,"
        "progress,evaluations,best_value,event";
    return cabecalho;
}

inline void salvar_resultado(
    const std::string& nome_do_arquivo,
    const ResultadoBRKGA& resultado) {

    std::ofstream arquivo(nome_do_arquivo, std::ios::app);
    if (!arquivo) {
        throw std::runtime_error(
            "nao foi possivel abrir " + nome_do_arquivo);
    }

    long long microssegundos = static_cast<long long>(
        resultado.tempo_segundos * 1000000.0);

    arquivo << resultado.nome_do_grafo << ','
            << resultado.numero_de_vertices << ','
            << resultado.numero_de_arestas << ','
            << resultado.densidade << ','
            << resultado.melhor_valor << ','
            << microssegundos << ','
            << resultado.algoritmo << ','
            << resultado.tentativa << ','
            << resultado.semente << ','
            << std::setprecision(12) << resultado.tempo_segundos << ','
            << resultado.tempo_ate_melhor_segundos << ','
            << resultado.tempo_limite_segundos << ','
            << resultado.tempo_ate_melhor_segundos /
                   resultado.tempo_limite_segundos << ','
            << resultado.geracoes << ','
            << resultado.melhorias << ','
            << resultado.geracoes_sem_melhoria << ','
            << resultado.motivo_da_parada << ',';

    arquivo << '"';
    for (std::size_t posicao = 0;
         posicao < resultado.melhor_sequencia.size();
         posicao++) {
        if (posicao > 0) {
            arquivo << ' ';
        }
        arquivo << resultado.melhor_sequencia[posicao];
    }
    arquivo << "\"\n";
}

inline void salvar_convergencia(
    const std::string& nome_do_arquivo,
    const ResultadoBRKGA& resultado) {

    if (nome_do_arquivo.empty()) {
        return;
    }
    std::ofstream arquivo(nome_do_arquivo, std::ios::app);
    if (!arquivo) {
        throw std::runtime_error(
            "nao foi possivel abrir " + nome_do_arquivo);
    }

    arquivo << std::setprecision(12);
    for (const PontoConvergenciaBRKGA& ponto : resultado.convergencia) {
        arquivo << resultado.algoritmo << ','
                << resultado.nome_do_grafo << ','
                << resultado.tentativa << ','
                << resultado.semente << ','
                << ponto.tempo_segundos << ','
                << "generation," << ponto.geracao << ",,"
                << ponto.melhor_valor << ','
                << ponto.evento << '\n';
    }
}

template <class Decodificador>
ResultadoBRKGA executar_tentativa(
    const Graph& grafo,
    const ParametrosBRKGA& parametros,
    const std::string& nome_do_algoritmo,
    unsigned tentativa) {

    ResultadoBRKGA resultado;
    resultado.algoritmo = nome_do_algoritmo;
    resultado.nome_do_grafo = std::filesystem::path(
        parametros.caminho_do_grafo).filename().string();
    resultado.numero_de_vertices = grafo.getOrder();
    resultado.numero_de_arestas = grafo.getSize();
    resultado.densidade = grafo.getDensity();
    resultado.tentativa = tentativa + 1;
    resultado.semente = parametros.semente + tentativa;
    resultado.tempo_limite_segundos = parametros.tempo_limite_segundos;

    auto inicio = Relogio::now();
    auto limite_de_tempo = inicio +
        std::chrono::duration_cast<Relogio::duration>(
            std::chrono::duration<double>(
                parametros.tempo_limite_segundos));

    Decodificador decodificador(grafo);
    MTRand gerador(resultado.semente);
    BRKGA<Decodificador, MTRand> algoritmo(
        grafo.getOrder(),
        parametros.tamanho_populacao,
        parametros.fracao_elite,
        parametros.fracao_mutantes,
        parametros.probabilidade_elite,
        decodificador,
        gerador,
        parametros.populacoes,
        parametros.threads);

    int melhor_valor = static_cast<int>(algoritmo.getBestFitness());
    auto agora = Relogio::now();
    resultado.tempo_ate_melhor_segundos = segundos_desde(inicio, agora);
    resultado.convergencia.push_back({
        resultado.tempo_ate_melhor_segundos,
        0,
        melhor_valor,
        "initial_population"});

    while (resultado.geracoes < parametros.limite_geracoes &&
           resultado.geracoes_sem_melhoria <
               parametros.limite_sem_melhoria &&
           Relogio::now() < limite_de_tempo) {

        algoritmo.evolve();
        resultado.geracoes++;

        int valor_atual = static_cast<int>(algoritmo.getBestFitness());
        if (valor_atual < melhor_valor) {
            melhor_valor = valor_atual;
            resultado.melhorias++;
            resultado.geracoes_sem_melhoria = 0;
            agora = Relogio::now();
            resultado.tempo_ate_melhor_segundos =
                segundos_desde(inicio, agora);
            resultado.convergencia.push_back({
                resultado.tempo_ate_melhor_segundos,
                resultado.geracoes,
                melhor_valor,
                "improvement"});
        } else if (resultado.geracoes > 1) {
            // A implementacao anterior comecava a contar a estagnacao
            // somente depois da primeira geracao. Mantemos essa semantica
            // para nao alterar os experimentos ja calibrados.
            resultado.geracoes_sem_melhoria++;
        }

        if (resultado.geracoes % parametros.intervalo_troca == 0) {
            algoritmo.exchangeElite(parametros.elites_trocados);
        }
    }

    auto fim = Relogio::now();
    resultado.tempo_segundos = segundos_desde(inicio, fim);
    resultado.melhor_valor = static_cast<int>(algoritmo.getBestFitness());

    if (fim >= limite_de_tempo) {
        resultado.motivo_da_parada = "time_limit";
    } else if (resultado.geracoes_sem_melhoria >=
               parametros.limite_sem_melhoria) {
        resultado.motivo_da_parada = "stagnation";
    } else {
        resultado.motivo_da_parada = "generation_limit";
    }

    resultado.convergencia.push_back({
        resultado.tempo_segundos,
        resultado.geracoes,
        resultado.melhor_valor,
        "end"});

    std::vector<double> melhor_cromossomo =
        algoritmo.getBestChromosome();
    resultado.melhor_sequencia =
        decodificador.obter_sequencia_queima(melhor_cromossomo);
    return resultado;
}

}  // namespace programa_brkga

template <class Decodificador>
int executar_programa_brkga(
    int argc,
    char* argv[],
    const std::string& nome_do_algoritmo,
    ParametrosBRKGA parametros_padrao) {

    try {
        ParametrosBRKGA parametros = programa_brkga::ler_argumentos(
            argc, argv, parametros_padrao);

        if (!parametros.modo_irace) {
            programa_brkga::garantir_cabecalho(
                parametros.arquivo_de_saida,
                programa_brkga::cabecalho_resultados());
            if (!parametros.arquivo_de_convergencia.empty()) {
                programa_brkga::garantir_cabecalho(
                    parametros.arquivo_de_convergencia,
                    programa_brkga::cabecalho_convergencia());
            }
        }

        Graph grafo(parametros.caminho_do_grafo);
        for (unsigned tentativa = 0;
             tentativa < parametros.tentativas;
             tentativa++) {

            ResultadoBRKGA resultado =
                programa_brkga::executar_tentativa<Decodificador>(
                    grafo,
                    parametros,
                    nome_do_algoritmo,
                    tentativa);

            if (parametros.modo_irace) {
                std::cout << resultado.melhor_valor << '\n';
            } else {
                programa_brkga::salvar_resultado(
                    parametros.arquivo_de_saida, resultado);
                programa_brkga::salvar_convergencia(
                    parametros.arquivo_de_convergencia, resultado);
            }
        }
    } catch (const std::exception& erro) {
        std::cerr << "Erro: " << erro.what() << '\n';
        return 1;
    }
    return 0;
}

#endif

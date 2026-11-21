#include "Decoder2Queima.h"
#include "ProgramaBRKGA.h"

int main(int argc, char* argv[]) {
    ParametrosBRKGA parametros;
    parametros.arquivo_de_saida = "results.csv";
    parametros.tamanho_populacao = 100;
    parametros.fracao_elite = 0.20;
    parametros.fracao_mutantes = 0.10;
    parametros.probabilidade_elite = 0.70;
    parametros.populacoes = 2;
    parametros.threads = 2;
    parametros.intervalo_troca = 100;
    parametros.elites_trocados = 2;
    parametros.limite_geracoes = 1000;
    parametros.limite_sem_melhoria = 400;

    return executar_programa_brkga<Decodificador2Queima>(
        argc, argv, "brkga_v1", parametros);
}

#include "Decoder2QueimaV4.h"
#include "ProgramaBRKGA.h"

int main(int argc, char* argv[]) {
    ParametrosBRKGA parametros;
    parametros.arquivo_de_saida = "results4.csv";
    parametros.tamanho_populacao = 383;
    parametros.fracao_elite = 0.2401;
    parametros.fracao_mutantes = 0.2271;
    parametros.probabilidade_elite = 0.8214;
    parametros.populacoes = 8;
    parametros.threads = 2;
    parametros.intervalo_troca = 73;
    parametros.elites_trocados = 2;
    parametros.limite_geracoes = 527;
    parametros.limite_sem_melhoria = 479;

    return executar_programa_brkga<Decodificador2Queima>(
        argc, argv, "brkga_v4", parametros);
}

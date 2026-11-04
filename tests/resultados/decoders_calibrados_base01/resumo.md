# Decoders calibrados na base inicial

Compara as seis versões do BRKGA, usando seus parâmetros calibrados, em 10 grafos da base inicial com 30 repetições por versão e instância.

| Grafo | Vértices | Arestas | Versão | Execuções | Válidas | Fitness médio | Melhor fitness | Desvio | Tempo médio (s) |
|---|---:|---:|---:|---:|---:|---:|---:|---:|---:|
| ciclo_n100.txt | 100 | 100 | V1_cromossomo | 30 | 30 | 52.633 | 52 | 0.490 | 1.542176 |
| ciclo_n100.txt | 100 | 100 | V2_grau_estatico | 30 | 30 | 52.800 | 52 | 0.484 | 1.235476 |
| ciclo_n100.txt | 100 | 100 | V3_reparo_grau_relativo | 30 | 30 | 51.667 | 51 | 0.479 | 0.443612 |
| ciclo_n100.txt | 100 | 100 | V4_reparo_gatilho | 30 | 30 | 51.033 | 51 | 0.183 | 1.243337 |
| ciclo_n100.txt | 100 | 100 | V5_reparo_combinado | 30 | 30 | 51.033 | 51 | 0.183 | 2.644694 |
| ciclo_n100.txt | 100 | 100 | V6_permutacao_pura | 30 | 30 | 52.567 | 51 | 0.568 | 1.522852 |
| cubic_100.txt | 100 | 150 | V1_cromossomo | 30 | 30 | 28.700 | 28 | 0.466 | 1.544530 |
| cubic_100.txt | 100 | 150 | V2_grau_estatico | 30 | 30 | 28.700 | 28 | 0.466 | 1.557375 |
| cubic_100.txt | 100 | 150 | V3_reparo_grau_relativo | 30 | 30 | 30.200 | 28 | 0.847 | 0.417166 |
| cubic_100.txt | 100 | 150 | V4_reparo_gatilho | 30 | 30 | 28.600 | 28 | 0.498 | 1.223688 |
| cubic_100.txt | 100 | 150 | V5_reparo_combinado | 30 | 30 | 28.033 | 28 | 0.183 | 5.110522 |
| cubic_100.txt | 100 | 150 | V6_permutacao_pura | 30 | 30 | 28.700 | 28 | 0.466 | 1.708105 |
| cycle1-order25.txt | 25 | 25 | V1_cromossomo | 30 | 30 | 14.000 | 14 | 0.000 | 0.331939 |
| cycle1-order25.txt | 25 | 25 | V2_grau_estatico | 30 | 30 | 14.000 | 14 | 0.000 | 0.260588 |
| cycle1-order25.txt | 25 | 25 | V3_reparo_grau_relativo | 30 | 30 | 14.000 | 14 | 0.000 | 0.066476 |
| cycle1-order25.txt | 25 | 25 | V4_reparo_gatilho | 30 | 30 | 14.000 | 14 | 0.000 | 0.265533 |
| cycle1-order25.txt | 25 | 25 | V5_reparo_combinado | 30 | 30 | 14.000 | 14 | 0.000 | 0.477050 |
| cycle1-order25.txt | 25 | 25 | V6_permutacao_pura | 30 | 30 | 14.000 | 14 | 0.000 | 0.279889 |
| cycle10-order250.txt | 250 | 250 | V1_cromossomo | 30 | 30 | 132.067 | 129 | 1.413 | 4.411235 |
| cycle10-order250.txt | 250 | 250 | V2_grau_estatico | 30 | 30 | 132.367 | 130 | 1.066 | 3.577919 |
| cycle10-order250.txt | 250 | 250 | V3_reparo_grau_relativo | 30 | 30 | 128.800 | 127 | 0.805 | 1.853939 |
| cycle10-order250.txt | 250 | 250 | V4_reparo_gatilho | 30 | 30 | 126.933 | 126 | 0.450 | 3.937554 |
| cycle10-order250.txt | 250 | 250 | V5_reparo_combinado | 30 | 30 | 126.800 | 126 | 0.484 | 7.716845 |
| cycle10-order250.txt | 250 | 250 | V6_permutacao_pura | 30 | 30 | 132.033 | 130 | 1.159 | 4.594440 |
| dwt__162.txt | 162 | 510 | V1_cromossomo | 30 | 30 | 10.833 | 10 | 0.379 | 2.264460 |
| dwt__162.txt | 162 | 510 | V2_grau_estatico | 30 | 30 | 10.967 | 10 | 0.183 | 2.157091 |
| dwt__162.txt | 162 | 510 | V3_reparo_grau_relativo | 30 | 30 | 10.967 | 10 | 0.183 | 0.568125 |
| dwt__162.txt | 162 | 510 | V4_reparo_gatilho | 30 | 30 | 10.933 | 10 | 0.254 | 1.566124 |
| dwt__162.txt | 162 | 510 | V5_reparo_combinado | 30 | 30 | 10.867 | 10 | 0.346 | 2.515091 |
| dwt__162.txt | 162 | 510 | V6_permutacao_pura | 30 | 30 | 11.000 | 11 | 0.000 | 2.270527 |
| grade_10x10.txt | 100 | 180 | V1_cromossomo | 30 | 30 | 14.967 | 14 | 0.183 | 1.415925 |
| grade_10x10.txt | 100 | 180 | V2_grau_estatico | 30 | 30 | 15.000 | 15 | 0.000 | 1.301370 |
| grade_10x10.txt | 100 | 180 | V3_reparo_grau_relativo | 30 | 30 | 14.967 | 14 | 0.183 | 0.480791 |
| grade_10x10.txt | 100 | 180 | V4_reparo_gatilho | 30 | 30 | 14.967 | 14 | 0.183 | 0.939460 |
| grade_10x10.txt | 100 | 180 | V5_reparo_combinado | 30 | 30 | 15.000 | 15 | 0.000 | 1.679142 |
| grade_10x10.txt | 100 | 180 | V6_permutacao_pura | 30 | 30 | 14.967 | 14 | 0.183 | 1.412734 |
| nos7.txt | 729 | 1944 | V1_cromossomo | 30 | 30 | 22.033 | 21 | 0.320 | 11.882110 |
| nos7.txt | 729 | 1944 | V2_grau_estatico | 30 | 30 | 22.300 | 22 | 0.466 | 11.131283 |
| nos7.txt | 729 | 1944 | V3_reparo_grau_relativo | 30 | 30 | 22.267 | 22 | 0.450 | 4.660701 |
| nos7.txt | 729 | 1944 | V4_reparo_gatilho | 30 | 30 | 22.200 | 22 | 0.407 | 9.068244 |
| nos7.txt | 729 | 1944 | V5_reparo_combinado | 30 | 30 | 21.967 | 21 | 0.320 | 13.371990 |
| nos7.txt | 729 | 1944 | V6_permutacao_pura | 30 | 30 | 22.200 | 22 | 0.407 | 12.505057 |
| random_denso_n50_p30.txt | 50 | 384 | V1_cromossomo | 30 | 30 | 4.000 | 4 | 0.000 | 0.895104 |
| random_denso_n50_p30.txt | 50 | 384 | V2_grau_estatico | 30 | 30 | 4.000 | 4 | 0.000 | 0.766701 |
| random_denso_n50_p30.txt | 50 | 384 | V3_reparo_grau_relativo | 30 | 30 | 4.000 | 4 | 0.000 | 0.193279 |
| random_denso_n50_p30.txt | 50 | 384 | V4_reparo_gatilho | 30 | 30 | 4.000 | 4 | 0.000 | 0.591839 |
| random_denso_n50_p30.txt | 50 | 384 | V5_reparo_combinado | 30 | 30 | 4.000 | 4 | 0.000 | 0.843572 |
| random_denso_n50_p30.txt | 50 | 384 | V6_permutacao_pura | 30 | 30 | 4.000 | 4 | 0.000 | 0.758778 |
| random_esparso_n200_d3.txt | 200 | 481 | V1_cromossomo | 30 | 30 | 12.700 | 12 | 0.466 | 2.976750 |
| random_esparso_n200_d3.txt | 200 | 481 | V2_grau_estatico | 30 | 30 | 12.867 | 12 | 0.346 | 2.666720 |
| random_esparso_n200_d3.txt | 200 | 481 | V3_reparo_grau_relativo | 30 | 30 | 12.867 | 12 | 0.346 | 0.758934 |
| random_esparso_n200_d3.txt | 200 | 481 | V4_reparo_gatilho | 30 | 30 | 12.800 | 12 | 0.407 | 2.066400 |
| random_esparso_n200_d3.txt | 200 | 481 | V5_reparo_combinado | 30 | 30 | 12.733 | 12 | 0.450 | 3.381907 |
| random_esparso_n200_d3.txt | 200 | 481 | V6_permutacao_pura | 30 | 30 | 12.767 | 12 | 0.430 | 3.399356 |
| steam3.txt | 80 | 424 | V1_cromossomo | 30 | 30 | 7.000 | 7 | 0.000 | 1.011870 |
| steam3.txt | 80 | 424 | V2_grau_estatico | 30 | 30 | 7.000 | 7 | 0.000 | 0.896012 |
| steam3.txt | 80 | 424 | V3_reparo_grau_relativo | 30 | 30 | 7.000 | 7 | 0.000 | 0.247904 |
| steam3.txt | 80 | 424 | V4_reparo_gatilho | 30 | 30 | 7.000 | 7 | 0.000 | 0.721144 |
| steam3.txt | 80 | 424 | V5_reparo_combinado | 30 | 30 | 7.000 | 7 | 0.000 | 1.055743 |
| steam3.txt | 80 | 424 | V6_permutacao_pura | 30 | 30 | 7.000 | 7 | 0.000 | 0.897267 |

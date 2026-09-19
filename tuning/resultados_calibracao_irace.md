# Resultados da calibração do BRKGA com o irace

Este documento reúne o tempo de execução e a melhor configuração encontrada
para cada uma das cinco versões do BRKGA. As informações foram extraídas dos
arquivos `irace-v1.Rdata` a `irace-v5.Rdata` com a função
`getFinalElites(iraceResults, n = 1)` do pacote irace.

## Configuração comum das calibrações

- Orçamento: 1.000 avaliações por versão.
- Semente do cenário: 20260913.
- Execuções simultâneas do irace: 1 (`parallel = 1`).
- Número de populações do BRKGA: `K = 2`.
- Número de threads do BRKGA: `MAXT = 4`.
- Número de indivíduos trocados entre populações: `X_NUMBER = 2`.
- Número de tentativas por avaliação: 1.

Os parâmetros `K`, `MAXT` e `X_NUMBER` foram mantidos fixos e, portanto, não
foram calibrados pelo irace.

## Resumo

| Versão | Tempo | Avaliações | Iterações | ID da melhor configuração | `p` | `pe` | `pm` | `rhoe` | `MAX_GENS` | `MAX_STAGT` | `X_INTVL` |
|---|---:|---:|---:|---:|---:|---:|---:|---:|---:|---:|---:|
| V1 | 05h58min33s | 999/1000 | 6 | 118 | 578 | 0,2788 | 0,0743 | 0,6237 | 583 | 477 | 125 |
| V2 | 04h51min52s | 1000/1000 | 5 | 42 | 518 | 0,2463 | 0,2171 | 0,6522 | 651 | 451 | 91 |
| V3 | 18h53min35s | 999/1000 | 4 | 88 | 572 | 0,2485 | 0,1638 | 0,6334 | 344 | 102 | 86 |
| V4 | 38h23min34s | 999/1000 | 4 | 114 | 407 | 0,2899 | 0,0692 | 0,6514 | 762 | 411 | 137 |
| V5 | 42h35min09s | 1000/1000 | 5 | 106 | 554 | 0,2689 | 0,2702 | 0,6252 | 652 | 486 | 37 |

O tempo acumulado das cinco calibrações foi de aproximadamente
**110h42min45s**, equivalente a **4 dias, 14 horas, 42 minutos e 45 segundos**.

## V1

- Arquivo: `irace-v1.Rdata`.
- Conclusão: 14/09/2026 às 03:26:42.
- Tempo: 21.513,568 segundos (05h58min33s).
- Avaliações: 999 de 1.000.
- Iterações: 6.
- Melhor configuração: ID 118.

```text
p          = 578
pe         = 0.2788
pm         = 0.0743
rhoe       = 0.6237
MAX_GENS   = 583
MAX_STAGT  = 477
X_INTVL    = 125
```

## V2

- Arquivo: `irace-v2.Rdata`.
- Conclusão: 14/09/2026 às 08:18:34.
- Tempo: 17.512,273 segundos (04h51min52s).
- Avaliações: 1.000 de 1.000.
- Iterações: 5.
- Melhor configuração: ID 42.

```text
p          = 518
pe         = 0.2463
pm         = 0.2171
rhoe       = 0.6522
MAX_GENS   = 651
MAX_STAGT  = 451
X_INTVL    = 91
```

## V3

- Arquivo: `irace-v3.Rdata`.
- Conclusão: 15/09/2026 às 03:12:11.
- Tempo: 68.015,720 segundos (18h53min35s).
- Avaliações: 999 de 1.000.
- Iterações: 4.
- Melhor configuração: ID 88.

```text
p          = 572
pe         = 0.2485
pm         = 0.1638
rhoe       = 0.6334
MAX_GENS   = 344
MAX_STAGT  = 102
X_INTVL    = 86
```

## V4

- Arquivo: `irace-v4.Rdata`.
- Conclusão: 16/09/2026 às 17:35:46.
- Tempo: 138.214,396 segundos (38h23min34s).
- Avaliações: 999 de 1.000.
- Iterações: 4.
- Melhor configuração: ID 114.

```text
p          = 407
pe         = 0.2899
pm         = 0.0692
rhoe       = 0.6514
MAX_GENS   = 762
MAX_STAGT  = 411
X_INTVL    = 137
```

## V5

- Arquivo: `irace-v5.Rdata`.
- Conclusão: 18/09/2026 às 17:53:15.
- Tempo: 153.309,020 segundos (42h35min09s).
- Avaliações: 1.000 de 1.000.
- Iterações: 5.
- Melhor configuração: ID 106.

```text
p          = 554
pe         = 0.2689
pm         = 0.2702
rhoe       = 0.6252
MAX_GENS   = 652
MAX_STAGT  = 486
X_INTVL    = 37
```

## Observação sobre 999 avaliações

As versões V1, V3 e V4 terminaram com 999 avaliações porque o orçamento
restante não era suficiente para o irace iniciar outra corrida com a quantidade
mínima de configurações sobreviventes. O estado registrado nos três arquivos é
`Not enough budget to race more than the minimum configurations`, que representa
uma conclusão normal da calibração, e não uma interrupção ou erro.

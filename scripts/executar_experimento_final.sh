#!/usr/bin/env bash
set -euo pipefail

if [[ $# -lt 3 ]]; then
    echo "Uso: $0 <v1|v4> <pasta_dos_grafos> <pasta_de_saida>" >&2
    exit 1
fi

VERSAO="${1,,}"
PASTA_GRAFOS="$(realpath "$2")"
PASTA_SAIDA="$(realpath -m "$3")"

REPETICOES="${REPETICOES:-15}"
TEMPO="${TEMPO:-300}"
SEMENTE_INICIAL="${SEMENTE_INICIAL:-20261120}"
LIMITE_INSTANCIAS="${LIMITE_INSTANCIAS:-0}"

PASTA_REPOSITORIO="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"

case "$VERSAO" in
    v1)
        EXECUTAVEL="$PASTA_REPOSITORIO/2queima"
        PARAMETROS=(
            --p 578 --pe 0.2788 --pm 0.0743 --rhoe 0.6237
            --K 2 --MAXT 4 --X_INTVL 125 --X_NUMBER 2
            --MAX_GENS 583 --MAX_STAGT 477
        )
        ;;
    v4)
        EXECUTAVEL="$PASTA_REPOSITORIO/2queimaV4"
        PARAMETROS=(
            --p 407 --pe 0.2899 --pm 0.0692 --rhoe 0.6514
            --K 2 --MAXT 4 --X_INTVL 137 --X_NUMBER 2
            --MAX_GENS 762 --MAX_STAGT 411
        )
        ;;
    *)
        echo "Versao invalida: use v1 ou v4" >&2
        exit 1
        ;;
esac

[[ -d "$PASTA_GRAFOS" ]] || { echo "Pasta inexistente: $PASTA_GRAFOS" >&2; exit 1; }
[[ -x "$EXECUTAVEL" ]] || { echo "Compile primeiro: make ${EXECUTAVEL##*/}" >&2; exit 1; }
[[ "$REPETICOES" =~ ^[1-9][0-9]*$ ]] || { echo "REPETICOES invalido" >&2; exit 1; }

PASTA_RESULTADOS="$PASTA_SAIDA/por_grafo/resultados"
PASTA_CONVERGENCIA="$PASTA_SAIDA/por_grafo/convergencia"
mkdir -p "$PASTA_RESULTADOS" "$PASTA_CONVERGENCIA"

mapfile -d '' GRAFOS < <(find "$PASTA_GRAFOS" -maxdepth 1 -type f -name '*.txt' -print0 | sort -z)
[[ ${#GRAFOS[@]} -gt 0 ]] || { echo "Nenhum grafo .txt encontrado" >&2; exit 1; }

if (( LIMITE_INSTANCIAS > 0 && LIMITE_INSTANCIAS < ${#GRAFOS[@]} )); then
    GRAFOS=("${GRAFOS[@]:0:LIMITE_INSTANCIAS}")
fi

resultado_completo() {
    local arquivo_resultado="$1"
    local arquivo_convergencia="$2"
    local linhas_resultado finais_convergencia

    [[ -f "$arquivo_resultado" && -f "$arquivo_convergencia" ]] || return 1
    linhas_resultado=$(($(wc -l < "$arquivo_resultado") - 1))
    finais_convergencia=$(awk -F, 'NR > 1 && $NF == "end" { total++ } END { print total + 0 }' "$arquivo_convergencia")
    [[ "$linhas_resultado" -eq "$REPETICOES" && "$finais_convergencia" -eq "$REPETICOES" ]]
}

guardar_incompleto() {
    local arquivo="$1"
    if [[ -f "$arquivo" ]]; then
        mv "$arquivo" "$arquivo.incompleto.$(date +%s%N)"
    fi
}

total=${#GRAFOS[@]}
for indice in "${!GRAFOS[@]}"; do
    grafo="${GRAFOS[$indice]}"
    nome="$(basename "${grafo%.txt}")"
    resultado="$PASTA_RESULTADOS/$nome.csv"
    convergencia="$PASTA_CONVERGENCIA/$nome.csv"

    if resultado_completo "$resultado" "$convergencia"; then
        echo "[$((indice + 1))/$total] $nome: ja concluido"
        continue
    fi

    guardar_incompleto "$resultado"
    guardar_incompleto "$convergencia"

    echo "[$((indice + 1))/$total] executando $nome ($VERSAO)"
    "$EXECUTAVEL" "$grafo" \
        "${PARAMETROS[@]}" \
        --tempo "$TEMPO" \
        --trials "$REPETICOES" \
        --seed "$SEMENTE_INICIAL" \
        --output "$resultado" \
        --convergence "$convergencia"
done

consolidar_csvs() {
    local pasta_origem="$1"
    local destino="$2"
    local temporario="$destino.tmp"
    local primeiro=1

    : > "$temporario"
    while IFS= read -r -d '' arquivo; do
        if (( primeiro )); then
            cat "$arquivo" >> "$temporario"
            primeiro=0
        else
            tail -n +2 "$arquivo" >> "$temporario"
        fi
    done < <(find "$pasta_origem" -maxdepth 1 -type f -name '*.csv' -print0 | sort -z)
    mv "$temporario" "$destino"
}

consolidar_csvs "$PASTA_RESULTADOS" "$PASTA_SAIDA/resultados.csv"
consolidar_csvs "$PASTA_CONVERGENCIA" "$PASTA_SAIDA/convergencia.csv"

echo "Experimento concluido: $PASTA_SAIDA"

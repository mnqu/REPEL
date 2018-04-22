#!/bin/sh

fact_file=$1
output_entity=$2
output_relation=$3
output_context=$4
samples=$5
ratio=$6
init=$7
input_entity=$8
input_relation=$9
input_context=$10
dataset=$11

./embed -alpha 0.01 -ratio ${ratio} -entity ${dataset}vocab.set -relation ${dataset}relation.set -network ${dataset}net.txt -triple ${fact_file} -output-en ${output_entity} -output-rl ${output_relation} -output-ct ${output_context} -binary 1 -size 100 -negative 5 -samples ${samples} -threads 20 -init ${init} -input-en ${input_entity} -input-rl ${input_relation} -input-ct ${input_context}


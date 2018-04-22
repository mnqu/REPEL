#!/bin/sh

pattern_fact_file=${dataset}""
relation_set_file=${dataset}""

entity_emb_file=$1
fact_file=$2
ranked_pattern_file=$3
all_fact_file=$4
thresh_d=$5
thresh_s=$6
pattern_top_k=$7
dataset=$8

./discover -seed ${fact_file} -fact ${dataset}fact.txt -link ${dataset}link.txt -entity ${entity_emb_file} -output-pattern ${ranked_pattern_file} -output-fact temp.txt -knns 20 -lambda 0 -threads 20 -top-k ${pattern_top_k} -thresh-d ${thresh_d} -thresh-s ${thresh_s}

python gen_seed.py ${dataset}fact.txt temp.txt temp2.txt

python combine.py ${fact_file} temp2.txt ${all_fact_file}


#!/bin/sh

parsing_result=$1
seed_file=$2
output_path=../dataset/

python parsed2text.py ${parsing_result} ${output_path}text.txt
python pattern.py ${parsing_result} ${output_path}pattern_fact.txt
./data2net -train ${output_path}text.txt -output ${output_path}net.txt -debug 2 -window 5 -min-count 10
python entity.py ${output_path}net.txt ${output_path}vocab.set
python relation.py ${seed_file} ${output_path}relation.set
python extract.py ${parsing_result} 1 ${output_path}pattern.txt ${output_path}fact.txt ${output_path}link.txt ${output_path}eid2name.txt

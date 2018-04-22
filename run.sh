#!/bin/sh

dataset=""
seed_file=""
workspace=""

samples=1000
ratio=1.1
thresh_d=8.5
thresh_s=0.2
top_k=50

# Iter 0
cd embed
./run.sh ${seed_file} ${workspace}ent0.emb ${workspace}rlt0.emb ${workspace}cont0.emb ${samples} ${ratio} 0 temp.emb temp.emb temp.emb ${dataset}
cd ..

cd pattern
./run.sh ${workspace}ent0.emb ${seed_file} ${workspace}pat0.txt ${workspace}fact0.txt ${thresh_d} ${thresh_s} ${top_k} ${dataset}
cd ..

# Iter 1
cd embed
./run.sh ${workspace}fact0.txt ${workspace}ent1.emb ${workspace}rlt1.emb ${workspace}cont1.emb ${samples} ${ratio} 1 ${workspace}ent0.emb ${workspace}rlt0.emb ${workspace}cont0.emb ${dataset}
cd ..

cd pattern
./run.sh ${workspace}ent1.emb ${workspace}fact0.txt ${workspace}pat1.txt ${workspace}fact1.txt ${thresh_d} ${thresh_s} ${top_k} ${dataset}
cd ..


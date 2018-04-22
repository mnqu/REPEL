# REPEL
This is an implementation of the REPEL model proposed in the WWW 2018 paper ["Weakly-supervised Relation Extraction by Pattern-enhanced Embedding Learning"](https://arxiv.org/abs/1711.03226).

Given a text corpus and some target relations specified by a set of seed entity pairs, REPEL will automatically discover some reliable textual patterns for each relation, and also more relation instances under each relation. REPEL consists of two modules, i.e., a pattern module and a distributional module. The pattern module aims at finding some highly reliable patterns for each target relation, and the distributional module tries to learn entity embeddings and a relation classifier for prediction. During training, we alternate between optimizing the pattern module and the distributional module, so that they can mutually enhance each other. Once the training process convergences, both modules can be used for relation extraction, which find new relation instances from different perspectives.

We provide the codes for data preprocessing in the "preprocess" folder, the codes of the distributional module in the "embed" folder, and the codes of the pattern module in the "pattern" folder. The datasets used in the paper will be uploaded soon.

## Install
Our codes rely on two external packages, which are the Eigen package and the GSL package.

#### Eigen
The [Eigen](http://eigen.tuxfamily.org/index.php?title=Main_Page) package is used for matrix and vector operations. To compile our codes, users need to download the package.

#### GSL
The [GSL](https://www.gnu.org/software/gsl/) package is used for random number generation. Users need to download and install the package.

## Compile
After installing the two packages, users need to modify the package paths in "embed/makefile". Then users may go to every folder and use the makefile to compile the codes.

## Data
To run REPEL, users need to provide two files. The first file is a text corpus parsed by the Stanford Core NLP Package, and linked by some entity linking tools. The second file is a set of entity pairs for each target relation.

We will upload the data used in the paper soon.

## Running
To run the REPEL model, users may first use the running script in the "preprocess" folder for data preprocessing. Then users can use the running script in the main folder to run the model.
```
./run.sh
```

## Contact: 
If you have any questions about the codes and data, please feel free to contact us.
```
Meng Qu, qumn123@gmail.com
```

## Citation
```
@inproceedings{qu2018weakly,
title={Weakly-supervised Relation Extraction by Pattern-enhanced Embedding Learning},
author={Qu, Meng and Ren, Xiang and Zhang, Yu and Han, Jiawei},
booktitle={Proceedings of the 2018 World Wide Web Conference on World Wide Web},
pages={1257--1266},
year={2018},
organization={International World Wide Web Conferences Steering Committee}
}
```

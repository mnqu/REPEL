import sys
import os

fi = open(sys.argv[1], 'r')
fo = open(sys.argv[2], 'w')
vocab = {}
for line in fi:
	u = line.split()[0]
	v = line.split()[1]
	vocab[u] = 1
	vocab[v] = 1
for ent in vocab.keys():
	fo.write(ent + '\n')
fi.close()
fo.close()


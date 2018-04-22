import sys
import os

fi = open(sys.argv[1], 'r')
fo = open(sys.argv[2], 'w')
rels = {}
for line in fi:
	rel = line.strip().split()[2]
	rels[rel] = 1
for rel in rels.keys():
	fo.write(rel + '\n')
fi.close()
fo.close()


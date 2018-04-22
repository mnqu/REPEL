import sys
import os

seed_file = sys.argv[1]
infer_file = sys.argv[2]
output_file = sys.argv[3]

dic = {}

fi = open(seed_file, 'r')
for line in fi:
	dic[line] = 1
fi.close()

fi = open(infer_file, 'r')
for line in fi:
	dic[line] = 1
fi.close()

print '#unique fact:', len(dic)

fo = open(output_file, 'w')
for line in dic.keys():
	fo.write(line)
fo.close()


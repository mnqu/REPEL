import sys
import os

fact_file = sys.argv[1]
reliable_fact_file = sys.argv[2]
output_file = sys.argv[3]

fid2fact = {}
fi = open(fact_file, 'r')
for line in fi:
	lst = line.strip().split('\t')
	fid = int(lst[0])
	u = lst[1]
	v = lst[2]
	fid2fact[fid] = (u, v)
fi.close()

print '#fact:', len(fid2fact)

fi = open(reliable_fact_file, 'r')
fo = open(output_file, 'w')
for line in fi:
	lst = line.strip().split()
	rlt = lst[0]
	fid = int(lst[1])
	u, v = fid2fact[fid]
	fo.write(u + '\t' + v + '\t' + rlt + '\n')
fi.close()
fo.close()


import sys
import os

input_file = sys.argv[1]
pattern_min_count = int(sys.argv[2])
pattern_file = sys.argv[3]
fact_file = sys.argv[4]
link_file = sys.argv[5]

print 'Reading data...'
data = []
cnt = 0
fi = open(input_file, 'r')
while True:
	line1 = fi.readline()
	line2 = fi.readline()
	line3 = fi.readline()
	line4 = fi.readline()
	line5 = fi.readline()

	if not line1:
		break

	if cnt % 1000000 == 0:
		print cnt
	cnt += 1

	eid1, eid2 = line1.strip().split('\t')[0], line1.strip().split('\t')[1]
	eid1, eid2 = '||' + eid1 + '||', '||' + eid2 + '||'
	name1, name2 = line2.strip().split('\t')[0], line2.strip().split('\t')[1]
	pat = line3.strip()

	if eid1 == eid2:
		continue

	data.append([eid1, eid2, pat])
fi.close()

print 'Extracting valid patterns...'
pat2cnt, pat2valid = {}, {}
for u, v, pat in data:
	pat2cnt[pat] = pat2cnt.get(pat, 0) + 1
for pat, cnt in pat2cnt.items():
	if cnt < pattern_min_count:
		continue
	if len(pat.split('<Ent>')) < 3:
		continue
	if len(pat.split(' ')) > 10:
		continue
	#if len(pat.split(' ')) < 3:
	#	continue

	pat2valid[pat] = 1

print 'Writing valid patterns...'
fo = open(pattern_file, 'w')
pat2pid = {}
pid = 0
for pat in pat2valid.keys():
	fo.write(str(pid) + '\t' + pat + '\t+\t' + str(pat2cnt[pat]) + '\n')
	pat2pid[(pat, '+')] = pid
	pid += 1
	fo.write(str(pid) + '\t' + pat + '\t-\t' + str(pat2cnt[pat]) + '\n')
	pat2pid[(pat, '-')] = pid
	pid += 1
fo.close()

print 'Extracting valid facts...'
fact2valid = {}
for u, v, pat in data:
	if pat2valid.get(pat, 0) == 1:
		if u > v:
			u, v = v, u
		fact2valid[(u, v)] = 1

print 'Writing valid facts...'
fo = open(fact_file, 'w')
fact2fid = {}
fid = 0
for u, v in fact2valid.keys():
	fo.write(str(fid) + '\t' + u + '\t' + v + '\n')
	fact2fid[(u, v)] = fid
	fid += 1
	fo.write(str(fid) + '\t' + v + '\t' + u + '\n')
	fact2fid[(v, u)] = fid
	fid += 1
fo.close()

print 'Extracting valid links...'
pidfid2cnt = {}
for u, v, pat in data:
	fid = fact2fid.get((u, v), -1)
	pid = pat2pid.get((pat, '+'), -1)
	if fid != -1 and pid != -1:
		pidfid2cnt[(pid, fid)] = 1

	fid = fact2fid.get((v, u), -1)
	pid = pat2pid.get((pat, '-'), -1)
	if fid != -1 and pid != -1:
		pidfid2cnt[(pid, fid)] = 1

print 'Writing valid links...'
fo = open(link_file, 'w')
for pid, fid in pidfid2cnt.keys():
	fo.write(str(pid) + '\t' + str(fid) + '\n')
fo.close()



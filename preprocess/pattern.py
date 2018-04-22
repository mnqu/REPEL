import sys
import os
import itertools

def read_sentences(file_name):
	sentences = []
	fi = open(file_name, 'r')
	sen = []
	for line in fi:
		if line == '\n':
			to = [[] for k in range(len(sen))]
			for entry in sen:
				pst = entry[0]
				frm = entry[4]
				to[frm].append(pst)
			for k in range(len(sen)):
				sen[k].append(to[k])
			sentences.append(sen)
			sen = []
		else:
			lst = line.strip().split('\t')
			pst = int(lst[0]) - 1
			word = lst[2]
			pos = lst[3]
			ner = lst[4]
			#if ner != 'O':
			#	ner = 'ENT'
			frm = int(lst[5]) - 1
			rel = lst[6]
			sen.append([pst, word, pos, ner, frm, rel])
	return sentences

def find_entity(sentence):
	entity_list = []
	last_ner = 'O'
	bg, ed = -1, -1
	for entry in sentence:
		pst = entry[0]
		word = entry[1]
		ner = entry[3]
		if ner != last_ner:
			if last_ner == 'O':
				p = pst
			else:
				q = pst
				entity_list.append((p, q, last_ner))
		last_ner = ner
	return entity_list

def shortest_path(sentence, entu, entv):
	u = entu[1] - 1
	v = entv[1] - 1
	queue = [[u]]
	flag = {}
	while queue != []:
		path = queue[0]
		u = path[-1]
		del(queue[0])
		if u == v:
			spath = []
			for k in path:
				if k in range(entu[0], entu[1]):
					continue
				if k in range(entv[0], entv[1]):
					continue
				spath.append(k)
			return [range(entu[0], entu[1]), range(entv[0], entv[1]), spath]
		if flag.get(u, 0) == 1:
			continue
		flag[u] = 1
		nb_list = itertools.chain(sentence[u][6], [sentence[u][4]])
		for x in nb_list:
			new_path = list(path)
			new_path.append(x)
			queue.append(new_path)
	return []

def generate_seq_text(sentence, seq):
	text = ''
	for pst in seq:
		entry = sentence[pst]
		text += entry[1] + ' '
	return text.strip()

def generate_path(sentence, path, tp=True):
	seq = path[2]
	seq.append(path[0][0])
	seq.append(path[1][0])
	seq = sorted(seq)

	text = ''
	for pst in seq:
		if pst in path[0]:
			if pst == path[0][0]:
				if tp == True:
					#text += ' $' + sentence[pst][3]
					text += ' <Ent>'
				else:
					text += ' ' + sentence[pst][1]
			else:
				if tp == False:
					text += '_' + sentence[pst][1]
		elif pst in path[1]:
			if pst == path[1][0]:
				if tp == True:
					#text += ' $' + sentence[pst][3]
					text += ' <Ent>'
				else:
					text += ' ' + sentence[pst][1]
			else:
				if tp == False:
					text += '_' + sentence[pst][1]
		else:
			text += ' ' + sentence[pst][1]
			#text += ' ' + sentence[pst][1] + '/' + sentence[pst][2].strip()
	return text.strip()

def generate_expansion(sentence, path, tp=True):
	out_dict = {}
	for pst in itertools.chain(path[0], path[1], path[2]):
		entry = sentence[pst]
		pos = entry[2]

		queue = [pst]
		while queue != []:
			cpst = queue[0]
			centry = sentence[cpst]
			cpos = centry[2]
			del queue[0]

			if out_dict.get(cpst, 0) == 1:
				continue
			out_dict[cpst] = 1

			if cpos[0:2] == 'VB':
				for npst in itertools.chain(centry[6], [centry[4]]):
					if npst <= cpst:
						continue
					if sentence[npst][2][0:2] == 'NN':
						queue.append(npst)
			elif cpos[0:2] == 'NN':
				for npst in itertools.chain(centry[6], [centry[4]]):
					if npst >= cpst:
						continue
					if sentence[npst][2][0:2] == 'NN':
						queue.append(npst)
					elif sentence[npst][2][0:2] == 'IN':
						queue.append(npst)
					elif sentence[npst][2][0:2] == 'TO':
						queue.append(npst)
					#elif sentence[npst][2][0:2] == 'DT':
	out_list = sorted(out_dict.keys())
	text = ''
	for pst in out_list:
		if pst in path[0]:
			if pst == path[0][0]:
				if tp == True:
					#text += ' $' + sentence[pst][3]
					text += ' <Ent>'
				else:
					text += ' ' + sentence[pst][1]
			else:
				if tp == False:
					text += '_' + sentence[pst][1]
		elif pst in path[1]:
			if pst == path[1][0]:
				if tp == True:
					#text += ' $' + sentence[pst][3]
					text += ' <Ent>'
				else:
					text += ' ' + sentence[pst][1]
			else:
				if tp == False:
					text += '_' + sentence[pst][1]
		else:
			text += ' ' + sentence[pst][1]
			#text += ' ' + sentence[pst][1] + '/' + sentence[pst][2].strip()
	return text.strip()


file_name = sys.argv[1]
fo = open(sys.argv[2], 'w')
sens = read_sentences(file_name)
print 'Number of sentences:', len(sens)
cnt = 0
for sen in sens:
	if cnt % 1000 == 0:
		print cnt
	cnt += 1
	
	if len(sen) >= 35:
		continue

	try:
		ents = find_entity(sen)
		for i in range(len(ents)):
			for j in range(len(ents)):
				if i >= j:
					continue
				typei = sen[ents[i][0]][3]
				typej = sen[ents[j][0]][3]

				path = shortest_path(sen, ents[i], ents[j])
				texti = generate_seq_text(sen, range(ents[i][0], ents[i][1])).replace(' ', '_')
				textj = generate_seq_text(sen, range(ents[j][0], ents[j][1])).replace(' ', '_')
				fo.write(typei + '\t' + typej + '\n')
				fo.write(texti + '\t' + textj + '\n')
				fo.write(generate_path(sen, path) + '\n')
				fo.write(generate_seq_text(sen, range(len(sen))) + '\n')
				fo.write('\n')
	except Exception as inst:
		print type(inst)
		continue
fo.close()


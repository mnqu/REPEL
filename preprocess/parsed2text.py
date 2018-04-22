import sys
import os

data = []
fi = open(sys.argv[1], 'r')
fo = open(sys.argv[2], 'w')
cnt = 0
for line in fi:
	if cnt % 1000000 == 0:
		print cnt
	cnt += 1

	if line == '\n':
		last_ner = 'O'
		lst = []
		for entry in data:
			items = entry.strip().split('\t')
			word = items[1].lower()
			ner = items[4]
			if ner == 'O':
				lst.append([[word, ner]])
			else:
				if ner == last_ner:
					lst[-1].append([word, ner])
				else:
					lst.append([[word, ner]])
			last_ner = ner
		
		string = ''
		for unit in lst:
			if len(unit) == 1:
				text = unit[0][0]
				string += text + ' '
			else:
				text = ''
				ner = unit[0][1]
				for pair in unit:
					text += pair[0] + ' '
				string += text.strip().replace(' ', '_') + '||' + ner + '||' + ' '
		fo.write(string.strip() + '\n')
		
		data = []
		continue
	data.append(line)

fi.close()
fo.close()

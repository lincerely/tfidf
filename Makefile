doc_dir = $(HOME)/Documents/box/

sanitizer = -fsanitize=address -fsanitize=undefined -fno-sanitize-recover=all -fsanitize=float-divide-by-zero -fsanitize=float-cast-overflow -fno-sanitize=null -fno-sanitize=alignment

tfidf: tfidf.c stem.c
	gcc-10 -Wall -fopenmp -Ofast stem.c tfidf.c -o tfidf

tfidf.debug: tfidf.c stem.c
	gcc-10 -Wall -fopenmp -Ofast -g $(sanitizer) stem.c tfidf.c -o tfidf.debug

filelist.txt:
	# get list of files excluding executables and directories
	(cd $(doc_dir); ls -F | grep -v -e '*' -e '/' ) |\
	   	sed s,^,$(doc_dir), > filelist.txt

.PHONY: profile profile.debug run install test

profile: tfidf filelist.txt
	time ./tfidf filelist.txt >/dev/null

profile.debug: tfidf.debug filelist.txt
	time ./tfidf.debug filelist.txt >/dev/null

run: tfidf filelist.txt
	./tfidf filelist.txt > /tmp/sim.csv

test: tfidf filelist.txt
	time tfidf filelist.txt > /tmp/sim_before.csv
	time ./tfidf filelist.txt > /tmp/sim.csv
	diff /tmp/sim_before.csv /tmp/sim.csv | head -n20

install: tfidf
	cp tfidf $(HOME)/bin/

unlinked.csv: tfidf unlinked.awk filelist.txt
	./tfidf filelist.txt | sort -rk 3 |\
		awk -v dir=$(doc_dir) -f unlinked.awk > unlinked.csv


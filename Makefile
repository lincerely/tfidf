doc_dir = $(HOME)/Documents/box/

sanitizer = -fsanitize=address -fsanitize=undefined -fno-sanitize-recover=all -fsanitize=float-divide-by-zero -fsanitize=float-cast-overflow -fno-sanitize=null -fno-sanitize=alignment

tfidf: tfidf.c stem.c
	gcc-10 -Wall -fopenmp -Ofast stem.c tfidf.c -o tfidf

tfidf.debug: tfidf.c stem.c
	gcc-10 -Wall -fopenmp -Ofast -g $(sanitizer) stem.c tfidf.c -o tfidf.debug

tfidf.clang: tfidf.c stem.c
	clang -Wall -Xclang -fopenmp -lomp -g3 stem.c tfidf.c -o tfidf.clang

filelist.txt:
	# get list of files excluding executables and directories
	(cd $(doc_dir); ls -F | grep -v -e '*' -e '/' ) |\
	   	sed s,^,$(doc_dir), > filelist.txt

.PHONY: profile profile.debug samply install test

profile: tfidf filelist.txt
	time ./tfidf filelist.txt >/tmp/sim.csv

profile.debug: tfidf.debug filelist.txt
	time ./tfidf.debug filelist.txt >/tmp/sim.csv

samply: tfidf.clang
	samply record ./tfidf.clang filelist.txt >/tmp/sim.csv

test: tfidf filelist.txt
	time tfidf filelist.txt > /tmp/sim_before.csv
	time ./tfidf filelist.txt > /tmp/sim.csv
	diff /tmp/sim_before.csv /tmp/sim.csv | head -n20

install: tfidf
	cp tfidf $(HOME)/bin/

unlinked.csv: tfidf unlinked.awk filelist.txt
	./tfidf filelist.txt | sort -rk 3 |\
		awk -v dir=$(doc_dir) -f unlinked.awk > unlinked.csv


tfidf: tfidf.c stem.c
	gcc-10 -Wall -fopenmp -Ofast stem.c tfidf.c -o tfidf

filelist.txt: 
	(cd ~/Documents/box; zk l | tr " " "\n" ) |\
	   	sed s,^,/Users/lincoln/Documents/box/, > filelist.txt

.PHONY: profile run install test

profile: tfidf
	time ./tfidf filelist.txt >/dev/null

run: tfidf
	./tfidf filelist.txt > /tmp/sim.csv

test: tfidf
	tfidf filelist.txt | head -n20 > /tmp/sim_from.csv
	./tfidf filelist.txt | head -n20 > /tmp/sim_to.csv

install: tfidf
	cp tfidf ~/bin/

unlinked.csv: tfidf unlinked.awk filelist.txt
	./tfidf filelist.txt | sort -rk 3 | awk -f unlinked.awk > unlinked.csv


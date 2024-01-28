tfidf: tfidf.c stem.c
	gcc-10 -Wall -fopenmp -Ofast stem.c tfidf.c -o tfidf

.PHONY: profile run install

profile: tfidf
	time ./tfidf filelist.txt >/dev/null

run: tfidf
	(cd ~/Documents/box; zk l | tr " " "\n") |\
	   	sed s,^,/Users/lincoln/Documents/box/, | ./tfidf > /tmp/sim.csv

install: tfidf
	cp tfidf ~/bin/

unlinked.csv: tfidf unlinked.awk filelist.txt
	./tfidf filelist.txt | sort -rk 3 | awk -f unlinked.awk > unlinked.csv


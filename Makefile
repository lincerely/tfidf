tfidf: tfidf.c stem.c
	gcc-10 -Wall -fopenmp -Ofast stem.c tfidf.c -o tfidf

.PHONY: profile
profile: tfidf
	time ./tfidf filelist.txt >/dev/null

unlinked.csv: tfidf unlinked.awk filelist.txt
	./tfidf filelist.txt | sort -rk 3 | awk -f unlinked.awk > unlinked.csv

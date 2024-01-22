tfidf: tfidf.c stem.c
	gcc-10 -Wall -Ofast stem.c tfidf.c -o tfidf

unlinked.csv: tfidf unlinked.awk filelist.txt
	./tfidf filelist.txt | sort -rk 3 | awk -f unlinked.awk > unlinked.csv

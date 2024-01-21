tfidf: tfidf.c
	gcc -Wall -fsanitize=address -g tfidf.c -o tfidf

unlinked.csv: tfidf unlinked.awk filelist.txt
	./tfidf filelist.txt | sort -rk 3 | awk -f unlinked.awk > unlinked.csv

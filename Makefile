tfidf: tfidf.c map.c map.h
	gcc -Wall -fsanitize=address -g map.c tfidf.c -o tfidf

unlinked.csv: tfidf unlinked.awk filelist.txt
	./tfidf filelist.txt | sort -rk 3 | awk -f unlinked.awk > unlinked.csv

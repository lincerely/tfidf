tfidf: tfidf.c map.c map.h
	gcc -Wall -fsanitize=address -g map.c tfidf.c -o tfidf


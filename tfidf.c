// return the tfidf cosine similarity matrix for all files in the directory
// reference:
//  - https://www.sejuku.net/blog/26420
//  - https://atmarkit.itmedia.co.jp/ait/articles/2112/23/news028.html
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <stdint.h>
#include <math.h>
#include <ctype.h>
#include <errno.h>
#include <sys/time.h>

extern int stem (char *p, int i, int j);

long microsec() {
	struct timeval t;
	gettimeofday(&t, NULL);
	return 1000000*t.tv_sec + t.tv_usec;
}

#define MAP_SIZE 5987
#define MAX_DOC 1024
#define MAX_TERM (1024*8)

uint16_t hash(const char *str) {
	unsigned int sum = 0;
	for (const char *c = str; *c != '\0'; c++)
		sum = *c + 5 * sum;
	return sum % MAP_SIZE;
}

struct keyval {
	char* key;
	float value;
	struct keyval* next;
};

struct keyval* map_get(struct keyval *map[MAP_SIZE], const char *key)
{
	uint16_t h = hash(key);
	for (struct keyval *kv = map[h]; kv != NULL; kv = kv->next)
		if (strcmp(key, kv->key) == 0)
			return kv;
	return NULL;
}

void map_insert(struct keyval *map[MAP_SIZE], const char *key, float value)
{
	uint16_t h = hash(key);
	struct keyval *kv = malloc(sizeof(struct keyval));
	kv->next = map[h];
	kv->key = malloc(sizeof(*key) * (strlen(key)+1));
	strcpy(kv->key, key);
	kv->value = value;
	map[h] = kv;
	return;
}

void map_debug(struct keyval *map[MAP_SIZE])
{
	for (int m = 0; m < MAP_SIZE; m++) {
		int count = 0;
		for (struct keyval *kv = map[m]; kv != NULL; kv = kv->next) {
			count++;
		}
		fprintf(stderr, "map[%04d]:\t%d\n", m, count);
	}
}

void map_free(struct keyval *map[MAP_SIZE])
{
	for (int i = 0; i < MAP_SIZE; i++) {
		while (map[i] != NULL) {
			struct keyval *next = map[i]->next;
			free(map[i]->key);
			free(map[i]);
			map[i] = next;
		}
	}
}

int main(int argc, char **argv)
{

	FILE *listfp = stdin;
	if (argc > 1) {
		listfp = fopen(argv[1], "r");
		if (listfp == NULL) {
			fprintf(stderr, "failed to open file list %s: %s\n", argv[1], strerror(errno));
			return 1;
		}
	}

	// term frequency: #term / #doc_term_count
	// document frequency: #docs_with_term / #all_docs
	// for all doc:
	// 	if termCount[doc][term] == 0:
	// 	  termDocCount[term]++;
	//  termCount[doc][term]++;
	//

	struct keyval **termMap = calloc(MAP_SIZE, sizeof(*termMap));
	int *termCounts[MAX_DOC];
	int termDocCounts[MAX_TERM] = {0};
	int docTermCounts[MAX_DOC] = {0};
	char *docfnames[MAX_DOC] = {0};
	int docCount = 0;
	int termCount = 0;

	long starttime = microsec();

	int d = 0;
	char *fname = NULL;
	size_t len = 0;
	while(getline(&fname, &len, listfp) != -1) {

		termCounts[d] = calloc(MAX_TERM, sizeof(*termCounts[d]));

		fname[strlen(fname)-1] = '\0';

		FILE *fp = fopen(fname, "r");
		if (fp == NULL) {
			fprintf(stderr, "failed to open %s: %s\n", fname, strerror(errno));
			return 1;
		}

		docfnames[d] = malloc(sizeof(char) * (len+1));
		strcpy(docfnames[d], fname);

		char *line = NULL;
		size_t linelen = 0;
		while (getline(&line, &linelen, fp) != -1) {

			char *str, *last, *token;
			for (str = line; ; str = NULL) {

				const char *delim = " \'\t\v\f\r\n!\"#$%&()*+,-./:;<=>?@[\\]^_`{|}~“”";
				token = strtok_r(str, delim, &last);
				if (token == NULL) {
					break;
				}

				for (char *c = token; *c != '\0'; c++) {
					if (!(ispunct(*c) || isspace(*c))) {
						break;
					}
				}

				for (char *c = token+strlen(token)-1; c > token; c--) {
					if (!(ispunct(*c) || isspace(*c))) {
						break;
					}
				}

				for (char *c = token; *c != '\0'; c++) {
					*c = tolower(*c);
				}

				if (*token == '\0') {
					continue;
				}

				int endc = stem(token, 0, strlen(token)-1);

				if (endc == 0) {
					continue;
				}

				token[endc+1] = '\0';

				int key;
				struct keyval *t_kv = map_get(termMap, token);
				if (t_kv == NULL) {
					key = termCount;
					map_insert(termMap, token, key);
					termCount++;

					if (termCount >= MAX_TERM) {
						fprintf(stderr, "maximum term limit reached: %d\n", MAX_TERM);
						return 1;
					}
				} else {
					key = t_kv->value;
				}

				docTermCounts[d]++;

				if (termCounts[d][key] == 0) {
					termDocCounts[key]++;
				}
				termCounts[d][key]++;
			}
		}
		if (ferror(fp)) {
			fprintf(stderr, "failed to getline from %s: %s\n", fname, strerror(errno));
			return 1;
		}
		free(line);
		fclose(fp);
		d++;
	}
	free(fname);
	docCount = d;

	fprintf(stderr, "token:\t%ld\n", microsec() -starttime);
	starttime = microsec();

	// calculate tfidf
	// tfidf = tf * log(docCount / df)

	float *tfidfs[docCount];
	#pragma omp parallel for
	for (int d = 0; d < docCount; d++) {
		tfidfs[d] = malloc(sizeof(*tfidfs[d]) * termCount);

		for (int t = 0; t < termCount; t++) {
			float tf = (float)termCounts[d][t];
			float df = (float)termDocCounts[t];
			tfidfs[d][t] = tf * log((float)docCount / df);
		}
	}

	fprintf(stderr, "tfidf\t%ld\n", microsec()-starttime);
	starttime = microsec();

	// for each doc
	//   calc magnitude
	// for each doc
	//   for each other doc
	//     calc dot product
	//     dot product / magnitude

	float mag[docCount];
	#pragma omp parallel for
	for (int d = 0; d < docCount; d++) {
		float sum = 0;
		float *x = tfidfs[d];
		for (int t = 0; t < termCount;  t++) {
			sum += x[t]*x[t];
		}
		mag[d] = sqrt(sum);
	}

	fprintf(stderr, "magni\t%ld\n", microsec()-starttime);
	starttime = microsec();

	float dots[docCount*docCount];
	#pragma omp parallel for
	for (int d1 = 0; d1 < docCount - 1; d1++) {
		for (int d2 = d1+1; d2 < docCount; d2++) {
			float s[8] = {0};
			float *x = tfidfs[d1];
			float *y = tfidfs[d2];
			for (int t = 0; t < termCount; t+=8) {
				s[0] += x[t+0] * y[t+0];
				s[1] += x[t+1] * y[t+1];
				s[2] += x[t+2] * y[t+2];
				s[3] += x[t+3] * y[t+3];
				s[4] += x[t+4] * y[t+4];
				s[5] += x[t+5] * y[t+5];
				s[6] += x[t+6] * y[t+6];
				s[7] += x[t+7] * y[t+7];
			}
			dots[d1*docCount+d2] = s[0]+s[1]+s[2]+s[3]+s[4]+s[5]+s[6]+s[7];
		}
	}

	fprintf(stderr, "dotprod\t%ld\n", microsec()-starttime);
	starttime = microsec();

	float similarities[docCount*docCount];
	#pragma omp parallel for
	for (int d1 = 0; d1 < docCount - 1; d1++) {
		for (int d2 = d1+1; d2 < docCount; d2++) {
			similarities[d1*docCount+d2] = dots[d1*docCount+d2]/(mag[d1]*mag[d2]);
		}
	}

	fprintf(stderr, "cosine\t%ld\n", microsec()-starttime);
	starttime = microsec();

	for (int d1 = 0; d1 < docCount - 1; d1++) {
		for (int d2 = d1+1; d2 < docCount; d2++) {
			printf("%s, %s, %.4f\n", docfnames[d1], docfnames[d2], similarities[d1*docCount+d2]);
		}
	}

	fprintf(stderr, "printf\t%ld\n", microsec()-starttime);
	return 0;
}

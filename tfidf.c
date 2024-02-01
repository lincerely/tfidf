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

uint16_t hash(const char *str)
{
	unsigned int sum = 0;
	for (const char *c = str; *c != '\0'; c++)
		sum = *c + 5 * sum;
	return sum % MAP_SIZE;
}

struct keyval {
	char* key;
	int value;
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

void map_insert(struct keyval *map[MAP_SIZE], const char *key, int value)
{
	uint16_t h = hash(key);
	struct keyval *kv = malloc(sizeof(struct keyval));
	if (kv == NULL) {
		perror("failed to malloc keyval");
		exit(1);
	}
	kv->next = map[h];
	kv->key = strdup(key);
	if (kv->key == NULL) {
		perror("failed to strdup key");
		exit(1);
	}
	kv->value = value;
	map[h] = kv;
	return;
}

void map_debug(struct keyval *map[MAP_SIZE])
{
	for (int m = 0; m < MAP_SIZE; m++) {
		int count = 0;
		for (struct keyval *kv = map[m]; kv != NULL; kv = kv->next) {
			fprintf(stderr, "%s %d\n", kv->key, kv->value);
			count++;
		}
		//fprintf(stderr, "map[%04d]:\t%d\n", m, count);
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

/* replace printf("%s, %s, %.4f\n", ...) */
static char* printsimilarity(char * restrict p, const char * restrict docfname1, const char * restrict docfname2, float sim)
{
	const char *sep = ", ";
	const char *sep2 = ", 0.";

	for(const char *c = docfname1; *c != '\0'; c++)
		*(p++) = *c;

	for(const char *c = sep; *c != '\0'; c++)
		*(p++) = *c;

	for(const char *c = docfname2; *c != '\0'; c++)
		*(p++) = *c;

	for(const char *c = sep2; *c != '\0'; c++)
		*(p++) = *c;

	int s = (int)(sim*10000+0.5);
	*(p++) = s/1000+'0';
	*(p++) = s/100%10+'0';
	*(p++) = s/10%10+'0';
	*(p++) = s%10+'0';
	*(p++) = '\n';

	return p;
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
	int docUniqueTermCounts[MAX_DOC] = {0};
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

		docfnames[d] = strdup(fname);
		if (docfnames[d] == NULL) {
			perror("failed to strdup fname");
			exit(1);
		}

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
					docUniqueTermCounts[d]++;
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
	map_free(termMap);
	docCount = d;

	fprintf(stderr, "token:\t%ld\n", microsec() -starttime);
	starttime = microsec();

	// calculate tfidf
	// tfidf = tf * log(docCount / df)

	struct tfidf {
		int term;
		float value;
	};
	struct tfidf *tfidfs[docCount];
	#pragma omp parallel for
	for (int d = 0; d < docCount; d++) {

		tfidfs[d] = malloc(sizeof(*tfidfs[d]) * docUniqueTermCounts[d]);
		if (tfidfs[d] == NULL) {
			perror("failed to malloc tfidfs");
			exit(1);
		}

		for (int t = 0, i = 0; t < termCount; t++) {
			if (termCounts[d][t] > 0) {
				float tf = (float)termCounts[d][t]/(float)docTermCounts[d];
				float idf = (float)docCount / (float)termDocCounts[t];
				tfidfs[d][i].term = t;
				tfidfs[d][i].value = tf * log(idf);
				i++;
			}
		}
	}

	fprintf(stderr, "tfidf\t%ld\n", microsec()-starttime);
	starttime = microsec();

	// for each doc
	//   calc magnitude
	// for each doc
	//   for each other doc
	//     calc dot product

	float mag[docCount];
	#pragma omp parallel for
	for (int d = 0; d < docCount; d++) {
		float sum = 0;
		for (int i = 0; i < docUniqueTermCounts[d]; i++) {
			sum += tfidfs[d][i].value*tfidfs[d][i].value;
		}
		mag[d] = sqrt(sum);
	}

	fprintf(stderr, "magni\t%ld\n", microsec()-starttime);
	starttime = microsec();

	float dots[docCount*docCount];
	#pragma omp parallel for
	for (int d1 = 0; d1 < docCount - 1; d1++) {
		float *map = calloc(termCount, sizeof(*map));
		int t1max = docUniqueTermCounts[d1];
		for (int t1 = 0; t1 < t1max; t1++) {
			map[tfidfs[d1][t1].term] = tfidfs[d1][t1].value;
		}
		for (int d2 = d1+1; d2 < docCount; d2++) {
			float s = 0;
			int t2max = docUniqueTermCounts[d2];
			for(int t2 = 0; t2 < t2max; t2++) {
				struct tfidf t = tfidfs[d2][t2];
				s += map[t.term] * t.value;
			}
			dots[d1*docCount+d2] = s;
		}
	}
	for (int d = 0; d < docCount; d++) {
		free(tfidfs[d]);
	}

	fprintf(stderr, "dotprod\t%ld\n", microsec()-starttime);
	starttime = microsec();

	// for each doc
	//   for each other doc
	//     dot product / magnitude

	float similarities[docCount*docCount];
	#pragma omp parallel for
	for (int d1 = 0; d1 < docCount - 1; d1++) {
		for (int d2 = d1+1; d2 < docCount; d2++) {
			similarities[d1*docCount+d2] =
				dots[d1*docCount+d2]/(mag[d1]*mag[d2]);
		}
	}

	fprintf(stderr, "cosine\t%ld\n", microsec()-starttime);
	starttime = microsec();

	for (int d1 = 0; d1 < docCount - 1; d1++) {
		const int B = 16;
		char buf[(512+12)*B];
		int d2 = d1+1;
		for (; d2 < docCount-B; d2=d2+B) {
			char *p = buf;
			for(int i = 0; i < B; i++) {
				p = printsimilarity(p, docfnames[d1], docfnames[d2+i], similarities[d1*docCount+d2+i]);
			}
			fwrite(buf, 1, p-buf, stdout);
		}
		for (; d2 < docCount; d2++) {
			char *p = printsimilarity(buf, docfnames[d1], docfnames[d2], similarities[d1*docCount+d2]);
			fwrite(buf, 1, p-buf, stdout);
		}
	}

	fprintf(stderr, "printf\t%ld\n", microsec()-starttime);
	return 0;
}

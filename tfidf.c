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

const uint16_t MAP_SIZE = 64;
const uint16_t MAX_DOC = 1024;

uint16_t hash(char *str) {
	uint16_t sum = 0;
	for (char *c = str; *c != '\0'; c++)
		sum += *c;
	return sum % MAP_SIZE;
}

struct keyval {
	char* key;
	float value;
	struct keyval* next;
};

struct keyval* map_get(struct keyval *map[MAP_SIZE], char *key)
{
	uint16_t h = hash(key);
	for (struct keyval *kv = map[h]; kv != NULL; kv = kv->next)
		if (strcmp(key, kv->key) == 0)
			return kv;
	return NULL;
}

void map_set(struct keyval *map[MAP_SIZE], char *key, float value)
{
	uint16_t h = hash(key);
	for (struct keyval *kv = map[h]; kv != NULL; kv = kv->next) {
		if (strcmp(key, kv->key) == 0) {
			kv->value = value;
			return;
		}
	}

	struct keyval *kv = malloc(sizeof(struct keyval));
	kv->next = map[h];
	kv->key = key;
	kv->value = value;
	map[h] = kv;
	return;
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

	struct keyval *termCounts[MAX_DOC][MAP_SIZE];
	struct keyval* termDocCounts[MAP_SIZE] = {0};
	int docTermCounts[MAX_DOC] = {0};
	char *docfnames[MAX_DOC] = {0};
	int docCount = 0;

	int d = 0;
	char *fname = NULL;
	size_t len = 0;
	while(getline(&fname, &len, listfp) != -1) {

		memset(termCounts[d], 0, sizeof(*termCounts[d]));

		int lastc = strlen(fname);
		fname[lastc-1] = '\0';

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

				const char *delim = " \t\v\f\r\n!\"#$%&()*+,-./:;<=>?@[\\]^_`{|}~";
				token = strtok_r(str, delim, &last);
				if (token == NULL) {
					break;
				}

				// trim front
				for (char *c = token; *c != '\0'; c++) {
					if (ispunct(*c) || isspace(*c)) {
						token++;
					} else {
						break;
					}
				}

				// trim back
				for (char *c = token+strlen(token)-1; c > token; c--) {
					if (ispunct(*c) || isspace(*c)) {
						*c = '\0';
					} else {
						break;
					}
				}

				for (char *c = token; *c != '\0'; c++) {
					*c = tolower(*c);
				}

				if (*token == '\0') {
					continue;
				}

				char *term = malloc(sizeof(char)*(strlen(token)+1));
				strcpy(term, token);

				docTermCounts[d]++;

				struct keyval* t_kv = map_get(termCounts[d], term);

				if (t_kv != NULL) {
					t_kv->value++;
				} else {
					map_set(termCounts[d], term, 1);

					// first appearence of this term in this doc
					struct keyval *td_kv = map_get(termDocCounts, term);
					if (td_kv != NULL) {
						td_kv->value++;
					} else {
						map_set(termDocCounts, term, 1);
					}
				}
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

	// calculate tfidf
	// tfidf = tf * log(docCount / df)

	struct keyval *tfidfs[MAX_DOC][MAP_SIZE];

	for (int d = 0; d < docCount; d++) {

		memset(tfidfs[d], 0, sizeof(*tfidfs[d]));

		for (int i = 0; i < MAP_SIZE; i++) {
			for (struct keyval *kv=termCounts[d][i]; kv!=NULL; kv=kv->next) {

				float tf = (float)kv->value / docTermCounts[d];

				struct keyval *df_kv = map_get(termDocCounts, kv->key);
				float tfidf = tf * log((float)docCount / df_kv->value);

				map_set(tfidfs[d], kv->key, tfidf);
				//printf("%s(%.4f) ", kv->key, tfidf);
			}
		}
		//printf("\n");
	}

	// for each doc
	//   calc magnitude
	// for each doc
	//   for each other doc
	//     calc dot product
	//     dot product / magnitude

	float mag[MAX_DOC] = {0};
	for (int d = 0; d < docCount; d++) {
		float sum = 0;
		for (int i = 0; i < MAP_SIZE; i++)
			for (struct keyval *kv=tfidfs[d][i]; kv!=NULL; kv=kv->next)
				sum += (float)kv->value*kv->value;

		mag[d] = sqrt(sum);
	}

	for (int d1 = 0; d1 < docCount - 1; d1++) {
		for (int d2 = d1+1; d2 < docCount; d2++) {
			float dot = 0;
			for (int i = 0; i < MAP_SIZE; i++) {
				for (struct keyval *kv=tfidfs[d1][i]; kv!=NULL; kv=kv->next) {
					struct keyval *kv2 = map_get(tfidfs[d2], kv->key);
					if (kv2 != NULL)
						dot += kv->value * kv2->value;
				}
			}
			float similarity = dot/(mag[d1]*mag[d2]);
			printf("%s, %s, %.4f\n", docfnames[d1], docfnames[d2], similarity);
		}
	}

	map_free(termDocCounts);
	for (int d = 0; d < docCount; d++)
		map_free(termCounts[d]);

	return 0;
}

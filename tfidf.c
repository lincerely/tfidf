// reference: 
// https://www.sejuku.net/blog/26420
// https://atmarkit.itmedia.co.jp/ait/articles/2112/23/news028.html
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <stdint.h>
#include <math.h>

const uint16_t MAP_SIZE = 64;
const uint16_t MAX_DOC = 1024;

char *terms[] = {
    "犬", "可愛い", "犬", "大きい", NULL,
    "猫", "小さい", "猫", "可愛い", "可愛い", NULL,
    "虫", "小さい", "可愛くない", NULL,
	NULL,
};

uint16_t hash(char *str) {
	uint16_t sum = 0;
	for (char *c = str; *c != '\0'; c++)
		sum += *c;
	return sum % MAP_SIZE;
}

struct keyval {
	char* key; // external
	float value;
	struct keyval* next; // free by map_free()
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
	struct keyval* kv = map_get(map, key);
	if (kv == NULL) {
		uint16_t h = hash(key);
		kv = malloc(sizeof(struct keyval));
		kv->next = map[h];
		kv->key = key;
		map[h] = kv;
	}
	kv->value = value;
	return;
}

void map_free(struct keyval *map[MAP_SIZE])
{
	for (int i = 0; i < MAP_SIZE; i++) {
		while (map[i] != NULL) {
			struct keyval *next = map[i]->next;
			free(map[i]);
			map[i] = next;
		}
	}
}

int main(void)
{
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
	int docCount = 0;

	int d = 0;
	for (int i = 0; terms[i] != NULL; i++) {

		memset(termCounts[d], 0, sizeof(*termCounts[d]));

		for (; terms[i] != NULL; i++) {

			docTermCounts[d]++;

			struct keyval* kv = map_get(termCounts[d], terms[i]);

			if (kv == NULL) { 
				map_set(termCounts[d], terms[i], 1);

				// first appearence of this term in this doc
				kv = map_get(termDocCounts, terms[i]);
				if (kv == NULL) {
					map_set(termDocCounts, terms[i], 1);
				} else {
					kv->value++;
				}
			} else {
				kv->value++;
			}
		}
		d++;
	}
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
				printf("%s(%.4f) ", kv->key, tfidf);
			}
		}
		printf("\n");
	}

	//  for each doc
	//  	calc magnitude
	// for each doc
	//  for each other doc
	//  	calc dot product
	//  	dot product / magnitude
	//
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
			printf("%d, %d, %.4f\n", d1, d2, similarity);
		}
	}

	map_free(termDocCounts);
	for (int d = 0; d < docCount; d++)
		map_free(termCounts[d]);

	return 0;
}

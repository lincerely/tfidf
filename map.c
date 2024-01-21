#include <string.h>
#include <stdlib.h>

#include "map.h"

uint16_t hash(char *str) {
	uint16_t sum = 0;
	for (char *c = str; *c != '\0'; c++)
		sum += *c;
	return sum % MAP_SIZE;
}

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
			//free(map[i]->key);
			free(map[i]);
			map[i] = next;
		}
	}
}
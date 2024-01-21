#ifndef MAP_H
#define MAP_H

#include <stdint.h>

#define MAP_SIZE 64

struct keyval {
	char* key;
	float value;
	struct keyval* next;
};

uint16_t hash(char *str);
struct keyval* map_get(struct keyval *map[MAP_SIZE], char *key);
void map_set(struct keyval *map[MAP_SIZE], char *key, float value);
void map_free(struct keyval *map[MAP_SIZE]);

#endif // MAP_H
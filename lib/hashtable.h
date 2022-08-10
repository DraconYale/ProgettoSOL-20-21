#ifndef HASHTABLE_H_DEFINED
#define HASHTABLE_H_DEFINED

#include <stdlib.h>
#include <string.h>

#include <hash.h>
#include <list.h>


typedef struct hashItem hashItem;

typedef struct hashTable hashTable;

int hashFunc(const char* s);

hashTable* hashTableInit(int maxFiles);

int hashInsert(hashTable* table, char* key, void* content);

int hashRemove(hashTable* table, char* key);

void* hashSearch(hashTable* table, char* key);

#endif

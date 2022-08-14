#include <stdlib.h>
#include <string.h>

#include <hash.h>
#include <list.h>


struct hashItem{
	char* key;
	void* content;
	struct hashItem* next;
}

struct hashTable{
	int tableSize;
	int entriesNumber;
	hashItem** items;
}

//https://cp-algorithms.com/string/string-hashing.html
int hashFunc(const char* s) {
	const int p = 31;
	const int m = 1e9 + 9;
	int hashValue = 0;
	int pPow = 1;
	for (char c : s) {
		hashValue = (hashValue + (c - 'a' + 1) * pPow) % m;
		pPow = (pPow * p) % m;
	}
	return hashValue;
}

hashTable* hashTableInit(int maxFiles){
	if(maxFiles <= 0){
		errno = EINVAL;
		return NULL;
	}
	hashTable* newTable = malloc(sizeof(hashTable));
	if(newTable == NULL){
		return NULL;	
	}
	newTable->tableSize = maxFiles;
	newTable->entriesNumber = 0;
	newTable->items = calloc(maxFiles, sizeof(list*));
	if(newTable->items == NULL){
		free(newTable);
		return NULL;	
	}
	return newTable;
}



int hashInsert(hashTable* table, char* key, void* content){
	if(table == NULL || key == NULL){
		errno = EINVAL;
		return -1;
	}
	int tmpHash = hashFunc(key) % table->tableSize;
	hashItem* newItem = malloc(sizeof(hashItem));
	newItem->key = key;
	newItem->content = content;
	newItem->next = NULL;
	hashItem* current = table->items[tmpHash];
	while(current != NULL){
		if(strcmp(current->key, key) == 0){
			current->content = content;			//updates item if exists
			free(newItem);
			return 0;
		}
		current = current->next;
	}
	newItem->next = table->items[tmpHash];
	table->items[tmpHash] = newItem;
	table->entriesNumber++;
	return 0;

}

int hashRemove(hashTable* table, void* key){
	if(table == NULL || key == NULL){
		errno = EINVAL;
		return -1;
	}
	int tmpHash = hashFunc(key) % table->tableSize;
	hashItem* current = table->items[tmpHash];
	hashItem* prev = NULL;
	while(current != NULL){
		if(strcmp(current->key, key) == 0){
			if(prev == NULL){
				table->items[tmpHash] = current->next;
			}
			else{
				prev->next = current->next;
			}
			free(current->key);
			if(current->content != NULL){
				free(current->content);
			}
			free(current);
			table->entriesNumber--;			
			return 0;
		}
		prev = current;
		current = current->next;
	}
	return -1;
}

void* hashSearch(hashTable* table, char* key){
	if(table == NULL || key == NULL){
		errno = EINVAL;
		return NULL;
	}
	int tmpHash = hashFunc(key) % table->tableSize;
	hashItem* current = table->items[tmpHash];
	while(current != NULL){
		if(strcmp(current->key, key) == 0){
			return current->content;
		}
		current = current->next;
	}
	return NULL;
}

int freeHash(hashtable* table){
	if(table == NULL){
		return 0;
	}
	int i = 0;
	for(i = 0, i < table->tablesize, i++){
		hashItem* current = table->items[i];
		hashItem* tmp = NULL;
		while(current != NULL){
			free(current->key);
			free(current->content);
			tmp = current;
			current = current->next;
			free(tmp);
		}
	}
	return 0;
}


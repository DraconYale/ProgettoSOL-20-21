#ifndef LIST_H_DEFINED
#define LIST_H_DEFINED

typedef struct elem{
	char* info;
	void* data;
	unsigned long size;
	struct elem* next;
	struct elem* prev;
}elem;

typedef struct list{
	elem* head;
	elem* tail;
	int elemNumber;
}list;

void freeElem(elem* elem);

list* initList();

elem* getHead(list* list);

elem* getTail(list* list);

void printList(list* list);

elem* appendList(list* list, char* name);

elem* appendListCont(list* list, char* name, int nameLength, void* content, unsigned long contentSize);

elem* popHead(list* list);

elem* popTail(list* list);

int containsList(list* list, char* str);

int removeList(list* list, void* str, void(*free)(void*));

elem* nextList(list* list, elem* elem);

elem* prevList(list* list, elem* elem);

int elemsNumber(list* list);

void freeList(list* list, void(*freeFunc)(void*));

#endif

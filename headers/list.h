#ifndef LIST_H_DEFINED
#define LIST_H_DEFINED

typedef struct elem{
	void* info;
	struct elem* next;
	struct elem* prev;
}elem;

typedef struct list{
	elem* head;
	elem* tail;
	int elemNumber;
}list;

list* initList();

elem* getHead(list* list);

elem* getTail(list* list);

void printList(list* list);

elem* appendList(list* list, void* content);

elem* popHead(list* list);

elem* popTail(list* list);

int containsList(list* list, char* str);

int removeList(list* list, void* str);

elem* nextList(list* list, elem* elem);

int elemsNumber(list* list);

void freeList(list* list);

#endif

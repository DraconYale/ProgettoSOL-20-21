#ifndef LIST_H_DEFINED
#define LIST_H_DEFINED

typedef struct elem elem;

typedef struct list list;

list createList();

elem* getHead(list* list);

elem* getTail(list* list);


void printList(list* list);

elem* appendList(list* list, void* content);

elem* popHead(list* list);

elem* popTail(list* list);

int containsList(list* list, char* str);

int removeList(list* list, char* str);

elem* nextList(list* list, elem* elem);

#endif

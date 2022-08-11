#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <list.h>

struct elem{
	char* info;
	struct elem* next;
	struct elem* prev;
}

struct list{
	elem* head;
	elem* tail;
	int elemNumber;
}

list initList(){

	list* newList;
	if((newList = malloc(sizeof(list))) == NULL){		//malloc sets errno = ENOMEM when fails
		return NULL;
	}
	newList->head = NULL;
	newlist->tail = NULL;
	newlist->elemNumber = 0;
	return newList;
	
}

elem* getHead(list* list){

	if(list == NULL || list->elemNumber == 0){
		return NULL;
	}
	return list->head;
	
}

elem* getTail(list* list){

	if(list == NULL || list->elemNumber == 0){
		return NULL;
	}
	return list->tail;
	
}


void printList(list* list){

	if (list == NULL || list->head == NULL){
		return;
	}	
	elem* tmp = list->head;
	while(elem != NULL) {
		printf("%s\n", (char*)elem->info);
		elem = elem->next;
	}

}

elem* appendList(list* list, char* content){

	if(list == NULL || content == NULL){
		return NULL;
	}
	elem* addedElem = NULL;
	if((addedElem = malloc(sizeof(elem))) == NULL){
		return NULL;
	}
	addedElem->info = content;
	addedElem->prev = NULL;
	addedElem->next = NULL;

	if(list->elemNumber == 0) {
		list->head = addedElem;
		list->tail = addedElem;
		list->elemNumber++;
	}
	else{
		addedElem->prev = list->tail;
		list->tail->next = addedElem;
		list->tail = addedElem;
		list->elemNumber++;
	}
	return addedElem;
}



elem* popHead(list* list){

	if(list == NULL || list->elemNumber == 0 || (list->head) == NULL){
		return NULL;
	}
	elem* tmpElem = list->head;
	list->head = list->head->next;
	if(list->head != NULL){
		list->head->prev = NULL
	}
	list->elemNumber--;

	if(list->elemNumber == 0){
		list->tail = list->head;
		if(list->head != NULL){
			list->head->next = NULL;
		}
	}
	
	return tmpElem;
}



elem* popTail(list* list){

	if(list == NULL || list->elemNumber == 0){
		return NULL;
	}

	elem* tmpElem = list->tail;
	list->tail = list->tail->prev;
	if(list->tail != NULL){
		list->tail->next = NULL;
	}
	list->elemNumber--;

	if(list->elemNumber == 0){
		list->head = list->tail;
		if(list->tail){
			list->tail->prev = NULL;
		}
	}

	return tmpElem;
}

int containsList(list* list, char* str){
	
	if (list == NULL || key == NULL || list->head == NULL){
		return 0;
	}
	elem* current = list->head;
	char* tmp;
	while(current != NULL){
		tmp = current->info;
		if(strcmp(tmp, str) == 0){
			return 1;
		}
		current = current->next;
	}
	return 0;
}

int removeList(list* list, char* str){
	if (list == NULL || str == NULL){
		errno = EINVAL;
		return -1;
	}
	elem* current = list->head;
	char* tmp;
	while(current != NULL){
		tmp = current->info;
		if(strcmp(tmp, str) != 0){
			current = current->next;
		}
		else{
			if (current->next == NULL){
				list->tail = current->prev;
			}
			if (current->prev == NULL){
				list->head = current->next;
			}
			current->prev->next = current->next;
			current->next->prev = current->prev;
			free(current);
			list->elemNumber--;
			return 0;
		}
	}
	return -1;
}

elem* nextList(list* list, elem* elem){
	if(list == NULL || elem == NULL){
		return NULL;
	}
	else{
		return elem->next;
	}
}

void freeList(list* list){
    	elem* tmp;
	elem* curr = list->head;
	while(current != NULL){
		tmp = current;
		current = current->next;
		free(tmp);
	}
	free(list);
}

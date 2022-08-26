#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>

#include <list.h>

void freeElem(elem* elem){
	if(elem->info != NULL){
		free(elem->info);
	}
	if(elem->data != NULL){
		free(elem->data);
	}
	free(elem);
}

list* initList(){

	list* newList;
	if((newList = malloc(sizeof(list))) == NULL){		//malloc sets errno = ENOMEM when fails
		return NULL;
	}
	newList->head = NULL;
	newList->tail = NULL;
	newList->elemNumber = 0;
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
	while(tmp != NULL) {
		printf("%s\n", (char*)tmp->info);
		tmp = tmp->next;
	}

}

elem* appendList(list* list, char* name){

	if(list == NULL || name == NULL){
		return NULL;
	}
	elem* addedElem;
	if((addedElem = malloc(sizeof(elem))) == NULL){
		return NULL;
	}
	addedElem->info = malloc(strlen(name)*sizeof(char)+1);
	strncpy(addedElem->info, name, strlen(name)+1);
	addedElem->data = NULL;
	addedElem->size = 0;
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

elem* appendListCont(list* list, char* name, int nameLength, void* content, unsigned long contentSize){

	if(list == NULL || content == NULL){
		return NULL;
	}
	elem* addedElem = NULL;
	char* tmpName = NULL;
	void* data = NULL;
	if((addedElem = malloc(sizeof(elem))) == NULL){
		return NULL;
	}
	if((tmpName = malloc(nameLength*sizeof(char)+1)) == NULL){
		return NULL;
	}
	strncpy(tmpName, name, nameLength+1);
	
	if((data = malloc(contentSize)) == NULL){
		return NULL;
	}
	memset(data, 0, contentSize);
	memcpy(data, content, contentSize);
	
	addedElem->info = tmpName;
	addedElem->size = contentSize;
	addedElem->data = data;
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
		list->head->prev = NULL;
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
	
	if (list == NULL || str == NULL || list->head == NULL){
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

int removeList(list* list, void* str, void(*free)(void*)){
	if (list == NULL || str == NULL){
		errno = EINVAL;
		return -1;
	}
	elem* current = getHead(list);
	while(current != NULL){
		if(strcmp(current->info , (char*)str) != 0){
			current = current->next;
			
		}
		else{
			if(strcmp(current->info , list->head->info) == 0){
				list->head = current->next;
				if(list->head != NULL){
					list->head->prev = NULL;
				}
				list->elemNumber--;
			}
			else if(strcmp(current->info , list->tail->info) == 0){
				list->tail = current->prev;
				if(list->tail != NULL){
					list->tail->next = NULL;
				}
				list->elemNumber--;
			}else{
				current->prev->next = current->next;
				current->next->prev = current->prev;
				list->elemNumber--;		
			
			}
			if(list->elemNumber == 0){
				list->head = NULL;
				list->tail = NULL;
			
			}
			
			(*free)(current);
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

elem* prevList(list* list, elem* elem){
	if(list == NULL || elem == NULL){
		return NULL;
	}
	else{
		return elem->prev;
	}
}


int elemsNumber(list* list){
	if(list == NULL){
		return 0;
	}
	elem* current = list->head;
	int count = 0;
	while(current != NULL){
		count++;
		current = current->next;
	}
	return count;
}


void freeList(list* list, void(*freeFunc)(void*)){
	if(list != NULL){
	    	elem* tmp;
		elem* curr = list->head;
		while(curr != NULL){
			tmp = curr;
			curr = curr->next;
			(*freeFunc)(tmp);
		}
		free(list);
	}
}

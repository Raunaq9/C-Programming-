#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "stringmap.h"

// Structure that handles linked list of string map elements
typedef struct SMLinkedList {
    StringMap *data;
    struct SMLinkedList *prevNode;
    struct SMLinkedList *nextNode;
} SMLinkedList;

// Allocate, initialise and return a new, empty StringMap
StringMap *stringmap_init(void){
    StringMap *newNode;
    newNode = (StringMap*) malloc(sizeof(StringMap));
    newNode->key = NULL;
    newNode->item = NULL;
    newNode->nextNode = NULL;
    return newNode;
}

// Free all memory associated with. a StringMap.
// frees stored key strings but does not free() the (void *)item pointers
// in each StringMapItem. Does nothing if sm is NULL.
void stringmap_free(StringMap *sm) {
    StringMap *currNode = sm;
    if (sm == NULL){
        return;
    }
    while(currNode != NULL){
        free(currNode->key);
        sm = currNode->nextNode;
        free(currNode);
        currNode = sm;
    }
    sm = NULL;
}

// Search a stringmap for a given key, returning a pointer to the entry
// if found, else NULL. If not found or sm is NULL or key is NULL then 
// returns NULL.
void *stringmap_search(StringMap *sm, char *key){
    StringMap *currNode = sm;
    if (sm == NULL || key == NULL){
        return NULL;
    }
    if (currNode->key == NULL){
        return NULL;
    }
    while(currNode != NULL){
        if (strcmp(currNode->key, key) == 0){
            return currNode->item;
        }
        currNode = currNode->nextNode;
    }
    return NULL;
}

// Add an item into the stringmap, return 1 if success else 0 (e.g. an item
// with that key is already present or any one of the arguments is NULL)
// The 'key' string is copied before being stored in the stringmap.
// The item pointer is stored as-is, no attempt is made to copy its contents.
int stringmap_add(StringMap *sm, char *key, void *item){
    StringMap *currNode, *prevNode;
    StringMap *newNode;
    currNode = sm;
    if (sm == NULL || key == NULL || item == NULL){
        return 0;
    }
    if (key == NULL){
        return 0;
    }
    if (item == NULL){
        return 0;
    }
    while(currNode != NULL){
        if (currNode->key == NULL){
            break;
        }
        if (strcmp(currNode->key, key) == 0){
            return 0;
        }
        prevNode = currNode;
        currNode = currNode->nextNode;
    }
    if (currNode == NULL){
        currNode = prevNode;
    }
    if (currNode->key == NULL){
        newNode = currNode;
    } else {
        newNode = (StringMap*) malloc(sizeof(StringMapItem));
        currNode->nextNode = newNode;
    }
    newNode->key = (char*) malloc(sizeof(char) * strlen(key));
    strcpy(newNode->key, key);
    newNode->item = item;
    newNode->nextNode = NULL;
    return 1;
}

// Removes an entry from a stringmap
// free()stringMapItem and the copied key string, but not
// the item pointer.
// Returns 1 if success else 0 (e.g. item not present or any argument is NULL)
int stringmap_remove(StringMap *sm, char *key){
    StringMap *currNode;
    StringMap *prevNode;
    currNode = sm;
    prevNode = NULL;
    if (sm == NULL || key == NULL || sm->key == NULL){
        return 0;
    }
    while(currNode != NULL){
        if (strcmp(currNode->key, key) == 0){
            break;
        }
        prevNode = currNode;
        currNode = currNode->nextNode;
    }
    if (currNode != NULL) {
        if (currNode == sm) {
            StringMap *nextNode;
            if (currNode->nextNode != NULL) {// As sm contains the root node,
                                // do not delete root node.
                                // Copy contents from next node to this node.
                nextNode = currNode->nextNode;
                strcpy(currNode->key, nextNode->key);
                currNode->item = nextNode->item;
                free(nextNode->key);
                currNode->nextNode = nextNode->nextNode;
                free(nextNode);
            } else { // only one element is there in node
                    // clear node and do not remove node. Take it
                    // to initialized state
                free(currNode->key);
                currNode->key = NULL;
                //if (currNode->item != NULL){
                //      free(currNode->item);
                //}
                currNode->item = NULL;
                currNode->nextNode = NULL;
            }
        } else {
            prevNode->nextNode = currNode->nextNode;
            free(currNode->key);
            free(currNode);
        }
        return 1;
    } else {
        return 0;
    }
}

// Iterate through the stringmap - if prev is NULL then the first entry is 
// returned otherwise prev should be a value returned from a previous call to 
// stringmap_iterate() and the "next" entry will be returned.
// This operation is not thread-safe - any changes to the stringmap between 
// successive calls to stringmap_iterate may result in undefined behaviour.
// Returns NULL if no more items to examine or sm is NULL.
// There is no expectation that items are returned in a particular order (i.e.
// the order does not have to be the same order in which items were added).
StringMapItem *stringmap_iterate(StringMap *sm, StringMapItem *prev){
    StringMap *currNode = sm;
    if (sm == NULL){
        return NULL;
    }
    if (prev == NULL){
        if (sm->key != NULL){
            return sm;
        } else {
            return NULL;
        }
    }
    while(currNode != NULL){
        if (currNode == prev){
            return currNode->nextNode;
        }
        currNode = currNode->nextNode;
    }
    return NULL;
}

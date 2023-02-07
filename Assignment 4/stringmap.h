#ifndef STRINGMAP_H
#define STRINGMAP_H

typedef struct StringMap StringMap;

// data structure stored in the StringMap
typedef struct StringMap {
    char *key;
    void *item;
    StringMap *nextNode;
} StringMapItem;

// Allocate, initialise and return a new, empty StringMap
StringMap *stringmap_init(void);

// Free all memory associated with a StringMap.
// frees stored key strings but does not free() the (void *)item pointers
// in each StringMapItem. Does nothing if sm is NULL.
void stringmap_free(StringMap *sm);

// Search a stringmap for a given key, returning a pointer to the entry
// if found, else NULL. If not found or sm is NULL or key is NULL then 
// returns NULL.
void *stringmap_search(StringMap *sm, char *key);

// Add an item into the stringmap, return 1 if success else 0 (e.g. an item
// with that key is already present or any one of the arguments is NULL)
// The 'key' string is copied before being stored in the stringmap.
// The item pointer is stored as-is, no attempt is made to copy its contents.
int stringmap_add(StringMap *sm, char *key, void *item);

// Removes an entry from a stringmap
// free()stringMapItem and the copied key string, but not
// the item pointer.
// Returns 1 if success else 0 (e.g. item not present or any argument is NULL)
int stringmap_remove(StringMap *sm, char *key);

// Iterate through the stringmap - if prev is NULL then the first entry 
// is returned otherwise prev should be a value returned from a previous 
// call to stringmap_iterate()  and the "next" entry will be returned.
// This operation is not thread-safe - any changes to the stringmap 
// between successive calls to stringmap_iterate may result in undefined 
// behaviour. Returns NULL if no more items to examine or sm is NULL.
// There is no expectation that items are returned in a particular order (i.e.
// the order does not have to be the same order in which items were added).
StringMapItem *stringmap_iterate(StringMap *sm, StringMapItem *prev);

#endif

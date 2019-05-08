#include "tldmap.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

/* 	Steve Kent, fkent, CIS 415 P0
 *	This is my own work with regard to the follow remarks. 
 *	This ADT implementation is derived from Professor Sventek's stack.c 
 *	example from the class slides. The destroy, create, arraymaker, and 
 *	iterator methods follow closely to what the stack.c implementation 
 *	achieves. Additionally, I referenced the Binary Search Tree Wikipedia 
 *	page for a refresher on the basics of a BST. Specifically, I 
 *	referenced this page to refresh on Insert, Search and Traversal 
 *	(used in destroy). I realize that a more reputable source should
 *	have been used than Wikipedia but I didn't want to dig out the 
 *	textbooks.
 */


#define DEFAULT 50L

typedef struct tldnode{
	char *key;
	long value;
	struct tldnode *left, *right;
} TLDNode;

typedef struct mapdata{
	TLDNode *me;
	void **array;
	long size;
	long cap;
} MapData;


/*	NewNode is used in SuperInsert to add a new node to the BST.
 *	Allocates space for the node and the TLD key on the heap.
 * 	Returns a new node for success, NULL for failure.
 */
static TLDNode *newNode(char *k, long v){
	TLDNode *new = (TLDNode *)malloc(sizeof(TLDNode));
	if (new != NULL){
		if ((new->key = strdup(k)) == NULL){
			free(new);
			return NULL;
		}
		new->value = v;
		new->left = NULL;
		new->right = NULL;
		return new;
	}
	return NULL;
}


/*	Destroyer is used in tld_destroy to free up heap memory.
 *	This function recursively traverses the BST to free the key and the node.
 * 	Returns nothing.
 */
static void destroyer(TLDNode *node){
	if (node != NULL){
		destroyer(node->left);
		destroyer(node->right);
		free(node->key);
		free(node);
	}
}


/*	ArrayMaker is used in iterator to create and array of void pointers
 *	need by the iterator. This function follows very closely to 
 *	what Professor Sventek did with his "arrayDupl" function does
 *	in the stack.c implementation. Returns a void * array.
 */
static void **ArrayMaker(const TLDMap *tld){
	MapData *map = (MapData *)tld->self;
	void **temp = NULL;
	size_t bytes = map->size * sizeof(void *);
	temp = (void **)malloc(bytes);
	if (temp != NULL){
		int i;
		for (i = 0; i < map->size; i++){
			temp[i] = map->array[i];
		}
	}
	
	return temp;
}


/*	SuperSearch is a helper function used in tld_lookup. This is a 
 *	recursive implementation of BST search that I referenced from the
 *	BST Wikipedia page. It needed to be separated from tld_lookup 
 *	because of its recursiveness. Returns nothing.
 */
static void SuperSearch(TLDNode *node, char *theTLD, long *count, int *status){
	int result;
	if (node != NULL){
		result = strcmp(node->key, theTLD);
		if (result == 0){
			*status = 1; 
			*count = node->value;
		} else if (result < 0){
			SuperSearch(node->left, theTLD, count, status);			
		} else {
			SuperSearch(node->right, theTLD, count, status);
		}
	}
}


/*	SuperInsert is a helper function used in tld_insert. This is a 
 *	recursive implementation of BST search that I referenced from 
 *	the BST Wikipedia page. It needed to be separated from tld_insert
 *	because of its recursiveness. This function also uses some resizing 
 *	methods that Professor Sventek used in his "st_push" function from 
 * 	the stack ADT. Returns a TLDNode.
 */

static TLDNode *SuperInsert(const TLDMap *tld, TLDNode *node, char *theTLD, long v, int *status){
	MapData *map = (MapData *)tld->self;
	int result;
	int check = 1;
	if (node == NULL){
		map->size += 1L;
		if (map->cap <= map->size){
			size_t nb = 2 * map->cap * sizeof(void *);
			void **temp = (void **)realloc(map->array, nb);
			if (temp == NULL){
				check = 0;	
			} else {
				map->array = temp;
				map->cap = nb;
			}
		}
		if (check){
			*status = 1;
			node = newNode(theTLD, v);
			map->array[map->size-1] = node;	
		}		
	} else{  
		result = strcmp(node->key, theTLD);
		if (result < 0){
			node->left = SuperInsert(tld, node->left, theTLD, v, status);

		} else if (result > 0){
			node->right = SuperInsert(tld, node->right, theTLD, v, status);
		}
	}
		
	return node;
}


/*	SuperReset is a helper function used in tld_reassign. This is a 
 *	recursive implementation of BST search that I referenced from the BST 
 *	Wikipedia page. It needed to be separated from tld_reassign because
 *	of its recursiveness. Essentially search but it sets the value. 
 *	Returns nothing.
 */
static void SuperReset(TLDNode *node, char *theTLD, long v, int *status){
	int result;
	if (node != NULL){	
		result = strcmp(node->key, theTLD);
		if (result == 0){
			*status = 1;
			node->value = v;
		} else if (result < 0){
			SuperReset(node->left, theTLD, v, status);
		
		} else {
			SuperReset(node->right, theTLD, v, status);
		}
	}
}


/*
 * destroy() destroys the list structure in `tld'
 *
 * all heap allocated storage associated with the map is returned to the heap
 */
static void tld_destroy(const TLDMap *tld){
	if (tld != NULL){
		MapData *map = (MapData *)tld->self;
		if (map != NULL){
			destroyer(map->me);
			free(map->array);
			free(map);
		}	
		free((void *)tld);
	}
};


/*
 * insert() inserts the key-value pair (theTLD, v) into the map;
 * returns 1 if the pair was added, returns 0 if there already exists en
 * entry corresponding to TLD
 */
static int tld_insert(const TLDMap *tld, char *theTLD, long v){
	MapData *map = (MapData *)tld->self;
	int status = 0;
	map->me = SuperInsert(tld, map->me, theTLD, v, &status);		
	return status;
};


/*
 * reassign() replaces the current value corresponding to theTLD in the
 * map with v.
 * returns 1 if the value was replaced, returns 0 if no such key exists
 */
static int tld_reassign(const TLDMap *tld, char *theTLD, long v){
	MapData *map = (MapData *)tld->self;
	int status = 0;
	SuperReset(map->me, theTLD, v, &status);
	return status;
};


/*
 * lookup() returns the current value associated with theTLD in *v
 * returns 1 if the lookup was successful, returns 0 if no such key exists
 */
static int tld_lookup(const TLDMap *tld, char *theTLD, long *v){
	MapData *map = (MapData *)tld->self;
	int status = 0;
	long count = 0L;
	SuperSearch(map->me, theTLD, &count, &status);
	*v = count;
	return status;
};


/*
 * itCreate() creates an iterator over the map
 * returns the iterator if successful, NULL if unsuccessful
 * NOTE: I implemented this based off the stack.c iterator create method. 
 * Very similar to Professor Sventek's work.
 */
static const Iterator* tld_itCreate(const TLDMap *tld){
	MapData *map = (MapData *)tld->self;
	const Iterator *it = NULL;
	void **temp = ArrayMaker(tld);
	if (temp != NULL){
		it = Iterator_create(map->size, temp);
		if (it == NULL){	
			free(temp);
		}
	}
	return it; 
};


/*
 * TLDNode_tldname returns the tld associated with the TLDNode
 */
char *TLDNode_tldname(TLDNode *node){
	return node->key;
}


/*
 * TLDNode_count returns the value currently associated with that TLD
 */
long TLDNode_count(TLDNode *node){	
	return node->value;
}


static TLDMap template = {NULL, tld_destroy, tld_insert, tld_reassign, tld_lookup, tld_itCreate};


/*
 * TLDMap_create generates a map structure for storing counts against
 * top level domains (TLDs)
 *
 * creates an empty TLDMap
 * returns a pointer to the list if successful, NULL if not
 * NOTE: Again, this follows Professor Sventek's stack.c ADT implementation.
 */
const TLDMap *TLDMap_create(void){
	TLDMap *tld = (TLDMap *)malloc(sizeof(TLDMap));
	if (tld != NULL){
		MapData *map = (MapData *)malloc(sizeof(MapData));
		if (map != NULL){
			long capacity = DEFAULT; 
			void **a= NULL;
			a = (void **)malloc(capacity * sizeof(void *));
			if (a != NULL){
				map->cap = capacity;
				map->me = NULL;
				map->size = 0L;
				map->array = a;
				*tld = template;
				tld->self = map;
			} else {
				free(map);
				free(tld);
				tld = NULL;
			}	
		} else {
			free(tld);
			tld = NULL;
		}	
	}
	return tld;
}	

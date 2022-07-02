/*A linked list that contains all viruses/bloom filters.*/

#ifndef FILTER_LIST_H
#define FILTER_LIST_H

#include "bloom_filter.h"

typedef struct filter_list* Filter_list;
typedef struct fnode* Fnode;

//Create one list to contain pointers to all bloom filters
void create_filter_list(int bloom_size);

//Search and return the specified by 'virus_name' bloom filter.
Bloom_filter get_bloom_filter(char* virus_name);

//Insert virus name to filter list.
Bloom_filter insert_filter_list(char* virus_name);

//Free filter list and all bloom filters.
void delete_filter_list();


#endif
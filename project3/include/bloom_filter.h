#ifndef BLOOM_FILTER_H
#define BLOOM_FILTER_H

#include "hash_functions.h"

typedef struct bloom_filter* Bloom_filter;

//Creates the bloom filter and initializes all bits to 0.
Bloom_filter create_bloom_filter(int bloom_size,char* virus_name);

//Given an item sets the apropriate bit of bloom filter to 1.
void set_bloom_filter(Bloom_filter bloom_filter,char* str_id);

//Returns 1 if item exists and 0 if it doesn't.
int check_bloom_filter(Bloom_filter bloom_filter,char* str_id);

//Get array of bloom filter.
char* get_bloom_array(Bloom_filter bloom_filter);

//Merge bloom filter's array with the given array - bitwise OR.
void merge_bloom_arrays(Bloom_filter* bloom_filter, char* array);

//Free bloom filter.
void delete_bloom_filter(Bloom_filter bloom_filter);


#endif
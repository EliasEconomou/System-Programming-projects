/*A hash table using chain hashing that contains citizen records*/

#ifndef HASH_TABLE_H
#define HASH_TABLE_H

#include "citizen_record.h"
#include "hash_functions.h"

typedef struct hash_table* Hash_table;
typedef struct bucket* Bucket;
typedef struct htnode* Htnode;

//Create hash table with the given number of buckets.
void create_hash_table(int bucket_number);

//Insert record to hash table.
int insert_hash_table(Citizen_record rec);

//Search for a record and return a pointer to it.
Citizen_record search_hash_table(char* id);

//Print all ids (debug).
void print_hash_table();

//Free hash table and every bucket, node and record is stored in it.
void delete_hash_table();

#endif
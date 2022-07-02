/*A virus hash table that has info about aprroved and denied request to travel to countries*/

#ifndef VIRUS_TABLE_H
#define VIRUS_TABLE_H

#include "stat_record.h"
#include "hash_functions.h"

typedef struct virus_table* Virus_table;
typedef struct vtbucket* VTBucket;
typedef struct vtnode* VTnode;

//Create virus hash table with the given number of buckets.
Virus_table create_virus_table(char* virus_name,int bucket_number);

//Insert record to hash table.
void insert_virus_table(Virus_table virus_table,Stat_record rec);

// //Search for a record and return a pointer to it.
// Stat_record search_virus_table(Virus_table virus_table,char* country_name);

//Print all countries.
void print_virus_table(Virus_table virus_table);

//Free virus hash table and every bucket, node and record is stored in it.
void delete_virus_table(Virus_table virus_table);

//Given virus table pointer, return virus name.
char* get_virus_name(Virus_table virus_table);

//Given virus table and dates, counts records.
void count_stats(Virus_table virus_table, char* date1, char* date2);

//Given virus table, dates and country, counts records.
void count_stats_country(Virus_table virus_table, char* date1, char* date2, char* country_name);

#endif
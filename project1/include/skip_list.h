/*A skip list that contains information about citizens based on a specific virus and wheather
citizens are vaccinated or not.*/

#ifndef SKIP_LIST_H
#define SKIP_LIST_H

#include "hash_table.h"

typedef struct skip_list* Skip_list;
typedef struct slnode* Slnode;

//Get the virus name for this skip list.
char* get_virus(Skip_list skip_list);

//Get whether this skip list if for vaccinated or not.
char* get_vacc(Skip_list skip_list);

//Create an empty skip list and return a pointer to it.
Skip_list create_skip_list(int max_level, char* virus_name, char* vacc_status);

//Insert item to skip list.
void insert_skip_list(Skip_list skip_list, char* key, char* date, char* country);

//Search for a key and return 1 if found - 0 if not.
int search_skip_list(Skip_list skip_list, char* id);

//Print all records of skip list.
void print_skip_list(Skip_list skip_list);

//Iterate every node of skip list and count the citizens that are/aren't vaccinated for
//the specific virus in the exact dates. (cvd=country-virus-dates)
int cvd_population_status(Skip_list skip_list, char* country_name, int date1, int date2);

//Iterate every node of skip list and count the citizens that are/aren't vaccinated for
//the specific virus. (cv=country-virus)
int cv_population_status(Skip_list skip_list, char* country_name);

//Iterate every node of skip list and count the citizens that are/aren't vaccinated for
//the specific country and virus in the exact dates for every age group. (cvd=country-virus-dates)
void cvd_pop_status_by_age(Skip_list skip_yes, Skip_list skip_no, char* country_name, int date1, int date2);

//Iterate every node of skip list and count the citizens that are/aren't vaccinated for
//the specific country and virus for every age group. (cv=country-virus)
void cv_pop_status_by_age(Skip_list skip_yes, Skip_list skip_no, char* country_name);

//Remove an item from skip list.
void remove_skip_list(Skip_list skip_list, char* key);

//Get the date a given citizen got vaccinated. If no date return NULL.
char* get_date_skip_list(Skip_list skip_list, char* key);

//Free skip list.
void delete_skip_list(Skip_list skip_list);


#endif
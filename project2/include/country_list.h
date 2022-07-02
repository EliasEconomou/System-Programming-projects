/*A linked list that contains all countries*/

#ifndef COUNTRY_LIST_H
#define COUNTRY_LIST_H

#include "skip_list.h"
#include "virus_list.h"

typedef struct country_list* Country_list;
typedef struct cnode* Cnode;

//Create country list.
void create_country_list();

//Search for a country and return its node.
Cnode search_country_list(char* country_name);

//Insert country name to country list and return a pointer to its node.
Cnode insert_country_list(char* country_name,Skip_list skip_list);

//Get number of countries.
int get_countries_number();

//Give a country node and return its country name.
char* get_country_name(Cnode node);

//Print countries.
void print_country_list();

//Iterate every node of country list, find any skip lists for the ginen virus and 
//count the citizens that are/aren't vaccinated for it. (v=virus)
void v_population_status(char* virus_name);

//Iterate every node of country list, find any skip lists for the ginen virus and 
//count the citizens that are/aren't vaccinated for it in the given dates. (vd=virus-dates)
void vd_population_status(char* virus_name, int date1, int date2);

//Iterate every node of country list, find any skip lists for the ginen virus and 
//count the citizens that are/aren't vaccinated for it for every age group. (v=virus)
void v_pop_status_by_age(char* virus_name);

//Iterate every node of country list, find any skip lists for the ginen virus and 
//count the citizens that are/aren't vaccinated for it in the given dates for every age group. (vd=virus-dates)
void vd_pop_status_by_age(char* virus_name, int date1, int date2);

//Free country list.
void delete_country_list();


#endif
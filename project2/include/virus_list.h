/*A linked list that contains viruses/skip lists depending on whether they are vaccinated.
This means two of these lists will be created one for all viruses that describe vaccinated
citizens and one for not vaccinated.*/

#ifndef VIRUS_LIST_H
#define VIRUS_LIST_H

#include "skip_list.h"

typedef struct virus_list* Virus_list;
typedef struct vnode* Vnode;

//Create two lists: one for all viruses with 'vaccinated status' = YES and one for those with 'NO'.
void create_virus_list(int levels);

//Return a pointer to the list specified by 'vaccinated status'.
//If vaccinated status other than 'YES' or 'NO' return error.
Virus_list get_virus_list(char* vacc_status);

//Search and return the specified by 'virus_name' skip list.
Skip_list get_skip_list(Virus_list virus_list,char* virus_name);

//Insert virus name to virus list specified by 'vaccinated status'.
Skip_list insert_virus_list(char* virus_name, char* vacc_status);

//Print all viruses of a virus list (debug).
void print_virus_list(char* vacc_status);

//Check given virus for a specific id and return 1 if found, 0 otherwise.
int findid_virus_list(char* virus_name,char* key);

//Check all skip lists for given id and print whether vaccinated or not.
void printid_virus_list(char* key);

//Get the number of yes or no skip lists or the total number (yes and no skip lists).
//Status can be "YES" / "NO" / "TOTAL".
int get_skip_lists_number(char* vacc_status);

//Get the names of the viruses in virus list specified by status "yes"/"no".
char** get_skip_names(char* vacc_status);

//Return a table of pointers to skip lists that contain the given key, while keeping track of their number.
Skip_list* skip_lists_key(int *count,char* key);

//Free virus lists and all skip lists (viruses).
void delete_virus_list();


#endif
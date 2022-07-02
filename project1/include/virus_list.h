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

//Print all viruses of a virus list (dubug).
void print_virus_list(char* vacc_status);

//Check given virus-skip list for a specific id and return 1 if found, 0 otherwise.
int findid_virus_list(char* virus_name,char* key);

//Check all skip lists for given id and print whether vaccinated or not.
void printid_virus_list(char* key);

//Get the total number of all skip lists (yes and no skip lists)
int get_skip_lists_number();

//Free virus lists and all skip lists (viruses).
void delete_virus_list();


#endif
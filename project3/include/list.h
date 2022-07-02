/*A linked list*/

#ifndef LIST_H
#define LIST_H


typedef struct list* List;
typedef struct listnode* Listnode;

//Create country list.
List create_list();

//Search for a key and return it's value.
int get_value_list(List list, char* key);

//Insert key and value to list.
void insert_list(List list, char* key, char* value);

//Update key with the new value.
void update_list(List list, char* key, char* value);

//Print list.
void print_list(List list);

//Free list.
void delete_list(List list);


#endif
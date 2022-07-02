#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "virus_list.h"
#include "various_functions.h"

struct virus_list
{
    Vnode vnode; //will point to the last node inserted - beginning / head
    char* vacc_status; //vaccinated status (YES/NO)
    int size; //number of nodes/viruses
    int levels; //number of levels a skip list can have
};

struct vnode
{
    char* virus_name;
    Skip_list skip_list;
    Vnode next;
};

Virus_list v_yes,v_no;

//Create two lists: one for all viruses with 'vaccinated status' = YES and one for those with 'NO'.
void create_virus_list(int levels)
{
    v_yes = malloc(sizeof(struct virus_list));
    if (v_yes==NULL)
    {
        perror("Error in malloc");
        exit(1);
    }
    v_no = malloc(sizeof(struct virus_list));
    if (v_no==NULL)
    {
        perror("Error in malloc");
        exit(1);
    }
    v_yes->vnode = malloc(sizeof(struct vnode));
    if (v_yes->vnode==NULL)
    {
        perror("Error in malloc");
        exit(1);
    }
    v_no->vnode = malloc(sizeof(struct vnode));
    if (v_no->vnode==NULL)
    {
        perror("Error in malloc");
        exit(1);
    }
    v_yes->vnode->next=NULL;
    v_no->vnode->next=NULL;
    v_yes->vacc_status = malloc(strlen("YES")+1);
    if (v_yes->vacc_status==NULL)
    {
        perror("Error in malloc");
        exit(1);
    }
    v_no->vacc_status = malloc(strlen("NO")+1);
    if (v_no->vacc_status==NULL)
    {
        perror("Error in malloc");
        exit(1);
    }
    strcpy(v_yes->vacc_status,"YES");
    strcpy(v_no->vacc_status,"NO");
    v_yes->levels=levels;
    v_no->levels=levels;
    v_yes->size=0;
    v_no->size=0;
}


//Return a pointer to the list specified by 'vaccinated status'.
//If vaccinated status other than 'YES' or 'NO' return error.
Virus_list get_virus_list(char* vacc_status)
{ 
    if (strcmp(vacc_status,"YES")==0)
    {
        return v_yes;
    }
    else if (strcmp(vacc_status,"NO")==0)
    {
        return v_no;
    }
    else
    {
        return NULL;
    }
    
}


//Search and return the specified by 'virus_name' skip list.
Skip_list get_skip_list(Virus_list virus_list,char* virus_name)
{
    if (virus_name==NULL)
    {
        return NULL;
    }
    
    Vnode node = virus_list->vnode;
    if (node==NULL)
    {
        return NULL;
    }
    else
    {
        while (node->next!=NULL)
        {
            if (strcmp(node->virus_name,virus_name)==0)
            {
                return node->skip_list;
            }
            node=node->next;
        }
        return NULL;
    }
}


//Insert virus name to virus list specified by 'vaccinated status'. If no skip list with that name,
//create one.
Skip_list insert_virus_list(char* virus_name, char* vacc_status)
{
    Virus_list vlist = get_virus_list(vacc_status);
    Skip_list virus = get_skip_list(vlist,virus_name);
    if (virus!=NULL)
    {
        return virus;
    }
    Vnode new_node = malloc(sizeof(struct vnode));
    if (new_node==NULL)
    {
        perror("Error in malloc");
        exit(1);
    }
    new_node->virus_name = malloc(strlen(virus_name)+1);
    if (new_node->virus_name==NULL)
    {
        perror("Error in malloc");
        exit(1);
    }
    strcpy(new_node->virus_name,virus_name);

    new_node->next = vlist->vnode;
    vlist->vnode = new_node;
    vlist->size++;
    vlist->vnode->skip_list = create_skip_list(get_levels(vlist->levels), virus_name, vacc_status);
    return vlist->vnode->skip_list;
}


//Print all viruses of a virus list (debug).
void print_virus_list(char* vacc_status)
{
    Virus_list vlist = get_virus_list(vacc_status);
    
    if (vlist->vnode!=NULL)
    {
        Vnode node = vlist->vnode;
        while (node->next!=NULL)
        {
            printf(" < %s > ",node->virus_name);
            node=node->next;
        }
    }
    printf("\n");
}


//Check given virus for a specific id and return 1 if found, 0 otherwise.
int findid_virus_list(char* virus_name,char* key)
{
    //search key in 'v_yes'
    Skip_list virus = get_skip_list(v_yes,virus_name);
    int key_found = search_skip_list(virus,key);
    if (key_found==1)
    {
        return key_found;
    }
    //search key in 'v_no'
    virus = get_skip_list(v_no,virus_name);
    key_found = search_skip_list(virus,key);
    if (key_found==1)
    {
        return key_found;
    }
    //not found
    return 0;
}


//Check all skip lists for given id and print whether vaccinated or not. Used in /vaccineStatus.
void printid_virus_list(char* key)
{
    if (v_yes->vnode!=NULL)
    {
        Vnode node = v_yes->vnode;
        while (node->next!=NULL)
        {
            Skip_list virus = get_skip_list(v_yes,node->virus_name);
            int found = search_skip_list(virus,key);
            if (found==1)
            {
                printf("%s %s %s\n",node->virus_name,v_yes->vacc_status,get_date_skip_list(virus,key));
            }
            node=node->next;
        }
    }
    if (v_no->vnode!=NULL)
    {
        Vnode node = v_no->vnode;
        while (node->next!=NULL)
        {
            Skip_list virus = get_skip_list(v_no,node->virus_name);
            int found = search_skip_list(virus,key);
            if (found==1)
            {
                printf("%s %s\n",node->virus_name,v_no->vacc_status);
            }
            node=node->next;
        }
    }
}


//Get the number of yes or no skip lists or the total number (yes and no skip lists).
//Status can be "YES" / "NO" / "TOTAL".
int get_skip_lists_number(char* vacc_status)
{
    if (strcmp(vacc_status,"YES")==0)
    {
        return (v_yes->size);
    }
    else if (strcmp(vacc_status,"NO")==0)
    {
        return (v_no->size);
    }
    else if (strcmp(vacc_status,"TOTAL")==0)
    {
        return (v_yes->size+v_no->size);
    }
    else
        return -1;
}


//Get the names of the viruses in virus list specified by status "YES"/"NO".
char** get_skip_names(char* vacc_status)
{
    Virus_list vlist = get_virus_list(vacc_status);
    printf("here1 size = %d\n",vlist->size);
    char** virus_names = malloc(vlist->size * sizeof(char*));
    Vnode node = vlist->vnode;
    if (vlist->vnode!=NULL)
    {
        int i=0;
        Vnode node = vlist->vnode;
        while (node->next!=NULL)
        {
            virus_names[i] = malloc(sizeof(char)*strlen(node->virus_name) + 1);
            strcpy(virus_names[i],node->virus_name);
            node=node->next;
            i++;
        }
    }
    return virus_names;

}


//Return a table of pointers to skip lists that contain the given key, while keeping track of their number.
Skip_list* skip_lists_key(int *count,char* key)
{
    Skip_list* table_of_skip_lists = malloc((*count+2)*sizeof(Skip_list));
    table_of_skip_lists[*count] = NULL;
    table_of_skip_lists[*count+1] = NULL;
    
    //We will loop both virus lists and store skip lists that have the key we're looking for.
    Vnode node = v_yes->vnode;
    if (v_yes->vnode!=NULL)
    {
        Vnode node = v_yes->vnode;
        while (node->next!=NULL)
        {
            int check = search_skip_list(node->skip_list,key);
            if (check == 1) //store this skip list because it has our key
            {
                table_of_skip_lists[*count] = node->skip_list;
                (*count)++;
                table_of_skip_lists = realloc(table_of_skip_lists,(*count+2)*sizeof(Skip_list));
                table_of_skip_lists[*count+1] = NULL; 
            }
            node=node->next;
        }
    }
    node = v_no->vnode;
    if (v_no->vnode!=NULL)
    {
        Vnode node = v_no->vnode;
        while (node->next!=NULL)
        {
            int check = search_skip_list(node->skip_list,key);
            if (check == 1) //store this skip list because it has our key
            {
                table_of_skip_lists[*count] = node->skip_list;
                (*count)++;
                table_of_skip_lists = realloc(table_of_skip_lists,(*count+2)*sizeof(Skip_list));
                table_of_skip_lists[*count+1] = NULL; 
            }
            node=node->next;
        }
    }
    return table_of_skip_lists;
}


//Free virus lists and all skip lists (viruses).
void delete_virus_list()
{
    
    Vnode node = v_yes->vnode;
    while (node->next!=NULL)
    {
        Vnode temp = node;
        node=node->next;
        free(temp->virus_name);
        delete_skip_list(temp->skip_list);
        free(temp);
    }
    free(node);
    free(v_yes->vacc_status);
    free(v_yes);


    node = v_no->vnode;
    while (node->next!=NULL)
    {
        Vnode temp = node;
        node=node->next;
        free(temp->virus_name);
        delete_skip_list(temp->skip_list);
        free(temp);
    }
    free(node);
    free(v_no->vacc_status);
    free(v_no);
    
}


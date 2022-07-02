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


//Check given virus-skip list for a specific id and return 1 if found, 0 otherwise.
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


//Get the total number of all skip lists (yes and no skip lists)
int get_skip_lists_number()
{
    return (v_yes->size+v_no->size);
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


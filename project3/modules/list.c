#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "list.h"

struct list
{
    Listnode list_node;
    int size;
};

struct listnode
{
    char* key;
    char* value;
    Listnode next;
};


//Create country list.
List create_list()
{
    List list = malloc(sizeof(struct list));
    if (list==NULL)
    {
        perror("Error in malloc");
        exit(1);
    }
    list->list_node=NULL;
    list->size=0;
    return list;
}


//Search for a key and return it's value.
int get_value_list(List list, char* key)
{
    Listnode node = list->list_node;
    if (node==NULL)
    {
        return -1;
    }
    else
    {
        
        if (strcmp(node->key,key)==0)
        {
            int value = atoi(node->value);
            return value;
        }
        
        while (node->next!=NULL)
        {
            node=node->next;
            if (strcmp(node->key,key)==0)
            {
                int value = atoi(node->value);
                return value;
            }
        }   
        return -1;
    }
}


//Search for a key and return the list node.
Listnode search_list(List list,char* key)
{
    Listnode node = list->list_node;
    if (node==NULL)
    {
        return NULL;
    }
    else
    {
        
        if (strcmp(node->key,key)==0)
        {
            return node;
        }
        
        while (node->next!=NULL)
        {
            node=node->next;
            if (strcmp(node->key,key)==0)
            {
                return node;
            }
        }   
        return NULL;
    }
}


//Insert key and value to list.
void insert_list(List list, char* key, char* value)
{
    Listnode search_node = search_list(list,key);
    if (search_node!=NULL) //item found
    {
        return;
    }
    
    Listnode new_node = malloc(sizeof(struct listnode));
    if (new_node==NULL)
    {
        perror("Error in malloc");
        exit(1);
    }
    new_node->key = malloc(strlen(key)+1);
    if (new_node->key==NULL)
    {
        perror("Error in malloc");
        exit(1);
    }
    new_node->value = malloc(strlen(value)+1);
    if (new_node->value==NULL)
    {
        perror("Error in malloc");
        exit(1);
    }
    strcpy(new_node->key,key);
    strcpy(new_node->value,value);

    new_node->next = list->list_node;
    list->list_node = new_node; 
    return;
}


//Update key with the new value.
void update_list(List list, char* key, char* value)
{
    Listnode search_node = search_list(list,key);
    if (search_node==NULL) //item not found
    {
        return;
    }
    free(search_node->value);
    search_node->value = malloc(strlen(value)+1);
    if (search_node->value==NULL)
    {
        perror("Error in malloc");
        exit(1);
    }
    strcpy(search_node->value,value);
    return;
}


//Print list.
void print_list(List list)
{
    if (list->list_node==NULL)
    {
        printf("Empty list.\n");
        return;
    }
    else
    {
        Listnode node = list->list_node;
        printf(" < %s - %s > \n",node->key,node->value);
        printf("\n");
        
        while (node->next!=NULL)
        {
            
            node=node->next;
            printf(" < %s - %s > \n",node->key,node->value);
            printf("\n");   
        }
        
    }
    printf("\n");
}


//Free list.
void delete_list(List list)
{
    Listnode node = list->list_node;
    while (node->next!=NULL)
    {
        Listnode temp = node;
        node=node->next;
        free(temp->key);
        free(temp->value);
        free(temp);
    }
    free(node->key);
    free(node->value);
    free(node);
    free(list);
}
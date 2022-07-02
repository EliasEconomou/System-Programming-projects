#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "filter_list.h"

struct filter_list
{
    Fnode fnode; //will point to the last node inserted - beginning / head
    int bloom_size;
    int size; //number of nodes/viruses
};

struct fnode
{
    char* virus_name;
    Bloom_filter bloom_filter;
    Fnode next;
};

Filter_list bf_list;

//Create one list to contain pointers to all bloom filters
void create_filter_list(int bloom_size)
{
    bf_list = malloc(sizeof(struct filter_list));
    if (bf_list==NULL)
    {
        perror("Error in malloc");
        exit(1);
    }
    bf_list->fnode = malloc(sizeof(struct fnode));
    if (bf_list->fnode==NULL)
    {
        perror("Error in malloc");
        exit(1);
    }
    bf_list->bloom_size=bloom_size;
    bf_list->fnode->next=NULL;
}


//Search and return the specified by 'virus_name' bloom filter.
Bloom_filter get_bloom_filter(char* virus_name)
{
    Fnode node = bf_list->fnode;
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
                return node->bloom_filter;
            }
            node=node->next;
        }
        return NULL;
    }
}


//Insert virus name to filter list specified by 'vaccinated status'.
Bloom_filter insert_filter_list(char* virus_name)
{
    //get me the right bloom filter
    Bloom_filter virus = get_bloom_filter(virus_name);
    if (virus!=NULL)
    {
        return virus;
    }
    Fnode new_node = malloc(sizeof(struct fnode));
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

    new_node->next = bf_list->fnode;
    bf_list->fnode = new_node;
    bf_list->size++;
    bf_list->fnode->bloom_filter = create_bloom_filter(bf_list->bloom_size, virus_name);
    return bf_list->fnode->bloom_filter;
}


//Free filter list and all bloom filters.
void delete_filter_list()
{
    Fnode node = bf_list->fnode;
    while (node->next!=NULL)
    {
        Fnode temp = node;
        node=node->next;
        free(temp->virus_name);
        delete_bloom_filter(temp->bloom_filter);
        free(temp);
    }
    free(node);
    free(bf_list);

}

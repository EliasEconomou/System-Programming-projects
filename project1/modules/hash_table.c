#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "hash_table.h"

//Hash table has buckets and every bucket has nodes.
struct hash_table
{
    int total_records; //total records of hash table
    int buckets_num;
    Bucket buckets; // 'buckets_num' buckets will be allocated at the creation of hash table
};

struct bucket
{
    Htnode ht_node; //will point to the last node inserted - beginning of the bucket / head
};

struct htnode
{
    Citizen_record record;
    Htnode next; //will be used if overflown occurs
};

Hash_table table_of_records;

//Create hash table with the given number of buckets and return a pointer to it.
void create_hash_table(int bucket_number)
{
    table_of_records = malloc(sizeof(struct hash_table));
    if (table_of_records==NULL)
    {
        perror("Error in malloc");
        exit(1);
    }
    table_of_records->buckets = malloc(bucket_number*sizeof(struct bucket));
    if (table_of_records->buckets==NULL)
    {
        perror("Error in malloc");
        exit(1);
    }
    table_of_records->total_records=0;
    table_of_records->buckets_num=bucket_number;
    for (int i = 0; i < bucket_number; i++)
    {
        table_of_records->buckets[i].ht_node=NULL;
    }
}


//Insert record to hash table.
int insert_hash_table(Citizen_record rec)
{
    //search for a possible duplicate
    Citizen_record rec2 = search_hash_table(rec->citizenID);
    if(rec2!=NULL) //if duplicate id
    {
        //if other attributes are different - ERROR - return and free
        if((strcmp(rec->firstName,rec2->firstName)!=0)||(strcmp(rec->lastName,rec2->lastName)!=0)||(rec->age!=rec2->age))
        {
            return 1;
        }
        //if other attributes are same - VALID - return but don't free directly
        else if((strcmp(rec->firstName,rec2->firstName)==0)&&(strcmp(rec->lastName,rec2->lastName)==0)&&(rec->age==rec2->age))
        {
            return 2;
        }
    }
    int hash = (sdbm((unsigned char*)rec->citizenID)%table_of_records->buckets_num);

    Htnode new_node = malloc(sizeof(struct htnode));
    if (new_node==NULL)
    {
        perror("Error in malloc");
        exit(1);
    }	
    new_node->record = rec;

    new_node->next = table_of_records->buckets[hash].ht_node;
    table_of_records->buckets[hash].ht_node = new_node;
    table_of_records->total_records++;
    return 0;
    
}


//Search for a record and return a pointer to it.
Citizen_record search_hash_table(char* id)
{
    int hash = (sdbm((unsigned char*)id)%table_of_records->buckets_num);
    Htnode node = table_of_records->buckets[hash].ht_node;
    if (node==NULL)
    {
        return NULL;
    }
    else
    {
        if (strcmp(node->record->citizenID,id)==0)
        {
            return node->record;
        }
        
        while (node->next!=NULL)
        {
            node=node->next;
            if (strcmp(node->record->citizenID,id)==0)
            {
                return node->record;
            }
            
        }
        return NULL;
    }
}


//Print all ids.
void print_hash_table()
{
    int bucket_number = table_of_records->buckets_num;
    for (int i = 0; i < bucket_number; i++)
    {
        printf("%d : ",i);
        if (table_of_records->buckets[i].ht_node!=NULL)
        {
            Htnode node = table_of_records->buckets[i].ht_node;
            while (node->next!=NULL)
            {
                printf(" < %s > ",node->record->citizenID);
                node=node->next;
            }   
            printf(" < %s > ",node->record->citizenID);
        }  
        printf("\n");
    }
}


//Free hash table and every bucket, node and record is stored in it.
void delete_hash_table()
{
    int bucket_number = table_of_records->buckets_num;
    for (int i = 0; i < bucket_number; i++)
    {
        if (table_of_records->buckets[i].ht_node!=NULL)
        {
            Htnode node = table_of_records->buckets[i].ht_node;
            while (node!=NULL)
            {
                Htnode temp = node;
                free(temp->record->citizenID);
                free(temp->record->firstName);
                free(temp->record->lastName);
                free(temp->record);
                node=node->next;
                free(temp);
            }  
        }  
    }
    free(table_of_records->buckets);
    free(table_of_records);
}
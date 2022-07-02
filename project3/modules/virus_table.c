#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "virus_table.h"
#include "various_functions.h"

//Virus table has buckets and every bucket has nodes.
struct virus_table
{
    char* virus_name;
    int total_requests; //not actually need it because dates
    // int approved_requests; //total approved requests for virus // todo delete maybe
    // int denied_requests; //total denied requests for virus
    int buckets_num;
    VTBucket buckets; // 'buckets_num' buckets will be allocated at the creation of hash table
};

struct vtbucket
{
    VTnode vt_node; //will point to the last node inserted - beginning of the bucket / head
};

struct vtnode
{
    Stat_record record;
    VTnode next; //will be used if overflown occurs
};


//Create virus table with the given number of buckets and return a pointer to it.
Virus_table create_virus_table(char* virus_name,int bucket_number)
{
    Virus_table virus_table = malloc(sizeof(struct virus_table));
    if (virus_table==NULL)
    {
        perror("Error in malloc");
        exit(1);
    }
    virus_table->buckets = malloc(bucket_number*sizeof(struct vtbucket));
    if (virus_table->buckets==NULL)
    {
        perror("Error in malloc");
        exit(1);
    }
    virus_table->virus_name = malloc(strlen(virus_name)+1);
    strcpy(virus_table->virus_name,virus_name);
    virus_table->buckets_num=bucket_number;
    for (int i = 0; i < bucket_number; i++)
    {
        virus_table->buckets[i].vt_node=NULL;
    }
    return virus_table;
}


//Insert record to hash table.
void insert_virus_table(Virus_table virus_table,Stat_record rec)
{
    int hash = (sdbm((unsigned char*)rec->country_name)%virus_table->buckets_num);

    VTnode new_node = malloc(sizeof(struct vtnode));
    if (new_node==NULL)
    {
        perror("Error in malloc");
        exit(1);
    }
    new_node->record = rec;

    new_node->next = virus_table->buckets[hash].vt_node;
    virus_table->buckets[hash].vt_node = new_node;
    virus_table->total_requests++;
    return; 
}


//Print all countries.
void print_virus_table(Virus_table virus_table)
{
    int bucket_number = virus_table->buckets_num;
    for (int i = 0; i < bucket_number; i++)
    {
        printf("%d : ",i);
        if (virus_table->buckets[i].vt_node!=NULL)
        {
            VTnode node = virus_table->buckets[i].vt_node;
            while (node->next!=NULL)
            {
                printf(" < %s > ",node->record->country_name);
                node=node->next;
            }   
            printf(" < %s > ",node->record->country_name);
        }  
        printf("\n");
    }
}


//Free virus hash table and every bucket, node and record is stored in it.
void delete_virus_table(Virus_table virus_table)
{
    int bucket_number = virus_table->buckets_num;
    for (int i = 0; i < bucket_number; i++)
    {
        if (virus_table->buckets[i].vt_node!=NULL)
        {
            VTnode node = virus_table->buckets[i].vt_node;
            while (node!=NULL)
            {
                VTnode temp = node;
                free(temp->record->country_name);
                free(temp->record->date);
                free(temp->record);
                node=node->next;
                free(temp);
            }  
        }  
    }
    free(virus_table->virus_name);
    free(virus_table->buckets);
    free(virus_table);
}


//Given virus table pointer, return virus name.
char* get_virus_name(Virus_table virus_table)
{
    return virus_table->virus_name;
}


//Given virus table and dates, counts records.
void count_stats(Virus_table virus_table, char* date1, char* date2)
{
    int accepted_counter=0;
    int rejected_counter=0;
    int date1_int = check_reformat_date(date1);
    int date2_int = check_reformat_date(date2);
    int bucket_number = virus_table->buckets_num;
    for (int i = 0; i < bucket_number; i++)
    {
        if (virus_table->buckets[i].vt_node!=NULL)
        {
            VTnode node = virus_table->buckets[i].vt_node;
            while (node->next!=NULL)
            {
                int date_int = check_reformat_date(node->record->date);
                if ((date_int>=date1_int)&&(date_int<=date2_int))
                {
                    if (node->record->approved=='y')
                    {
                        accepted_counter++;
                    }
                    else if (node->record->approved=='n')
                    {
                        rejected_counter++;
                    }
                }
                node=node->next;
            }
            int date_int = check_reformat_date(node->record->date);
            if ((date_int>=date1_int)&&(date_int<=date2_int))
            {
                if (node->record->approved=='y')
                {
                    accepted_counter++;
                }
                else if (node->record->approved=='n')
                {
                    rejected_counter++;
                }
            }
        }  
    }
    printf("Travel Statistics:\n"
            "\tTOTAL REQUESTS %d\n"
            "\tACCEPTED %d\n"
            "\tREJECTED %d\n",accepted_counter+rejected_counter,accepted_counter,rejected_counter);
}


//Given virus table, dates and country, counts records.
void count_stats_country(Virus_table virus_table, char* date1, char* date2, char* country_name)
{
    int accepted_counter=0;
    int rejected_counter=0;
    int date1_int = check_reformat_date(date1);
    int date2_int = check_reformat_date(date2);
    int hash = (sdbm((unsigned char*)country_name)%virus_table->buckets_num);
    if (virus_table->buckets[hash].vt_node!=NULL)
    {
        VTnode node = virus_table->buckets[hash].vt_node;
        while (node->next!=NULL)
        {
            if (strcmp(node->record->country_name,country_name)==0)
            {
                int date_int = check_reformat_date(node->record->date);
                if ((date_int>=date1_int)&&(date_int<=date2_int))
                {
                    if (node->record->approved=='y')
                    {
                        accepted_counter++;
                    }
                    else if (node->record->approved=='n')
                    {
                        rejected_counter++;
                    }
                }
            }
            node=node->next;
        }
        if (strcmp(node->record->country_name,country_name)==0)
        {
            int date_int = check_reformat_date(node->record->date);
            if ((date_int>=date1_int)&&(date_int<=date2_int))
            {
                if (node->record->approved=='y')
                {
                    accepted_counter++;
                }
                else if (node->record->approved=='n')
                {
                    rejected_counter++;
                }
            }
        }
    }
    printf("Travel Statistics:\n"
            "\tTOTAL REQUESTS %d\n"
            "\tACCEPTED %d\n"
            "\tREJECTED %d\n",accepted_counter+rejected_counter,accepted_counter,rejected_counter);
}
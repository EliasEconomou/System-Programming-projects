#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "skip_list.h"
#include "country_list.h"
#include "various_functions.h"


struct skip_list
{
    char *virus_name;
    char *vacc_status; //'yes' for vaccinated skip list - 'no' for not vaccinated skip list
    Slnode head;
    Slnode tail;
    int size; //number of 'internal' nodes/citizens
    int levels; //the number of levels, so 7 levels means max level is 6 - min level is 0
};

struct slnode
{
    char* key;
    Slnode *next_table; //every node has a table that will contain pointers to the nodes this node connects to
    int cur_lvl; //'max level minus the null pointers of a node'
    char* dateVaccinated; //if 'vacc_status' has 'yes' this will have a date else NULL
    Cnode country;
};


char* get_virus(Skip_list skip_list)
{
    return (skip_list->virus_name);
}


char* get_vacc(Skip_list skip_list)
{
    return (skip_list->vacc_status);
}


//Create an empty skip list and return a pointer to it.
Skip_list create_skip_list(int levels, char* virus_name, char* vacc_status)
{
    Skip_list skip_list = malloc(sizeof(struct skip_list));
    if (skip_list==NULL)
    {
        perror("Error in malloc");
        exit(1);
    }
    skip_list->head = malloc(sizeof(struct slnode));
    if (skip_list->head==NULL)
    {
        perror("Error in malloc");
        exit(1);
    }
    skip_list->tail = malloc(sizeof(struct slnode));
    if (skip_list->tail==NULL)
    {
        perror("Error in malloc");
        exit(1);
    }
    skip_list->head->next_table = malloc(sizeof(struct slnode)*levels);
    if (skip_list->head->next_table==NULL)
    {
        perror("Error in malloc");
        exit(1);
    }
    skip_list->tail->next_table = malloc(sizeof(struct slnode)*levels);
    if (skip_list->tail->next_table==NULL)
    {
        perror("Error in malloc");
        exit(1);
    }
    skip_list->levels = levels;
    skip_list->virus_name = malloc(strlen(virus_name)+1);
    if (skip_list->virus_name==NULL)
    {
        perror("Error in malloc");
        exit(1);
    }
    skip_list->vacc_status = malloc(strlen(vacc_status)+1);
    if (skip_list->vacc_status==NULL)
    {
        perror("Error in malloc");
        exit(1);
    }
    strcpy(skip_list->virus_name,virus_name);
    strcpy(skip_list->vacc_status,vacc_status);
    skip_list->size=0;
    //initially all head pointers point to tail while tail pointers point to NULL
    for (int i = 0; i < levels; i++)
    {
        skip_list->head->next_table[i] = skip_list->tail;
        skip_list->tail->next_table[i] = NULL;
    }
    skip_list->tail->key = malloc(sizeof(skip_list->tail->key));
    if (skip_list->tail->key==NULL)
    {
        perror("Error in malloc");
        exit(1);
    }
    strcpy(skip_list->tail->key,"~~~~"); //large ascii characters, we will never enter - i hope :-)
    skip_list->tail->dateVaccinated=NULL;
    skip_list->tail->country=NULL;
    return skip_list;
}


//Determine the current level of a new node - used in 'insert_skip_list'.
int random_lvl(int levels)
{
    int cur_level=0;
    

    for (int i = 1; i < levels; i++) //run level-1 times
    {
        int random = rand();
        if (random%100<25) //25% chance to grow a node's level
        {
            cur_level++;
        }
    }
    return cur_level;
}


//Insert key to skip list.
void insert_skip_list(Skip_list skip_list, char* key, char* date, char* country_name)
{

    Slnode node = skip_list->head;
    Slnode previous_table[skip_list->levels];
    for (int i = skip_list->levels-1; i >= 0; i--)
    {
        while((node->next_table[i]!=skip_list->tail)&&(strcmp(node->next_table[i]->key,key)<0))
        {
            node=node->next_table[i];
        }
        //store the 'last' nodes that we'll probably (depends on the random level) need to change where they point to
        previous_table[i] = node;
    }
    node=node->next_table[0];

    if (strcmp(node->key,key)==0)
    {
        strcpy(node->key,key);
    }
    else
    {
        node = malloc(sizeof(struct slnode));
        if (node==NULL)
        {
            perror("Error in malloc");
            exit(1);
        }
        node->next_table = malloc(sizeof(struct slnode)*skip_list->levels);
        if (node->next_table==NULL)
        {
            perror("Error in malloc");
            exit(1);
        }
        node->key = malloc(sizeof(node->key));
        if (node->key==NULL)
        {
            perror("Error in malloc");
            exit(1);
        }
        strcpy(node->key,key);
        node->country = insert_country_list(country_name,skip_list);
        if (date!=NULL)
        {
            node->dateVaccinated = malloc(strlen(date)+1);
            if (node->dateVaccinated==NULL)
            {
                perror("Error in malloc");
                exit(1);
            }
            strcpy(node->dateVaccinated,date);
        }
        else
            node->dateVaccinated = NULL;
        node->cur_lvl = random_lvl(skip_list->levels);
        for (int i = 0; i <= node->cur_lvl; i++)
        {
            //the 'next' pointers of the new node will point where the previous did
            node->next_table[i] = previous_table[i]->next_table[i];
            //and the previous will point to the new node
            previous_table[i]->next_table[i] = node;
        }
        skip_list->size++;
    }
}


//Search for a key and return 1 if found - 0 if not.
int search_skip_list(Skip_list skip_list,char* key)
{

    if (skip_list==NULL) //skip list doesn't exist yet - key not found
    {
        return 0;
    }
    
    Slnode node = skip_list->head;
    //loop all levels from top to bottom
    for (int i = skip_list->levels-1; i >= 0; i--)
    {
        while((node->next_table[i]!=skip_list->tail)&&(strcmp(node->next_table[i]->key,key)<0))
        {
            node=node->next_table[i];
        }
    }
    
    node=node->next_table[0];
    if (strcmp(node->key,key)==0)
    {
        return 1;
    }
    else
    {
        return 0;
    }
    return 0;
}


//Print all records of skip list.
void print_skip_list(Skip_list skip_list)
{
    Slnode node = skip_list->head;
    while (node->next_table[0]!=skip_list->tail)
    {
        node=node->next_table[0];
        Citizen_record rec = search_hash_table(node->key);
        printf("%s %s %s %s %d \n",node->key,rec->firstName,rec->lastName,get_country_name(node->country),rec->age);
    }
}


//Iterate every node of skip list and count the citizens that are/aren't vaccinated for
//the specific country and virus in the exact dates. (cvd=country-virus-dates)
int cvd_population_status(Skip_list skip_list, char* country_name, int date1, int date2)
{
    Slnode node = skip_list->head;
    int counter=0;
    while (node->next_table[0]!=skip_list->tail)
    {
        node=node->next_table[0];
        if (strcmp(country_name,get_country_name(node->country))==0)
        {
            if (node->dateVaccinated==NULL) //we are in a 'no-date skip list'
            {
                counter++;
                continue;
            }
            int citizen_date = check_reformat_date(node->dateVaccinated);
            if ((citizen_date>=date1)&&(citizen_date<=date2))
            {
                counter++;
            }
        }
    }
    return counter;
}


//Iterate every node of skip list and count the citizens that are/aren't vaccinated for
//the specific country and virus. (cv=country-virus)
int cv_population_status(Skip_list skip_list, char* country_name)
{
    Slnode node = skip_list->head;
    int counter=0;
    while (node->next_table[0]!=skip_list->tail)
    {
        node=node->next_table[0];
        if (strcmp(country_name,get_country_name(node->country))==0)
        {
            counter++;
        } 
    }
    return counter;
}


//Iterate every node of skip list and count the citizens that are/aren't vaccinated for
//the specific country and virus in the exact dates for every age group. (cvd=country-virus-dates)
void cvd_pop_status_by_age(Skip_list skip_yes, Skip_list skip_no, char* country_name, int date1, int date2)
{
    //age groups
    int g1_yes=0, g1_no=0; //age: 0-20
    int g2_yes=0, g2_no=0; //age: 20-40
    int g3_yes=0, g3_no=0; //age: 40-60
    int g4_yes=0, g4_no=0; //age: 60+
    if (skip_yes!=NULL)
    {
        Slnode node_yes = skip_yes->head;
        while (node_yes->next_table[0]!=skip_yes->tail)
        {
            node_yes=node_yes->next_table[0];
            if (strcmp(country_name,get_country_name(node_yes->country))==0)
            {
                int citizen_date = check_reformat_date(node_yes->dateVaccinated);
                if ((citizen_date>=date1)&&(citizen_date<=date2))
                {
                    Citizen_record rec = search_hash_table(node_yes->key);
                    if ((rec->age>0)&&(rec->age<=20))
                    {
                        g1_yes++;
                    }
                    else if ((rec->age>20)&&(rec->age<=40))
                    {
                        g2_yes++;
                    }
                    else if ((rec->age>40)&&(rec->age<=60))
                    {
                        g3_yes++;
                    }
                    else if (rec->age>60)
                    {
                        g4_yes++;
                    }
                }
            }
        }
    }
    if (skip_no!=NULL)
    {
        Slnode node_no = skip_no->head;
        while (node_no->next_table[0]!=skip_no->tail)
        {
            node_no=node_no->next_table[0];
            if (strcmp(country_name,get_country_name(node_no->country))==0)
            {
                Citizen_record rec = search_hash_table(node_no->key);
                if ((rec->age>0)&&(rec->age<=20))
                {
                    g1_no++;
                }
                else if ((rec->age>20)&&(rec->age<=40))
                {
                    g2_no++;
                }
                else if ((rec->age>40)&&(rec->age<=60))
                {
                    g3_no++;
                }
                else if (rec->age>60)
                {
                    g4_no++;
                }
            }
        }
    }
    printf("%s\n",country_name);
    if (g1_yes==0&&g1_no==0)
        printf("0-20 \t %d \tno citizen found\n",g1_yes);
    else
        printf("0-20 \t %d \t%.2f%%\n",g1_yes,100*g1_yes/(float)(g1_yes+g1_no));
    if (g2_yes==0&&g2_no==0)
        printf("20-40 \t %d \tno citizen found\n",g2_yes);
    else
        printf("20-40 \t %d \t%.2f%%\n",g2_yes,100*g2_yes/(float)(g2_yes+g2_no));
    if (g3_yes==0&&g3_no==0)
        printf("40-60 \t %d \tno citizen found\n",g3_yes);
    else
        printf("40-60 \t %d \t%.2f%%\n",g3_yes,100*g3_yes/(float)(g3_yes+g3_no));
    if (g4_yes==0&&g4_no==0)
        printf("60+ \t %d \tno citizen found\n",g4_yes);
    else
        printf("60+ \t %d \t%.2f%%\n",g4_yes,100*g4_yes/(float)(g4_yes+g4_no));
}


//Iterate every node of skip list and count the citizens that are/aren't vaccinated for
//the specific country and virus for every age group. (cv=country-virus)
void cv_pop_status_by_age(Skip_list skip_yes, Skip_list skip_no, char* country_name)
{
    //age groups
    int g1_yes=0, g1_no=0; //age: 0-20
    int g2_yes=0, g2_no=0; //age: 20-40
    int g3_yes=0, g3_no=0; //age: 40-60
    int g4_yes=0, g4_no=0; //age: 60+

    if (skip_yes!=NULL)
    {
        Slnode node_yes = skip_yes->head;
        while (node_yes->next_table[0]!=skip_yes->tail)
        {
            node_yes=node_yes->next_table[0];
            if (strcmp(country_name,get_country_name(node_yes->country))==0)
            {
                Citizen_record rec = search_hash_table(node_yes->key);
                if ((rec->age>0)&&(rec->age<=20))
                {
                    g1_yes++;
                }
                else if ((rec->age>20)&&(rec->age<=40))
                {
                    g2_yes++;
                }
                else if ((rec->age>40)&&(rec->age<=60))
                {
                    g3_yes++;
                }
                else if (rec->age>60)
                {
                    g4_yes++;
                }
            }
        }
    }
    if (skip_no!=NULL)
    {
        Slnode node_no = skip_no->head;
        while (node_no->next_table[0]!=skip_no->tail)
        {
            node_no=node_no->next_table[0];
            if (strcmp(country_name,get_country_name(node_no->country))==0)
            {
                Citizen_record rec = search_hash_table(node_no->key);
                if ((rec->age>0)&&(rec->age<=20))
                {
                    g1_no++;
                }
                else if ((rec->age>20)&&(rec->age<=40))
                {
                    g2_no++;
                }
                else if ((rec->age>40)&&(rec->age<=60))
                {
                    g3_no++;
                }
                else if (rec->age>60)
                {
                    g4_no++;
                }
            }
        }
    }
    printf("%s\n",country_name);
    if (g1_yes==0&&g1_no==0)
        printf("0-20 \t %d \tno citizen found\n",g1_yes);
    else
        printf("0-20 \t %d \t%.2f%%\n",g1_yes,100*g1_yes/(float)(g1_yes+g1_no));
    if (g2_yes==0&&g2_no==0)
        printf("20-40 \t %d \tno citizen found\n",g2_yes);
    else
        printf("20-40 \t %d \t%.2f%%\n",g2_yes,100*g2_yes/(float)(g2_yes+g2_no));
    if (g3_yes==0&&g3_no==0)
        printf("40-60 \t %d \tno citizen found\n",g3_yes);
    else
        printf("40-60 \t %d \t%.2f%%\n",g3_yes,100*g3_yes/(float)(g3_yes+g3_no));
    if (g4_yes==0&&g4_no==0)
        printf("60+ \t %d \tno citizen found\n",g4_yes);
    else
        printf("60+ \t %d \t%.2f%%\n",g4_yes,100*g4_yes/(float)(g4_yes+g4_no));
}


//Remove a key from skip list.
void remove_skip_list(Skip_list skip_list, char* key)
{
    Slnode node = skip_list->head;
    Slnode previous_table[skip_list->levels];
    for (int i = skip_list->levels-1; i >= 0; i--)
    {
        while((node->next_table[i]!=skip_list->tail)&&(strcmp(node->next_table[i]->key,key)<0))
        {
            node=node->next_table[i];
        }
        previous_table[i] = node;
    }
    node=node->next_table[0];
    if (strcmp(node->key,key)==0)
    {
        for (int i = 0; i <= node->cur_lvl; i++)
        {
            //the previous pointers will point to the 'next' of the 'to be deleted' node
            previous_table[i]->next_table[i] = node->next_table[i];
        }
        free(node->key);
        free(node->dateVaccinated);
        free(node->next_table);
        free(node);
        skip_list->size--;
    }
}


//Get the date a given citizen got vaccinated. If no date return NULL.
char* get_date_skip_list(Skip_list skip_list,char* key)
{

    if (skip_list==NULL) //skip list doesn't exist
    {
        return NULL;
    }
    
    Slnode node = skip_list->head;
    //loop all levels from top to bottom
    for (int i = skip_list->levels-1; i >= 0; i--)
    {
        while((node->next_table[i]!=skip_list->tail)&&(strcmp(node->next_table[i]->key,key)<0))
        {
            node=node->next_table[i];
        }
    }
    
    node=node->next_table[0];
    if ((strcmp(node->key,key)==0) && (node->dateVaccinated!=NULL))
    {
        return node->dateVaccinated;
    }
    else
    {
        return NULL;
    }
    return NULL;
}


//Free skip list.
void delete_skip_list(Skip_list skip_list)
{
    if (skip_list->size==0) //empty skip list - no nodes
    {
        free(skip_list->tail->key);
        free(skip_list->virus_name);
        free(skip_list->vacc_status);
        free(skip_list->tail->next_table);
        free(skip_list->head->next_table);
        free(skip_list->tail);
        free(skip_list->head);
        free(skip_list);
        return;
    }
    Slnode node = skip_list->head->next_table[0];
    while (node->next_table[0]!=skip_list->tail)
    {  
        
        Slnode temp = node;
        node=node->next_table[0];
        free(temp->key);
        free(temp->dateVaccinated); 
        free(temp->next_table);   
        free(temp);
        
    }
    free(node->dateVaccinated);
    free(node->key);
    free(node->next_table);   
    free(node);
    free(skip_list->tail->key);
    free(skip_list->virus_name);
    free(skip_list->vacc_status);
    free(skip_list->tail->next_table);
    free(skip_list->head->next_table);
    free(skip_list->tail);
    free(skip_list->head);
    free(skip_list);
    
}
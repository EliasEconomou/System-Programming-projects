#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "country_list.h"

struct country_list
{
    Cnode cnode;
    int total_countries; //total number of countries
};

struct cnode
{
    char* country_name;
    Cnode next;
    Skip_list *table_of_viruses; //pointers to skip lists
    int skip_size; //current number of skip lists
};

Country_list c_list;

//Create country list.
void create_country_list()
{
    c_list = malloc(sizeof(struct country_list));
    if (c_list==NULL)
    {
        perror("Error in malloc");
        exit(1);
    }
    c_list->cnode=NULL;
    c_list->total_countries=0;
}


//Search for a country and return its node.
Cnode search_country_list(char* country_name)
{
    Cnode node = c_list->cnode;
    if (node==NULL)
    {
        return NULL;
    }
    else
    {
        
        if (strcmp(node->country_name,country_name)==0)
        {
            return node;
        }
        
        while (node->next!=NULL)
        {
            node=node->next;
            if (strcmp(node->country_name,country_name)==0)
            {
                return node;
            }
        }   
        return NULL;
    }
}


//Insert country name to country list and return a pointer to its node.
Cnode insert_country_list(char* country_name, Skip_list skip_list)
{
    Cnode search_node = search_country_list(country_name);
    if (search_node!=NULL) //country found
    {
        for (int i = 0; i < search_node->skip_size; i++)
        {
            if (search_node->table_of_viruses[i]==skip_list)
            {
                return search_node;
            }
        }
        //reallocate the space we had before plus 1 for NULL
        search_node->table_of_viruses = realloc(search_node->table_of_viruses,(search_node->skip_size+2)*sizeof(Skip_list));
        if (search_node->table_of_viruses==NULL)
        {
            perror("Error in malloc");
            exit(1);
        }
        search_node->table_of_viruses[search_node->skip_size] = skip_list;
        search_node->table_of_viruses[search_node->skip_size+1] = NULL;
        search_node->skip_size++;
        return search_node;
    }
    
    //country not found, create a new one
    c_list->total_countries++;
    Cnode new_node = malloc(sizeof(struct cnode));
    if (new_node==NULL)
    {
        perror("Error in malloc");
        exit(1);
    }
    new_node->country_name = malloc(strlen(country_name)+1);
    if (new_node->country_name==NULL)
    {
        perror("Error in malloc");
        exit(1);
    }
    new_node->skip_size=0;
    //malloc size of two skip lists, first position for the skip list, second NULL
    new_node->table_of_viruses = malloc((new_node->skip_size+2)*sizeof(Skip_list));
    if (new_node->table_of_viruses==NULL)
    {
        perror("Error in malloc");
        exit(1);
    }
    new_node->table_of_viruses[new_node->skip_size+1] = NULL;
    new_node->table_of_viruses[new_node->skip_size] = skip_list;
    new_node->skip_size++;
    strcpy(new_node->country_name,country_name);
    new_node->next = c_list->cnode;
    c_list->cnode = new_node; 
    return new_node;
}


//Get number of countries.
int get_countries_number()
{
    return c_list->total_countries;
}


//Give a country node and return its country name.
char* get_country_name(Cnode node)
{
    return(node->country_name);
}


//Print countries and the viruses they are vaccinated for.
void print_country_list()
{
    if (c_list->cnode!=NULL)
    {
        Cnode node = c_list->cnode;
        printf(" < %s > ",node->country_name);
        int i=0;
        while (node->table_of_viruses[i]!=NULL)
        {
            
            printf("%s ",get_virus(node->table_of_viruses[i]));
            i++;
        }
        printf("\n");
        while (node->next!=NULL)
        {
            
            node=node->next;
            printf(" < %s > ",node->country_name);
            int i=0;
            while (node->table_of_viruses[i]!=NULL)
            {
                printf("%s ",get_virus(node->table_of_viruses[i]));
                i++;
            }
            printf("\n");   
        }
        
    }
    printf("\n");
}


//Iterate every node of country list, find any skip lists for the ginen virus and 
//count the citizens that are/aren't vaccinated for it. (v=virus)
void v_population_status(char* virus_name)
{
    if (c_list->cnode!=NULL)
    {
        Cnode node = c_list->cnode;
        int citizen_count_yes = 0, citizen_count_no = 0;
        int i=0;
        while (node->table_of_viruses[i]!=NULL) //search the country's table of pointers to viruses/skip lists
        {
            if (strcmp(virus_name,get_virus(node->table_of_viruses[i]))==0) //find the right skip list(s)
            {
                if (strcmp("YES",get_vacc(node->table_of_viruses[i]))==0) //chech whether it's a 'yes' skip list
                {
                    citizen_count_yes = cv_population_status(node->table_of_viruses[i], node->country_name);
                }
                if (strcmp("NO",get_vacc(node->table_of_viruses[i]))==0) //chech whether it's a 'no' skip list
                {
                    citizen_count_no = cv_population_status(node->table_of_viruses[i], node->country_name);
                }
                
            }
            i++;
        }
        if (citizen_count_yes!=0||citizen_count_no!=0)
        {
            printf("%s %d %.2f%%\n",node->country_name,citizen_count_yes,100*citizen_count_yes/(float)(citizen_count_yes+citizen_count_no));
        }  
       
        while (node->next!=NULL)
        {
            citizen_count_yes = 0;
            citizen_count_no = 0;
            node=node->next;
            int i=0;
            while (node->table_of_viruses[i]!=NULL)
            {
                if (strcmp(virus_name,get_virus(node->table_of_viruses[i]))==0) //find the right skip list(s)
                {
                    if (strcmp("YES",get_vacc(node->table_of_viruses[i]))==0) //chech whether it's a 'yes' skip list
                    {
                        citizen_count_yes = cv_population_status(node->table_of_viruses[i], node->country_name);
                    }
                    if (strcmp("NO",get_vacc(node->table_of_viruses[i]))==0) //chech whether it's a 'no' skip list
                    {
                        citizen_count_no = cv_population_status(node->table_of_viruses[i], node->country_name);
                    }
                    
                }
                i++;
            }
            if (citizen_count_yes!=0||citizen_count_no!=0)
            {
                printf("%s %d %.2f%%\n",node->country_name,citizen_count_yes,100*citizen_count_yes/(float)(citizen_count_yes+citizen_count_no));
            } 
        }   
    }
}


//Iterate every node of country list, find any skip lists for the ginen virus and 
//count the citizens that are/aren't vaccinated for it in the given dates. (vd=virus-dates)
void vd_population_status(char* virus_name, int date1, int date2)
{
    if (c_list->cnode!=NULL)
    {
        Cnode node = c_list->cnode;
        int citizen_count_yes = 0, citizen_count_no = 0;
        int i=0;
        while (node->table_of_viruses[i]!=NULL) //search the country's table of pointers to viruses/skip lists
        {
            if (strcmp(virus_name,get_virus(node->table_of_viruses[i]))==0) //find the right skip list(s)
            {
                if (strcmp("YES",get_vacc(node->table_of_viruses[i]))==0) //chech whether it's a 'yes' skip list
                {
                    citizen_count_yes = cvd_population_status(node->table_of_viruses[i], node->country_name, date1, date2);
                }
                if (strcmp("NO",get_vacc(node->table_of_viruses[i]))==0) //chech whether it's a 'no' skip list
                {
                    citizen_count_no = cvd_population_status(node->table_of_viruses[i], node->country_name, date1, date2);
                }
                
            }
            i++;
        }
        if (citizen_count_yes!=0||citizen_count_no!=0)
        {
            printf("%s %d %.2f%%\n",node->country_name,citizen_count_yes,100*citizen_count_yes/(float)(citizen_count_yes+citizen_count_no));
        }  
       
        while (node->next!=NULL)
        {
            citizen_count_yes = 0;
            citizen_count_no = 0;
            node=node->next;
            int i=0;
            while (node->table_of_viruses[i]!=NULL)
            {
                if (strcmp(virus_name,get_virus(node->table_of_viruses[i]))==0) //find the right skip list(s)
                {
                    if (strcmp("YES",get_vacc(node->table_of_viruses[i]))==0) //chech whether it's a 'yes' skip list
                    {
                        citizen_count_yes = cvd_population_status(node->table_of_viruses[i], node->country_name, date1, date2);
                    }
                    if (strcmp("NO",get_vacc(node->table_of_viruses[i]))==0) //chech whether it's a 'no' skip list
                    {
                        citizen_count_no = cvd_population_status(node->table_of_viruses[i], node->country_name, date1, date2);
                    }
                    
                }
                i++;
            }
            if (citizen_count_yes!=0||citizen_count_no!=0)
            {
                printf("%s %d %.2f%%\n",node->country_name,citizen_count_yes,100*citizen_count_yes/(float)(citizen_count_yes+citizen_count_no));
            } 
        }   
    }
}


//Iterate every node of country list, find any skip lists for the ginen virus and 
//count the citizens that are/aren't vaccinated for it for every age group. (v=virus)
void v_pop_status_by_age(char* virus_name)
{
    if (c_list->cnode!=NULL)
    {
        Cnode node = c_list->cnode;
        Skip_list skip_yes=NULL;
        Skip_list skip_no=NULL;
        int i=0;
        while (node->table_of_viruses[i]!=NULL) //search the country's table of pointers to viruses/skip lists
        {
            if (strcmp(virus_name,get_virus(node->table_of_viruses[i]))==0) //find the right skip list(s)
            {
                if (strcmp("YES",get_vacc(node->table_of_viruses[i]))==0) //chech whether it's a 'yes' skip list
                {
                    skip_yes = node->table_of_viruses[i];
                }
                if (strcmp("NO",get_vacc(node->table_of_viruses[i]))==0) //chech whether it's a 'no' skip list
                {
                    skip_no = node->table_of_viruses[i];
                }
            }
            i++;
        }
        cv_pop_status_by_age(skip_yes, skip_no, get_country_name(node));
        printf("\n");

        while (node->next!=NULL)
        {
            node=node->next;
            int i=0;
            while (node->table_of_viruses[i]!=NULL)
            {
                if (strcmp(virus_name,get_virus(node->table_of_viruses[i]))==0) //find the right skip list(s)
                {
                    if (strcmp("YES",get_vacc(node->table_of_viruses[i]))==0) //chech whether it's a 'yes' skip list
                    {
                        skip_yes = node->table_of_viruses[i];
                    }
                    if (strcmp("NO",get_vacc(node->table_of_viruses[i]))==0) //chech whether it's a 'no' skip list
                    {
                        skip_no = node->table_of_viruses[i];
                    }
                }
                i++;
            }
            cv_pop_status_by_age(skip_yes, skip_no, get_country_name(node));
            printf("\n");
        }   
    }
}


//Iterate every node of country list, find any skip lists for the ginen virus and 
//count the citizens that are/aren't vaccinated for it in the given dates for every age group. (vd=virus-dates)
void vd_pop_status_by_age(char* virus_name, int date1, int date2)
{
    if (c_list->cnode!=NULL)
    {
        Cnode node = c_list->cnode;
        Skip_list skip_yes=NULL;
        Skip_list skip_no=NULL;
        int i=0;
        while (node->table_of_viruses[i]!=NULL) //search the country's table of pointers to viruses/skip lists
        {
            if (strcmp(virus_name,get_virus(node->table_of_viruses[i]))==0) //find the right skip list(s)
            {
                if (strcmp("YES",get_vacc(node->table_of_viruses[i]))==0) //chech whether it's a 'yes' skip list
                {
                    skip_yes = node->table_of_viruses[i];
                }
                if (strcmp("NO",get_vacc(node->table_of_viruses[i]))==0) //chech whether it's a 'no' skip list
                {
                    skip_no = node->table_of_viruses[i];
                }
            }
            i++;
        }
        cvd_pop_status_by_age(skip_yes, skip_no, get_country_name(node), date1, date2);
        printf("\n");

        while (node->next!=NULL)
        {
            node=node->next;
            int i=0;
            while (node->table_of_viruses[i]!=NULL)
            {
                if (strcmp(virus_name,get_virus(node->table_of_viruses[i]))==0) //find the right skip list(s)
                {
                    if (strcmp("YES",get_vacc(node->table_of_viruses[i]))==0) //chech whether it's a 'yes' skip list
                    {
                        skip_yes = node->table_of_viruses[i];
                    }
                    if (strcmp("NO",get_vacc(node->table_of_viruses[i]))==0) //chech whether it's a 'no' skip list
                    {
                        skip_no = node->table_of_viruses[i];
                    }
                }
                i++;
            }
            cvd_pop_status_by_age(skip_yes, skip_no, get_country_name(node), date1, date2);
            printf("\n");
        }   
    }
}


//Free country list.
void delete_country_list()
{
    Cnode node = c_list->cnode;
    while (node->next!=NULL)
    {
        Cnode temp = node;
        node=node->next;
        free(temp->country_name);
        free(temp->table_of_viruses);
        free(temp);
    }
    free(node->country_name);
    free(node->table_of_viruses);
    free(node);
    free(c_list);
}
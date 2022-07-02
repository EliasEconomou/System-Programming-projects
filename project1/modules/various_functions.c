#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <time.h>
#include "various_functions.h"

//Used to count all records.
int count_lines(char* input_file)
{
    FILE *fp;
	fp = fopen(input_file,"r");
    if (fp==NULL)
    {
        perror("ERROR ");
        exit(0);
    }
    int lines=0;
    char str_rec[100];
    while (fgets(str_rec,sizeof(str_rec),fp))
    {
        lines++;
    }
    fclose(fp);
    return lines;
}


//Used for hash table creation.
int get_buckets(int records)
{
    int buckets_num = records/0.9;
    if (buckets_num%2==0) //let number of buckets be odd
    {
        buckets_num++;
    }
    return buckets_num;
}


//Used for skip list creation. A skip list can have at most all records refer to the same virus.
int get_levels(int records)
{
    int lvl_counter=1;
    while(records>=2)
    {
        lvl_counter++;
        records/=2;

    }
    return lvl_counter;
}


//Get current date.
void get_current_date(char** dd_str)
{
    time_t t = time(NULL);
    struct tm *local_time = localtime(&t);
    //according to localtime() function
    /*  tm_mday = day of the month, range 1 to 31     */
    /*  tm_mon = month, range 0 to 11                 */
    /*  tm_year = The number of years since 1900      */
    
    char temp_dd_str[3];
    char mm_str[3];
    char temp_mm_str[3];
    char yyyy_str[4];

    //append day
    sprintf(temp_dd_str,"%d",local_time->tm_mday);
    if (strlen(temp_dd_str)==1) //if one digit day - add 0 in front
    {
        strcat(*dd_str,"0");
        strcat(*dd_str,temp_dd_str);
    }
    else
    {
        strcpy(*dd_str,temp_dd_str);
    }
    strcat(*dd_str,"-");

    sprintf(temp_mm_str,"%d",local_time->tm_mon+1);
    if (strlen(temp_mm_str)==1) //if one digit month - add 0 in front
    {
        strcpy(mm_str,"0");
        strcat(mm_str,temp_mm_str);
        strcat(*dd_str,mm_str);
    }
    else
    {
        strcat(*dd_str,temp_mm_str);
    }
    strcat(*dd_str,"-");

    sprintf(yyyy_str,"%d",local_time->tm_year+1900);
    strcat(*dd_str,yyyy_str);
}


//Check if a date is valid and if it is, return a reformatted integer - else return -1
//(for example: if date is 17-10-2007 return 20071017)
int check_reformat_date(char* date)
{
    if (strlen(date)!=10) //invalid date
    {
        return -1; 
    }
    char str_date[9];
    char temp_str[11];
    strcpy(temp_str,date);
    char* dd = strtok(temp_str,"-");
    if (strlen(dd)!=2) //valid day
    {
        return -1;
    }
    int ddint = atoi(dd);
    if ((ddint<1)||(ddint>30))
    {
        return -1;
    }   
    
    char* mm = strtok(NULL,"-");
    if (strlen(mm)!=2) //valid month
    {
        return -1;
    }
    int mmint = atoi(mm);
    if ((mmint<1)||(mmint>12))
    {
        return -1;
    }

    char* yyyy = strtok(NULL," ");
    if (strlen(yyyy)==4) //valid year
    {
        for (int i = 0; i < 3; i++)
        {
            if (!isdigit(yyyy[i]))
            {
                return -1;
            }
        }
    }
    else
        return -1;
    
    strcpy(str_date,yyyy);
    strcat(str_date,mm);
    strcat(str_date,dd);
    int date_int = atoi(str_date);
    return date_int;
}
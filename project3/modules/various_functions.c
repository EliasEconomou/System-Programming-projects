#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <time.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <dirent.h>
#include <signal.h>
#include <errno.h>
#include "various_functions.h"

//Read from pipe or write to pipe.
int string_pipe(char* function, int fd, char* string, int string_length, int buffer_size)
{
    int bytes;
    if (strcmp(function,"read")==0||strcmp(function,"READ")==0)
    {
        char buf[buffer_size];
        int buff_times=string_length/buffer_size; //these many times we read bytes equal to buffersize
        int remaining_bytes=string_length%buffer_size; //bytes left, less than buffersize
        while (buff_times>0)
        {
            bytes = read(fd,buf,buffer_size);
            if (bytes==-1)
            {
                if (errno==EINTR)
                {
                    return -1;
                }
            }
            
            buff_times--;
            strcat(string,buf);
        }  
        if (remaining_bytes>0)
        {
            for (int i = 0; i < buffer_size; i++)
            {
                buf[i]=0;
            }
            bytes = read(fd,buf,remaining_bytes);
            if (bytes==-1)
            {
                if (errno==EINTR)
                {
                    return -1;
                }
            }
            strcat(string,buf);
        }
        return 0;
    }
    else if(strcmp(function,"write")==0||strcmp(function,"WRITE")==0)
    {
        char* ptr_buf;
        int buff_times=string_length/buffer_size;
        int remaining_bytes=string_length%buffer_size;
        int buff_times_copy=buff_times;
        while (buff_times_copy>0)
        {
            ptr_buf = string + ((buff_times-buff_times_copy) * buffer_size);
            bytes = write(fd,ptr_buf,buffer_size);
            if (bytes==-1)
            {
                if (errno==EINTR)
                {
                    printf("signal\n");
                    return -1;
                }
            }
            buff_times_copy--;
        }  
        if (remaining_bytes>0)
        {
            ptr_buf = string + ((buff_times-buff_times_copy) * buffer_size);
            bytes = write(fd,ptr_buf,remaining_bytes);
            if (bytes==-1)
            {
                if (errno==EINTR)
                {
                    printf("signal\n");
                    return -1;
                }
            }
        }
        return 0;
    }

    printf("Error in arguments.\n");
    return -1;
}

//String comparison function for qsort.
int string_cmp(const void *string1, const void *string2)
{ 
    const char **str1 = (const char **)string1;
    const char **str2 = (const char **)string2;
    int result = strcmp(*str1,*str2);
    return result;
}

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
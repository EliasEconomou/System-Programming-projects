#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <dirent.h>
#include <errno.h>
#include "various_functions.h"
#include "filter_list.h"
#include "list.h"
#include "virus_table.h"

#define SIZE_OF_DATE 10 //dd-mm-yyyy

static int sigint_flag=0;
static int sigchld_flag=0;

static void iq_handler (int signo) //handle SIGINT and SIGQUIT
{
    sigint_flag=1;
}

static void c_handler (int signo) //handle SIGCHLD
{
    pid_t wpid;
    while ((wpid=waitpid(-1,NULL,WNOHANG))>0) //wait all the children that terminate
    {
        sigchld_flag=wpid;
    }
}

int main(int argc,char *argv[])
{
    //Establishing SIGINT/SIGQUIT handler.
    static struct sigaction iq_act; //handle SIGINT and SIGQUIT
	iq_act.sa_handler=iq_handler;
	sigfillset(&(iq_act.sa_mask)); //other signals will be blocked when handling
    iq_act.sa_flags = 0;
	sigaction(SIGINT, &iq_act, NULL);
    sigaction(SIGQUIT, &iq_act, NULL);

    //Establishing SIGCHLD handler.
    static struct sigaction c_act;
	c_act.sa_handler=c_handler;
	sigfillset(&(c_act.sa_mask)); //other signals will be blocked when handling
	sigaction(SIGCHLD, &c_act, NULL);

    printf("<TravelMonitor (pid : %d) starting>\n",getpid());

    if ((argc!=9)||(argv[1][0]!='-')||(argv[3][0]!='-')||(argv[5][0]!='-')||(argv[7][0]!='-')) //must have flags
    {
        printf("Error in arguments. Execute program as: ./travelMonitor -m numMonitors -b bufferSize -s sizeOfBloom -i input_dir\n\n");
        return (-1);
    } 
    if ((argv[1][1]!='m')||(argv[3][1]!='b')||(argv[5][1]!='s')||(argv[7][1]!='i')) //checking the flags
    {
        printf("Error in arguments. Execute program as: ./travelMonitor -m numMonitors -b bufferSize -s sizeOfBloom -i input_dir\n\n");
        return (-1);
    }
    if ((argv[1][2]!='\0')||(argv[3][2]!='\0')||(argv[5][2]!='\0')||(argv[7][2]!='\0'))
    {
        printf("Error in arguments. Execute program as: ./travelMonitor -m numMonitors -b bufferSize -s sizeOfBloom -i input_dir\n\n");
        return (-1);
    }
    int numMonitors = atoi(argv[2]);
    int bufferSize = atoi(argv[4]);
    int sizeOfBloom = atoi(argv[6]);
    if ((numMonitors<1)||(bufferSize<1)||(sizeOfBloom<1)) //checking given arguments
    {
        printf("Error in arguments. NumMonitors - bufferSize - sizeOfBloom must be integers greater than 0.\n\n");
        return (-1);
    }
    char* input_dir = malloc(strlen("../../")+strlen(argv[8])+1);
    if (input_dir==NULL)
    {
        perror("Error in malloc");
        exit(1);
    }
    strcpy(input_dir,"../../");
    strcat(input_dir,argv[8]);
    DIR *dir = opendir(input_dir); //checking if given directory exists
    if(dir == NULL)
    {
        perror("Error in directory");
        exit(1);
    }

    //Find the number of countries.
    struct dirent *directory;
    int num_countries=0;
    while ((directory = readdir(dir))) //loop subdirectories to get number of countries
    {
        if((strcmp(directory->d_name,".")==0)||(strcmp(directory->d_name,"..")==0)) //skip ./ and ../
        {
            continue;
        }
        num_countries++;
    }
    closedir(dir);
    
    if (numMonitors>num_countries) //no need to keep more monitors than countries
    {
        numMonitors=num_countries;
    }
     
    //Creating two pipes for every monitor-child that will be forked and storing their names.
    char read_fifos[numMonitors][20];
    char write_fifos[numMonitors][20];
    char rd_i[20];
    char wr_i[20];
    int check;
    for (int i = 0; i < numMonitors; i++)
    {
        sprintf(rd_i,"../pipes/read_%d",i);
        strcpy(read_fifos[i],rd_i);
        sprintf(wr_i,"../pipes/write_%d",i);
        strcpy(write_fifos[i],wr_i);
        check = mkfifo(rd_i,0666);
        if (check==-1)
        {
            perror("Error in mkfifo");
            exit(1);
        }
        check = mkfifo(wr_i,0666);
        if (check==-1)
        {
            perror("Error in mkfifo");
            exit(1);
        }
    }
    int read_fds[numMonitors]; //keep fds for reading from pipes
    int write_fds[numMonitors]; //keep fds for writing to pipes
    int pids[numMonitors]; //keep pids of children
    
    //Forking numMonitors children.
    pid_t pid;
    for (int i = 0; i < numMonitors; i++)
    {
        pid = fork();
        if (pid==-1)
        {
            perror("Error in fork");
            exit(1);
        }
        if (pid>0) //Parent process
        {
            pids[i]=pid;
            int fd = open(read_fifos[i], O_RDONLY);
            if (fd==-1)
            {
                perror("Error in opening fifo");
                exit(1);
            }
            read_fds[i]=fd;
            fd = open(write_fifos[i], O_WRONLY);
            if (fd==-1)
            {
                perror("Error in opening fifo");
                exit(1);
            }
            write_fds[i]=fd;
        }
        if (pid==0) //Child process uses Monitor
        {
            execl("../monitor/Monitor","Monitor",read_fifos[i],write_fifos[i],NULL);
        }  
    }

    //Children used exec so we are in parent process:
    
    //TravelMonitor will keep track of total travel requests to all countries (for log file).
    int total_rejected_requests=0;
    int total_accepted_requests=0;

    //Passing bufferSize to children through pipes.
    for (int i = 0; i < numMonitors; i++)
    {
        write(write_fds[i],&bufferSize,sizeof(int));
    }

    //Passing sizeOfBloom to children through pipes.
    for (int i = 0; i < numMonitors; i++)
    {
        write(write_fds[i],&sizeOfBloom,sizeof(int));
    }

    //Passing input_dir' name to children through pipes.
    for (int i = 0; i < numMonitors; i++)
    {
        int dir_length = strlen(input_dir);
        write(write_fds[i],&dir_length,sizeof(int));
        string_pipe("write",write_fds[i],input_dir,dir_length,bufferSize);
    }



    //Keep all countries.
    char** countries = malloc(num_countries * sizeof(char*));
    dir = opendir(input_dir);
    int i=0;
    while ((directory = readdir(dir))) //loop subdirectories to get every country
    {
        if((strcmp(directory->d_name,".")==0)||(strcmp(directory->d_name,"..")==0)) //skip ./ and ../
        {
            continue;
        }
        countries[i] = malloc(strlen(directory->d_name) + 1);
        strcpy(countries[i],directory->d_name);
        i++;
    }
    closedir(dir);

    //Sort countries alphabetically.    
    qsort(countries,num_countries,sizeof(char*),string_cmp);

    //Inform each child how many countries it will have.
    int countries_per_child[numMonitors];
    for (int i = 0; i < numMonitors; i++)
    {
        countries_per_child[i]=0;
    }
    for (int i = 0; i < num_countries; i++)
    {
        countries_per_child[i%numMonitors]++;
    }

    for (int i = 0; i < numMonitors; i++)
    {  
        write(write_fds[i],&countries_per_child[i],sizeof(int));
    }

    //Create maps with key:country and value:fd to hold the fds used to write/read to/from pipe
    //to a specific monitor for every country.
    List country_write_fd_list = create_list();
    List country_read_fd_list = create_list();
    List country_pid_list = create_list();

    int country_length;
    for (int i = 0; i < num_countries; i++)
    {
        country_length = strlen(countries[i]);
        //Determine which child-process will receive the current country.
        int process = i%numMonitors;
        write(write_fds[process],&country_length,sizeof(int));
        //Send the country's name to monitor.
        string_pipe("write",write_fds[process],countries[i],country_length,bufferSize);
        
        //Insert fds to map-lists.
        char* str_write_fd = malloc(5);
        sprintf(str_write_fd,"%d",write_fds[process]);
        insert_list(country_write_fd_list,countries[i],str_write_fd);
        free(str_write_fd);
        char* str_read_fd = malloc(5);
        sprintf(str_read_fd,"%d",read_fds[process]);
        insert_list(country_read_fd_list,countries[i],str_read_fd);
        free(str_read_fd);

        //Insert pids to country-pid map list.
        char* str_pid = malloc(10);
        sprintf(str_pid,"%d",pids[process]);
        insert_list(country_pid_list,countries[i],str_pid);
        free(str_pid);
    }

    //At these point program expects bloom filters from Monitors.
    //We'll read bloom filter's names then read bloom filter arrays of known size sizeOFBloom.
    
    create_filter_list(sizeOfBloom);
    for (int i = 0; i < numMonitors; i++)
    {
        //Get the number of bloom filters of monitor.
        int number_bloom_filters;
        read(read_fds[i],&number_bloom_filters,sizeof(int));
        for (int j = 0; j < number_bloom_filters; j++)
        {
            int virus_name_length;
            //Get virus name from monitor.
            read(read_fds[i],&virus_name_length,sizeof(int));
            char* virus_name = malloc(sizeof(char)*virus_name_length + 1);
            virus_name[0] = '\0';
            string_pipe("read",read_fds[i],virus_name,virus_name_length,bufferSize);

            //Get bloom filter's array from monitor.
            char* bf_array = malloc(sizeOfBloom);
            char* tempptr;
            if (bf_array==NULL)
            {
                perror("Error in malloc");
                exit(1);
            }
            for(int k=0; k<sizeOfBloom; k++) 
            {
                bf_array[k]=bf_array[k]&0;
            }
            Bloom_filter bloom_filter = insert_filter_list(virus_name);
            int buff_times=sizeOfBloom/bufferSize; //these many times we read bytes equal to buffersize
            int remaining_bytes=sizeOfBloom%bufferSize; //bytes left, less than buffersize
            int buff_times_copy = buff_times;
            while (buff_times_copy>0)
            {
                tempptr = bf_array + (buff_times-buff_times_copy) * bufferSize;
                read(read_fds[i],tempptr,bufferSize);
                buff_times_copy--;
            }  
            if (remaining_bytes>0)
            {
                tempptr = bf_array + (buff_times-buff_times_copy) * bufferSize;
                read(read_fds[i],tempptr,remaining_bytes);
            }
            merge_bloom_arrays(&bloom_filter,bf_array);
            free(virus_name);
            free(bf_array);
        }
    }




//////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////



    
    //About to begin receiving commands from user.

    int num_of_viruses=0; //keeps track of number of distinct viruses from travel requests
    Virus_table* table_of_viruses; //will include all virus table pointers from travel requests
    table_of_viruses = malloc(2*sizeof(Virus_table)); //allocate size for first pointer to virus table
    if (table_of_viruses==NULL)
    {
        perror("Error in malloc");
        exit(1);
    }

    
    char input[100], temp_input[100];
    char *command, *in1, *in2, *in3, *in4, *in5, *in6, *in7, *in8, *in9;
    do
    {
        printf("\nGive input to execute: ");
        fgets(input,sizeof(input),stdin);
        if (EINTR == errno) //did some signal occured?
        {
            if ((sigchld_flag!=0)) //has some child died?
            {
                int ch;
                for (ch = 0; ch < numMonitors; ch++)
                {
                    //Loop pids and get the dead child's position.
                    if (pids[ch]==sigchld_flag)
                    {
                        break;
                    }  
                }
                
                close(write_fds[ch]);
                close(read_fds[ch]);
                
                //Fork a new child and give it the info the old one had.
                pid = fork();
                if (pid==-1)
                {
                    perror("Error in fork");
                    exit(1);
                }
                if (pid>0) //Parent process
                {
                    pids[ch]=pid;
                    int fd = open(read_fifos[ch], O_RDONLY);
                    if (fd==-1)
                    {
                        perror("Error in opening fifo");
                        exit(1);
                    } 
                    read_fds[ch]=fd;
                    fd = open(write_fifos[ch], O_WRONLY);
                    if (fd==-1)
                    {
                        perror("Error in opening fifo");
                        exit(1);
                    }
                    write_fds[ch]=fd;
                }
                if (pid==0) //Child process uses Monitor.
                {
                    execl("../monitor/Monitor","Monitor",read_fifos[ch],write_fifos[ch],NULL);
                }

                //Passing bufferSize,sizeOfBloom and input_dir to new forked child.
                write(write_fds[ch],&bufferSize,sizeof(int));  
                write(write_fds[ch],&sizeOfBloom,sizeof(int));
                int dir_length = strlen(input_dir);
                write(write_fds[ch],&dir_length,sizeof(int));
                string_pipe("write",write_fds[ch],input_dir,dir_length,bufferSize);

                //Inform the new child how many countries it will have.
                int countries_per_child[numMonitors];
                for (int i = 0; i < numMonitors; i++)
                {
                    countries_per_child[i]=0;
                }
                for (int i = 0; i < num_countries; i++)
                {
                    countries_per_child[i%numMonitors]++;
                }
                for (int i = 0; i < numMonitors; i++)
                {
                    if (i%numMonitors==ch)
                    {
                        write(write_fds[ch],&countries_per_child[i],sizeof(int));
                    }
                }

                int country_length;
                for (int i = 0; i < num_countries; i++)
                {
                    //Determine which child-process will receive the current country.
                    int process = i%numMonitors;
                    //If it is the new child's turn, give it the country.
                    if (process==ch)
                    {
                        //Update the map-lists (Country:fd) with the new fds for the specific process.
                        country_length = strlen(countries[i]);
                        write(write_fds[ch],&country_length,sizeof(int));
                        string_pipe("write",write_fds[ch],countries[i],country_length,bufferSize);
                        char* str_write_fd = malloc(5);
                        sprintf(str_write_fd,"%d",write_fds[ch]);
                        update_list(country_write_fd_list,countries[i],str_write_fd);
                        free(str_write_fd);
                        char* str_read_fd = malloc(5);
                        sprintf(str_read_fd,"%d",read_fds[ch]);
                        update_list(country_read_fd_list,countries[i],str_read_fd);
                        free(str_read_fd);

                        //Update pids in 'country-pid map list' for the specific process.
                        char* str_pid = malloc(10);
                        sprintf(str_pid,"%d",pids[ch]);
                        update_list(country_pid_list,countries[i],str_pid);
                        free(str_pid);
                    }
                }

                //Get the number of bloom filters of the new monitor.
                int number_bloom_filters;
                read(read_fds[ch],&number_bloom_filters,sizeof(int));
                for (int j = 0; j < number_bloom_filters; j++)
                {
                    int virus_name_length;
                    //Get virus name from monitor.
                    read(read_fds[ch],&virus_name_length,sizeof(int));
                    char* virus_name = malloc(sizeof(char)*virus_name_length + 1);
                    virus_name[0] = '\0';
                    string_pipe("read",read_fds[ch],virus_name,virus_name_length,bufferSize);

                    //Get bloom filter's array from monitor.
                    char* bf_array = malloc(sizeOfBloom);
                    char* tempptr;
                    if (bf_array==NULL)
                    {
                        perror("Error in malloc");
                        exit(1);
                    }
                    for(int k=0; k<sizeOfBloom; k++) 
                    {
                        bf_array[k]=bf_array[k]&0;
                    }
                    Bloom_filter bloom_filter = insert_filter_list(virus_name);
                    int buff_times=sizeOfBloom/bufferSize; //these many times we read bytes equal to buffersize
                    int remaining_bytes=sizeOfBloom%bufferSize; //bytes left, less than buffersize
                    int buff_times_copy = buff_times;
                    while (buff_times_copy>0)
                    {
                        tempptr = bf_array + (buff_times-buff_times_copy) * bufferSize;
                        read(read_fds[ch],tempptr,bufferSize);
                        buff_times_copy--;
                    }  
                    if (remaining_bytes>0)
                    {
                        tempptr = bf_array + (buff_times-buff_times_copy) * bufferSize;
                        read(read_fds[ch],tempptr,remaining_bytes);
                    }
                    merge_bloom_arrays(&bloom_filter,bf_array);
                    free(virus_name);
                    free(bf_array);
                }
                sigchld_flag=0; //flag is 0 again, waiting for another child to die. That's sad :(
                continue;
            }
            //Check for SIGINT/SIGQUIT
            //Break, kill monitors, print to log file, free stuff and exit
            if (sigint_flag==1)
            {
                break;
            }
        }

        input[strcspn(input, "\n")] = 0; //removing new line
        strcpy(temp_input,input); //break record into pieces - keep input saved
        command = strtok(input," ");
        if (command==NULL)
        {
            continue;
        }
        if (command[0]!='/')
        {
            printf("Give command as : /command\n");
            continue;
        }

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////    /travelRequest    /////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

        if (strcmp(command,"/travelRequest")==0)
        {
            in1 = strtok(NULL," "); //citizenID
            in2 = strtok(NULL," "); //date
            in3 = strtok(NULL," "); //countryFrom
            in4 = strtok(NULL," "); //countryTo
            in5 = strtok(NULL," "); //virusName
            in6 = strtok(NULL," "); //NULL
            if ((in1==NULL)||(in2==NULL)||(in3==NULL)||(in4==NULL)||(in5==NULL)||(in6!=NULL))
            {
                printf("WRONG INPUT! Try again as : /travelRequest citizenID date countryFrom countryTo virusName\n");
                continue;
            }
            int reformatted_date_given = check_reformat_date(in2);
            if (reformatted_date_given == -1)
            {
                printf("Error: Invalid date.\n");
            }

            Virus_table virus_table;
            int i;
            //Check for virus if it exists in table_of_viruses.
            for (i = 0; i < num_of_viruses; i++)
            {
                //Found virus name, break and we have the pointer to the virus table we want.
                if (strcmp(get_virus_name(table_of_viruses[i]),in5)==0)
                {
                    virus_table = table_of_viruses[i];
                    break;
                }
            }
            if (i==num_of_viruses) //Did not find virus name.
            {
                //Didn't find virus table so create a new one.
                virus_table = create_virus_table(in5,5);
                table_of_viruses[i] = virus_table;
                num_of_viruses++;
                // reallocate space for the next pointer in table of viruses.
                table_of_viruses = realloc(table_of_viruses,(num_of_viruses+2)*sizeof(Virus_table));
                if (table_of_viruses==NULL)
                {
                    perror("Error in malloc");
                    exit(1);
                }
            }
            int write_fd = get_value_list(country_write_fd_list,in3);
            int read_fd = get_value_list(country_read_fd_list,in3);
            if ((write_fd == -1) || (read_fd == -1))
            {
                printf("WRONG INPUT! Country does not exist. Try again.\n");
                continue;
            }
            
            Stat_record rec = malloc(sizeof(struct stat_record));
            if (rec==NULL)
            {
                perror("Error in malloc");
            }
            
            rec->country_name = malloc(strlen(in4)+1);
            if (rec->country_name==NULL)
            {
                perror("Error in malloc");
            }
            rec->date = malloc(strlen(in2)+1);
            if (rec->date==NULL)
            {
                perror("Error in malloc");
            }
            strcpy(rec->country_name,in4);
            strcpy(rec->date,in2);

            Bloom_filter bloom_filter = get_bloom_filter(in5); //unknown virus - not vaccinated
            if (bloom_filter == NULL)
            {
                printf("Virus not found.\n");
                printf("REQUEST REJECTED – YOU ARE NOT VACCINATED\n");
                total_rejected_requests++;
                rec->approved='n';
                insert_virus_table(virus_table,rec);
                continue;
            }
            else //virus found
            {
                int check = check_bloom_filter(bloom_filter,in1); //id not found - not vaccinated
                if (check == 0)
                {
                    printf("Citizen id not found for this virus.\n");
                    printf("REQUEST REJECTED – YOU ARE NOT VACCINATED\n");
                    total_rejected_requests++;
                    rec->approved='n';
                    insert_virus_table(virus_table,rec);
                    continue;
                }
                else //order the child to check for virus
                {
                
                    int command_length = strlen(command);
                    write(write_fd,&command_length,sizeof(int));
                    string_pipe("write",write_fd,command,command_length,bufferSize);

                    //Send virus name.
                    int virus_length = strlen(in5);
                    write(write_fd,&virus_length,sizeof(int));
                    string_pipe("write",write_fd,in5,virus_length,bufferSize);
                    
                    //Send key.
                    int key_length = strlen(in1);
                    write(write_fd,&key_length,sizeof(int));
                    string_pipe("write",write_fd,in1,key_length,bufferSize);

                    //Get a 'key found-value': 1 = 'key found' and 0 = 'key not found'.
                    int found;
                    read(read_fd,&found,sizeof(int));

                    //Get answer and if 'YES' get a date.
                    char* answer;
                    char* date_vaccinated = malloc(SIZE_OF_DATE+1);
                    date_vaccinated[0]='\0';
                    if (found == 1) //expect YES and date of vaccination
                    {
                        answer = malloc(strlen("YES")+1);
                        answer[0]='\0';
                        string_pipe("read",read_fd,answer,sizeof("YES"),bufferSize);
                        string_pipe("read",read_fd,date_vaccinated,SIZE_OF_DATE,bufferSize);
                    }
                    else if (found == 0) //expect NO
                    {
                        answer = malloc(strlen("NO")+1);
                        string_pipe("read",read_fd,answer,sizeof("NO"),bufferSize);
                        date_vaccinated=NULL;
                    }
                    if (strcmp(answer,"YES")==0)
                    {
                        int days=reformatted_date_given-check_reformat_date(date_vaccinated);
                        int vacc_limit;
                        if ((in2[4]>='6')||(in2[3]=='1'))
                        {
                            if (date_vaccinated[3]=='1' && date_vaccinated[4]=='2')
                            {
                                vacc_limit=9400;
                            }
                            else
                            {
                                vacc_limit=600;
                            }   
                        }
                        else if(in2[4]<'6')
                        {
                            vacc_limit=9400;
                        }
                        if (days>vacc_limit) //if citizen last vaccinated before 6 months of the travel date
                        {
                            printf("REQUEST REJECTED – YOU WILL NEED ANOTHER VACCINATION BEFORE TRAVEL DATE\n");
                            total_rejected_requests++;
                            rec->approved='n';
                            insert_virus_table(virus_table,rec);
                        }
                        else
                        {
                            printf("REQUEST ACCEPTED – HAPPY TRAVELS\n");
                            total_accepted_requests++;
                            rec->approved='y';
                            insert_virus_table(virus_table,rec);
                        }
                        free(answer);
                        free(date_vaccinated);
                    }
                    else if (strcmp(answer,"NO")==0)
                    {
                        printf("REQUEST REJECTED – YOU ARE NOT VACCINATED\n");
                        rec->approved='n';
                        insert_virus_table(virus_table,rec);
                        free(answer);
                        free(date_vaccinated);
                    }
                }
            }
            //Let the monitor know if request to it's country has been approved/rejected.
            write(write_fd,&rec->approved,sizeof(char));
            continue;
        }

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////    /travelStats    /////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

        else if (strcmp(command,"/travelStats")==0)
        {
            in1 = strtok(NULL," "); //virusName
            in2 = strtok(NULL," "); //date1
            in3 = strtok(NULL," "); //date2
            in4 = strtok(NULL," "); //[country]
            in5 = strtok(NULL," "); //NULL
            if ((in1==NULL)||(in2==NULL)||(in3==NULL)||(in5!=NULL))
            {
                printf("WRONG INPUT! Try again as : /travelStats virusName date1 date2 [country]\n");
                continue;
            }
            int reformatted_date1 = check_reformat_date(in2);
            int reformatted_date2 = check_reformat_date(in3);
            if ((reformatted_date1 == -1)||(reformatted_date2 == -1))
            {
                printf("Error: Invalid date.\n");
            }
            
            Bloom_filter bloom_filter = get_bloom_filter(in1);
            if (bloom_filter == NULL)
            {
                printf("WRONG INPUT! Virus doesn't exist.\n");
                continue;
            }
            else //virus found
            {
                Virus_table virus_table;
                //Get virus table with request stats.
                //Check for virus if it exists in table_of_viruses.
                for (i = 0; i < num_of_viruses; i++)
                {
                    //Found virus name, break and we have the pointer to the virus table we want.
                    if (strcmp(get_virus_name(table_of_viruses[i]),in1)==0)
                    {
                        virus_table = table_of_viruses[i];
                        break;
                    }
                }
                if (i==num_of_viruses) //Did not find virus in table of viruses.
                {
                    printf("Travel Statistics:\n"
                    "\tTOTAL REQUESTS %d\n"
                    "\tACCEPTED %d\n"
                    "\tREJECTED %d\n",0,0,0);
                    continue;
                }
                if (in4!=NULL) //country is given
                {
                    count_stats_country(virus_table,in2,in3,in4);
                }
                else if (in4==NULL) ////no country is given
                {
                    count_stats(virus_table,in2,in3);
                }
                continue;
            }
        }

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////    /addVaccinationRecords    ////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
        
        else if (strcmp(command,"/addVaccinationRecords")==0)
        {
            in1 = strtok(NULL," "); //country
            in2 = strtok(NULL," "); //NULL
            if ((in1==NULL)||(in2!=NULL))
            {
                printf("WRONG INPUT! Try again as : /addVaccinationRecords country\n");
                continue;
            }
            int pid = get_value_list(country_pid_list,in1);
            kill(pid,SIGUSR1);

            //Now read from child the new/old bloom filters.
            int read_fd = get_value_list(country_read_fd_list,in1);
            //Get the number of bloom filters of the monitor.
            int number_bloom_filters;
            read(read_fd,&number_bloom_filters,sizeof(int));
            for (int j = 0; j < number_bloom_filters; j++)
            {
                int virus_name_length;
                //Get virus name from monitor.
                read(read_fd,&virus_name_length,sizeof(int));
                char* virus_name = malloc(sizeof(char)*virus_name_length + 1);
                virus_name[0] = '\0';
                string_pipe("read",read_fd,virus_name,virus_name_length,bufferSize);

                //Get bloom filter's array from monitor.
                char* bf_array = malloc(sizeOfBloom);
                char* tempptr;
                if (bf_array==NULL)
                {
                    perror("Error in malloc");
                    exit(1);
                }
                for(int k=0; k<sizeOfBloom; k++) 
                {
                    bf_array[k]=bf_array[k]&0;
                }
                Bloom_filter bloom_filter = insert_filter_list(virus_name);
                int buff_times=sizeOfBloom/bufferSize; //these many times we read bytes equal to buffersize
                int remaining_bytes=sizeOfBloom%bufferSize; //bytes left, less than buffersize
                int buff_times_copy = buff_times;
                while (buff_times_copy>0)
                {
                    tempptr = bf_array + (buff_times-buff_times_copy) * bufferSize;
                    read(read_fd,tempptr,bufferSize);
                    buff_times_copy--;
                }  
                if (remaining_bytes>0)
                {
                    tempptr = bf_array + (buff_times-buff_times_copy) * bufferSize;
                    read(read_fd,tempptr,remaining_bytes);
                }
                merge_bloom_arrays(&bloom_filter,bf_array);
                free(virus_name);
                free(bf_array);
            }

            continue;
        }

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////    /searchVaccinationStatus    ///////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

        else if (strcmp(command,"/searchVaccinationStatus")==0)
        {
            in1 = strtok(NULL," "); //id
            in2 = strtok(NULL," "); //NULL
            if ((in1==NULL)||(in2!=NULL))
            {
                printf("WRONG INPUT! Try again as : /searchVaccinationStatus citizenID\n");
                continue;
            }
            int command_length = strlen(command);
            for (int i = 0; i < numMonitors; i++)
            {
                //Send command.
                write(write_fds[i],&command_length,sizeof(int));
                string_pipe("write",write_fds[i],command,command_length,bufferSize);
            }
            for (int i = 0; i < numMonitors; i++)
            {
                //Send key.
                int key_length = strlen(in1);
                write(write_fds[i],&key_length,sizeof(int));
                string_pipe("write",write_fds[i],in1,key_length,bufferSize);
            }

            int found; //we'll get 'numMonitor' founds that can be 1 or 0
            int temp_fds[numMonitors];
            for (int i = 0; i < numMonitors; i++) //temp_fds will have all fds at first
            {
                temp_fds[i]=read_fds[i];
            }
            for (int i = 0; i < numMonitors; i++)
            {
                read(read_fds[i],&found,sizeof(int));

                if (found==0) //when a monitor gives found : 0, make the corresponding fd 0
                {
                    temp_fds[i]=0;
                } 
            }
            int read_fd=0;
            for (int i = 0; i < numMonitors; i++)
            {
                //The fd we'll use to read from the monitor that has the key, is the one that is not 0.
                if (temp_fds[i]!=0) 
                {
                    read_fd=temp_fds[i]; //assign it to read_fd
                }   
            }
            if (read_fd==0) //If read_fd didn't change then no monitor has the key
            {
                printf("Error: Citizen doesn't exist. Try again.\n");
                continue;
            }
            
            //Receive the sizes of the attributes we are about to have.
            int fname_length,lname_length,country_length;
            read(read_fd,&fname_length,sizeof(int));
            read(read_fd,&lname_length,sizeof(int));
            read(read_fd,&country_length,sizeof(int));

            //Receive the attributes.
            char* first_name = malloc(fname_length+1);
            first_name[0] = '\0';
            char* last_name = malloc(lname_length+1);
            last_name[0] = '\0';
            char* country = malloc(country_length+1);
            country[0] = '\0';
            int age;
            string_pipe("read",read_fd,first_name,fname_length,bufferSize);
            string_pipe("read",read_fd,last_name,lname_length,bufferSize);
            string_pipe("read",read_fd,country,country_length,bufferSize);
            read(read_fd,&age,sizeof(int));

            printf("%s %s %s %s\n",in1,first_name,last_name,country);
            printf("AGE %d\n",age);

            free(first_name);
            free(last_name);
            free(country);

            //Receive the number of the virus names the key assosiates with.
            int number_of_viruses;
            read(read_fd,&number_of_viruses,sizeof(int));

            //Receive and print virus names - status - dates
            int virus_length,status_length;
            
            for (int i = 0; i < number_of_viruses; i++)
            {
                int virus_length;
                read(read_fd,&virus_length,sizeof(int));
                char* virus_name = malloc(virus_length+1);
                virus_name[0]='\0';
                string_pipe("read",read_fd,virus_name,virus_length,bufferSize);
                int status_length;
                read(read_fd,&status_length,sizeof(int));
                char* status = malloc(status_length+1);
                status[0]='\0';
                string_pipe("read",read_fd,status,status_length,bufferSize);
                if (strcmp(status,"YES")==0) //expect a date
                {
                    char date[SIZE_OF_DATE];
                    date[0]='\0';
                    string_pipe("read",read_fd,date,SIZE_OF_DATE,bufferSize);
                    printf("%s VACCINATED ON %s\n",virus_name,date);
                }
                else
                {
                    printf("%s NOT YET VACCINATED\n",virus_name);
                }
                free(virus_name);
                free(status);
            }
            continue;
        }

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////    /exit    /////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////

        else if (strcmp(command,"/exit")==0)
        {
            in1 = strtok(NULL," ");
            if (in1!=NULL)
            {
                printf("WRONG INPUT! Try again as : /exit.\n");
                continue;
            }
            break;
        }

        else if (strcmp(command,"/info")==0)
        {
            printf("Available commands:\n"
            "\t1) /travelRequest citizenID date countryFrom countryTo virusName\n"
            "\t2) /addVaccinationRecords country\n"
            "\t3) /vaccineStatus citizenID\n"
            "\t4) /searchVaccinationStatus citizenID\n"
            "\t5) /exit\n");
        }

        else
        {
            printf("WRONG INPUT! Command does not exist. Try /info to see a list of commands.\n");
            continue;
        }
    }while(1);

    //Kill all monitors.
    for (int i = 0; i < numMonitors; i++)
    {
        kill(pids[i],SIGKILL);
    }

    pid = getpid();
    char logfile[30];
    sprintf(logfile,"./log_files/log_file.%d",pid);
    int log_fd = open(logfile,O_CREAT | O_WRONLY,0666);
    int std_out = dup(1); //save stdout
    int error = dup2(log_fd,1); //redirect stdout to log file from now on
    if (error==-1)
    {
        perror("Error dup2 \n");
    }
    for (int i = 0; i < num_countries; i++)
    {
        printf("%s\n",countries[i]);
    }
    printf("TOTAL TRAVEL REQUESTS %d\n",total_accepted_requests+total_rejected_requests);
    printf("ACCEPTED %d\n",total_accepted_requests);
    printf("REJECTED %d\n",total_rejected_requests);
    dup2(std_out,1); //back to stdout
    printf("\n<TravelMonitor's log file created>\n");
    
    for (int i = 0; i < numMonitors; i++)
    {
        close(write_fds[i]);
        close(read_fds[i]);
    }

    printf("<Unlink pipes...>\n");
    for (int i = 0; i < numMonitors; i++)
    {
        unlink(read_fifos[i]);
        unlink(write_fifos[i]);
    }

    printf("<Freeing allocated memory...>\n");
    for (int i = 0; i < num_countries; i++)
    {
        free(countries[i]);
    }
    free(countries);
    free(input_dir);
    delete_list(country_write_fd_list);
    delete_list(country_read_fd_list);
    delete_list(country_pid_list);
    for (int i = 0; i < num_of_viruses; i++)
    {
        Virus_table virus_table;
        virus_table = table_of_viruses[i];
        delete_virus_table(virus_table);
    }
    free(table_of_viruses);
    delete_filter_list();
    printf("<Exiting...>\n");
    exit(0);
}
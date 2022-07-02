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
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <time.h>
#include "various_functions.h"
#include "filter_list.h"
#include "list.h"
#include "virus_table.h"

#define SIZE_OF_DATE 10 //dd-mm-yyyy
#define PORT_START 50000
#define PORT_END 60000

int main(int argc,char *argv[])
{
    printf("<CLIENT %d : Starting>\n",getpid());

    if ((argc!=13)||(argv[1][0]!='-')||(argv[3][0]!='-')||(argv[5][0]!='-')||(argv[7][0]!='-')||(argv[9][0]!='-')||(argv[11][0]!='-')) //must have flags
    {
        printf("Error in arguments. Execute program as: ./travelMonitorClient -m numMonitors -b socketBufferSize -c cyclicBufferSize -s sizeOfBloom -i input_dir -t numThreads\n\n");
        return (-1);
    } 
    if ((argv[1][1]!='m')||(argv[3][1]!='b')||(argv[5][1]!='c')||(argv[7][1]!='s')||(argv[9][1]!='i')||(argv[11][1]!='t')) //checking the flags
    {
        printf("Error in arguments. Execute program as: ./travelMonitorClient -m numMonitors -b socketBufferSize -c cyclicBufferSize -s sizeOfBloom -i input_dir -t numThreads\n\n");
        return (-1);
    }
    if ((argv[1][2]!='\0')||(argv[3][2]!='\0')||(argv[5][2]!='\0')||(argv[7][2]!='\0')||(argv[9][2]!='\0')||(argv[11][2]!='\0'))
    {
        printf("Error in arguments. Execute program as: ./travelMonitorClient -m numMonitors -b socketBufferSize -c cyclicBufferSize -s sizeOfBloom -i input_dir -t numThreads\n\n");
        return (-1);
    }
    int numMonitors = atoi(argv[2]);
    int socketBufferSize = atoi(argv[4]);
    int cyclicBufferSize = atoi(argv[6]);
    int sizeOfBloom = atoi(argv[8]);
    int numThreads = atoi(argv[12]);
    if ((numMonitors<1)||(socketBufferSize<1)||(cyclicBufferSize<1)||(sizeOfBloom<1)||(numThreads<1)) //checking given arguments
    {
        printf("Error in arguments. NumMonitors - socketBufferSize - cyclicBufferSize - sizeOfBloom - numThreads must be integers greater than 0.\n\n");
        return (-1);
    }
    char* input_dir = malloc(strlen("../../")+strlen(argv[10])+1);
    if (input_dir==NULL)
    {
        perror("Error in malloc");
        exit(1);
    }
    strcpy(input_dir,"../../");
    strcat(input_dir,argv[10]);
    DIR *dir = opendir(input_dir); //checking if given directory exists
    if(dir == NULL)
    {
        perror("Error in directory");
        exit(1);
    }

    int total_rejected_requests=0;
    int total_accepted_requests=0;

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


    char** countries = malloc(num_countries * sizeof(char*)); //keep all countries' names.
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

    //Save the number of countries each child will have.
    int countries_per_child[numMonitors];
    for (int i = 0; i < numMonitors; i++)
    {
        countries_per_child[i]=0;
    }
    for (int i = 0; i < num_countries; i++)
    {
        countries_per_child[i%numMonitors]++;
    }

    //Save the ports that will be assigned to monitor-servers.
    srand(time(NULL));
    int ports[numMonitors];
    for (int i = 0; i < numMonitors; i++)
    {
        int port = (rand() % PORT_END) + PORT_START;
        ports[i]=port;
    }
    
    
    //Forking numMonitors children.
    int pids[numMonitors]; //keep pids of children
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
        }
        if (pid==0) //Child process uses Monitor
        {
            
            char** exec_argv = malloc((7 + countries_per_child[i]) * sizeof(char*));
            exec_argv[0] = malloc(strlen("monitorServer")+1); //program name
            strcpy(exec_argv[0],"monitorServer");

            char buf[9];
            sprintf(buf,"%d",ports[i]); //port number
            exec_argv[1] = malloc(9);
            strcpy(exec_argv[1],buf);

            sprintf(buf,"%d",numThreads); //numThreads
            exec_argv[2] = malloc(9);
            strcpy(exec_argv[2],buf);

            sprintf(buf,"%d",socketBufferSize); //socketBufferSize
            exec_argv[3] = malloc(9);
            strcpy(exec_argv[3],buf);

            sprintf(buf,"%d",cyclicBufferSize); //cyclicBufferSize
            exec_argv[4] = malloc(9);
            strcpy(exec_argv[4],buf);

            sprintf(buf,"%d",sizeOfBloom); //sizeOfBloom
            exec_argv[5] = malloc(9);
            strcpy(exec_argv[5],buf);
            
            int pos=6;
            for (int j = 0; j < num_countries; j++)
            {
                if (j%numMonitors==i)
                {
                    exec_argv[pos] = malloc(strlen(input_dir) + 1 + strlen(countries[j]) + 1);
                    strcpy(exec_argv[pos],input_dir);
                    strcat(exec_argv[pos],"/");
                    strcat(exec_argv[pos],countries[j]);
                    pos++;
                }  
            }
            exec_argv[pos]=NULL;
            
            //monitorServer -p port -t numThreads -b socketBufferSize -c cyclicBufferSize -s sizeOfBloom path1 path2 ... pathn
            int error = execv("../monitorServer/monitorServer",exec_argv);
            if (error == -1)
            {
                printf("Error in execv\n");
                exit(1);
            }
        }
    }

    //Children used exec so we are in parent process:

    struct sockaddr_in servers[numMonitors];
    struct sockaddr *serverptr;
    struct hostent *rem;
    
    //Create 'numMonitor' sockets.
    int sockets[numMonitors];
    for (int i = 0; i < numMonitors; i++)
    {
        int sock = socket(PF_INET, SOCK_STREAM, 0);
        if (sock < 0)
        {
            perror("Error in socket\n");
            exit(1);
        }
        sockets[i] = sock;
    }

    //Map countries to sockets.
    List country_socket_list = create_list();
    for (int i = 0; i < numMonitors; i++)
    {
        for (int j = 0; j < num_countries; j++)
        {
            if (j%numMonitors==i)
            {
                char *str_soc = malloc(9);
                sprintf(str_soc,"%d",sockets[i]);
                insert_list(country_socket_list,countries[j],str_soc);
                free(str_soc);
            }  
        }
    }

    /* Find server address */
    char host_name[100];
    gethostname(host_name,sizeof(host_name));
    rem = gethostbyname(host_name);
    if (rem == NULL)
    {
        printf("Error in gethostbyname");
    }
    for (int i = 0; i < numMonitors; i++)
    {
        servers[i].sin_family = AF_INET;
        memcpy(&(servers[i]).sin_addr, rem->h_addr, rem->h_length);
        servers[i].sin_port = htons(ports[i]);
    }

    int connect_error;
    for (int i = 0; i < numMonitors; i++)
    {
        do
        {
            serverptr = (struct sockaddr *) &servers[i];
            connect_error = connect(sockets[i], serverptr, sizeof(servers[i]));
        }
        while (connect_error < 0);
    }


    //sleep(0);

/////////////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////                                              /////////////////////////////////
//////////////////////////////        GET BLOOM FILTERS FROM SERVERS        /////////////////////////////////
//////////////////////////////                                              /////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////////////

    

    create_filter_list(sizeOfBloom); //create a list to hold bloom filters
    for (int i = 0; i < numMonitors; i++) //loop to the number of servers
    {
        //Get the number of bloom filters of monitor.
        int number_bloom_filters;
        read(sockets[i],&number_bloom_filters,sizeof(int));
        for (int j = 0; j < number_bloom_filters; j++) //for every server loop to the number of bloom filters it has
        {
            
            //Get virus' name-length and name from server.
            int virus_name_length;
            read(sockets[i],&virus_name_length,sizeof(int));            
            char* virus_name = malloc(sizeof(char)*virus_name_length + 1);
            virus_name[0] = '\0';
            string_pipe("read",sockets[i],virus_name,virus_name_length,socketBufferSize);
            
            //Get bloom filter's array from monitor.
            char* bf_array = malloc(sizeOfBloom);
            bf_array[0]='\0';
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

            char* ptr_buf;
            int buff_times=sizeOfBloom/socketBufferSize; //these many times we read bytes equal to buffersize
            int remaining_bytes=sizeOfBloom%socketBufferSize; //bytes left, less than buffersize
            int buff_times_copy = buff_times;
            while (buff_times_copy>0)
            {
                ptr_buf = bf_array + (buff_times-buff_times_copy) * socketBufferSize;
                int received, n;
                for(received = 0; received < socketBufferSize; received+=n)
                {
                    if ((n = read(sockets[i], ptr_buf+received, socketBufferSize-received)) == -1)
                    return -1; /* error */
                }
                buff_times_copy--;
            }  
            if (remaining_bytes>0)
            {
                ptr_buf = bf_array + ((buff_times-buff_times_copy) * socketBufferSize);
                int received, n;
                for(received = 0; received < remaining_bytes; received+=n)
                {
                    if ((n = read(sockets[i], ptr_buf+received, remaining_bytes-received)) == -1)
                    return -1;
                }
            }
            merge_bloom_arrays(&bloom_filter,bf_array);
            free(virus_name);
            free(bf_array);
        }
    }
    //print_filter_list(); //print all viruses we got




/////////////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////                                          /////////////////////////////////////
//////////////////////////////        RECEIVE COMMANDS FROM USER        /////////////////////////////////////
//////////////////////////////                                          /////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////////////




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
            int socket = get_value_list(country_socket_list,in3);
            if (socket == -1)
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
                    write(socket,&command_length,sizeof(int));
                    string_pipe("write",socket,command,command_length,socketBufferSize);

                    //Send virus name.
                    int virus_length = strlen(in5);
                    write(socket,&virus_length,sizeof(int));
                    string_pipe("write",socket,in5,virus_length,socketBufferSize);
                    
                    //Send key.
                    int key_length = strlen(in1);
                    write(socket,&key_length,sizeof(int));
                    string_pipe("write",socket,in1,key_length,socketBufferSize);

                    //Get a 'key found-value': 1 = 'key found' and 0 = 'key not found'.
                    int found;
                    read(socket,&found,sizeof(int));

                    //Get answer and if 'YES' get a date.
                    char* answer;
                    char* date_vaccinated = malloc(SIZE_OF_DATE+1);
                    date_vaccinated[0]='\0';
                    if (found == 1) //expect YES and date of vaccination
                    {
                        answer = malloc(strlen("YES")+1);
                        answer[0]='\0';
                        string_pipe("read",socket,answer,sizeof("YES"),socketBufferSize);
                        string_pipe("read",socket,date_vaccinated,SIZE_OF_DATE,socketBufferSize);
                    }
                    else if (found == 0) //expect NO
                    {
                        answer = malloc(strlen("NO")+1);
                        string_pipe("read",socket,answer,sizeof("NO"),socketBufferSize);
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
            write(socket,&rec->approved,sizeof(char));
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

            int socket = get_value_list(country_socket_list,in1); //get appropriate socket for this country' monitor
            
            //Give command to monitor-server.
            int command_length = strlen(command);
            write(socket,&command_length,sizeof(int));
            string_pipe("write",socket,command,command_length,socketBufferSize);

            //Turn country to path-country
            char* path_country = malloc(strlen(input_dir) + 1 + strlen(in1) + 1);
            strcpy(path_country,input_dir);
            strcat(path_country,"/");
            strcat(path_country,in1);

            //Give path-country to monitor-server.
            int path_country_length = strlen(path_country);
            write(socket,&path_country_length,sizeof(int));
            string_pipe("write",socket,path_country,path_country_length,socketBufferSize);
            free(path_country);
            
            //If added is 0 that means no new txt-files have been added. If 1 the opposite.
            int added;
            read(socket,&added,sizeof(int));

            if (added==0)
            {
                printf("No new files added.\n");
                continue;
            }
            else if (added==1)
            {
                //Get the number of bloom filters of monitor.
                int number_bloom_filters;
                read(socket,&number_bloom_filters,sizeof(int));
                for (int j = 0; j < number_bloom_filters; j++) //for every server loop to the number of bloom filters it has
                {
                    
                    //Get virus' name-length and name from server.
                    int virus_name_length;
                    read(socket,&virus_name_length,sizeof(int));
                    char* virus_name = malloc(sizeof(char)*virus_name_length + 1);
                    virus_name[0] = '\0';
                    string_pipe("read",socket,virus_name,virus_name_length,socketBufferSize);
                    
                    //Get bloom filter's array from monitor.
                    char* bf_array = malloc(sizeOfBloom);
                    bf_array[0]='\0';
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

                    char* ptr_buf;
                    int buff_times=sizeOfBloom/socketBufferSize; //these many times we read bytes equal to buffersize
                    int remaining_bytes=sizeOfBloom%socketBufferSize; //bytes left, less than buffersize
                    int buff_times_copy = buff_times;
                    while (buff_times_copy>0)
                    {
                        ptr_buf = bf_array + (buff_times-buff_times_copy) * socketBufferSize;
                        int received, n;
                        for(received = 0; received < socketBufferSize; received+=n)
                        {
                            if ((n = read(socket, ptr_buf+received, socketBufferSize-received)) == -1)
                            return -1; /* error */
                        }
                        buff_times_copy--;
                    }  
                    if (remaining_bytes>0)
                    {
                        ptr_buf = bf_array + ((buff_times-buff_times_copy) * socketBufferSize);
                        int received, n;
                        for(received = 0; received < remaining_bytes; received+=n)
                        {
                            if ((n = read(socket, ptr_buf+received, remaining_bytes-received)) == -1)
                            return -1;
                        }
                    }

                    merge_bloom_arrays(&bloom_filter,bf_array);
                    free(virus_name);
                    free(bf_array);
                }
            }
        }



// //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ///////////////////////////////////////////    /searchVaccinationStatus    ///////////////////////////////////////////////
// //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////




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
                
                write(sockets[i],&command_length,sizeof(int));
                string_pipe("write",sockets[i],command,command_length,socketBufferSize);
            }
            for (int i = 0; i < numMonitors; i++)
            {
                //Send key.
                int key_length = strlen(in1);
                write(sockets[i],&key_length,sizeof(int));
                string_pipe("write",sockets[i],in1,key_length,socketBufferSize);
            }

            int found=0; //we'll get 'numMonitor' founds that can be 1 or 0
            int temp_fds[numMonitors];
            for (int i = 0; i < numMonitors; i++)
            {
                temp_fds[i]=sockets[i];
            }
            for (int i = 0; i < numMonitors; i++)
            {
                read(sockets[i],&found,sizeof(int));

                if (found==0) //when a monitor gives found : 0, make the corresponding fd 0
                {
                    temp_fds[i]=0;
                } 
            }
            int socket=0;
            for (int i = 0; i < numMonitors; i++)
            {
                //The socket we'll use to read from the monitor that has the key, is the one that is not 0.
                if (temp_fds[i]!=0) 
                {
                    socket=temp_fds[i]; //assign it to read_fd
                }   
            }
            if (socket==0) //If socket didn't change then no monitor has the key
            {
                printf("Error: Citizen doesn't exist. Try again.\n");
                continue;
            }
            
            //Receive the sizes of the attributes we are about to have.
            int fname_length,lname_length,country_length;
            read(socket,&fname_length,sizeof(int));
            read(socket,&lname_length,sizeof(int));
            read(socket,&country_length,sizeof(int));

            //Receive the attributes.
            char* first_name = malloc(fname_length+1);
            first_name[0] = '\0';
            char* last_name = malloc(lname_length+1);
            last_name[0] = '\0';
            char* country = malloc(country_length+1);
            country[0] = '\0';
            int age;
            string_pipe("read",socket,first_name,fname_length,socketBufferSize);
            string_pipe("read",socket,last_name,lname_length,socketBufferSize);
            string_pipe("read",socket,country,country_length,socketBufferSize);
            read(socket,&age,sizeof(int));

            printf("%s %s %s %s\n",in1,first_name,last_name,country);
            printf("AGE %d\n",age);

            free(first_name);
            free(last_name);
            free(country);

            //Receive the number of the virus names the key assosiates with.
            int number_of_viruses;
            read(socket,&number_of_viruses,sizeof(int));

            //Receive and print virus names - status - dates
            int virus_length,status_length;
            
            for (int i = 0; i < number_of_viruses; i++)
            {
                int virus_length;
                read(socket,&virus_length,sizeof(int));
                char* virus_name = malloc(virus_length+1);
                virus_name[0]='\0';
                string_pipe("read",socket,virus_name,virus_length,socketBufferSize);
                int status_length;
                read(socket,&status_length,sizeof(int));
                char* status = malloc(status_length+1);
                status[0]='\0';
                string_pipe("read",socket,status,status_length,socketBufferSize);
                if (strcmp(status,"YES")==0) //expect a date
                {
                    char date[SIZE_OF_DATE];
                    date[0]='\0';
                    string_pipe("read",socket,date,SIZE_OF_DATE,socketBufferSize);
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
            //Inform monitor servers that they are about to exit.
            int command_length = strlen(command);
            for (int i = 0; i < numMonitors; i++)
            {
                //Send command.
                write(sockets[i],&command_length,sizeof(int));
                string_pipe("write",sockets[i],command,command_length,socketBufferSize);
            }

            printf("<CLIENT %d : Waiting servers to exit>\n",getpid());
            pid_t wpid;
            while ((wpid = wait(NULL)) > 0);


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

            printf("<CLIENT %d : Frees stuff and exits>\n",getpid());
            for (int i = 0; i < num_countries; i++)
            {
                free(countries[i]);
            }
            free(countries);
            free(input_dir);
            delete_list(country_socket_list);

            for (int i = 0; i < num_of_viruses; i++)
            {
                Virus_table virus_table;
                virus_table = table_of_viruses[i];
                delete_virus_table(virus_table);
            }
            free(table_of_viruses);
            delete_filter_list();
            for (int i = 0; i < numMonitors; i++)
            {
                close(sockets[i]);
            }
            exit(0);
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
}
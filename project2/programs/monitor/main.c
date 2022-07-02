#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <dirent.h>
#include <signal.h>
#include <errno.h>
#include "virus_list.h"
#include "filter_list.h"
#include "various_functions.h"
#include "country_list.h"


#define ESTIMATED_LINES 5 //for hash table of records and skip lists
#define SIZE_OF_DATE 10 //dd-mm-yyyy

static int sigint_flag=0;
static int sigusr1_flag=0;

static void iq_handler (int signo) //handle SIGINT and SIGQUIT
{
    sigint_flag=1;
}

static void u_handler (int signo) //handle SIGUSR1
{
    sigusr1_flag=1;
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

    //Establishing SIGUSR1 handler.
    static struct sigaction u_act; //handle SIGUSR1
	u_act.sa_handler=u_handler;
	sigfillset(&(u_act.sa_mask)); //other signals will be blocked when handling
    u_act.sa_flags = 0;
	sigaction(SIGUSR1, &u_act, NULL);

    printf("<Monitor (pid : %d) starting>\n",getpid());

    //Monitor will keep track of travel requests (for log file).
    int rejected_requests=0;
    int accepted_requests=0;

    //Opening pipes.
    int write_fd;
    int read_fd;
    write_fd = open(argv[1], O_WRONLY);
    if (write_fd==-1)
    {
        perror("Error in opening fifo <write>");
        exit(1);
    }
    read_fd = open(argv[2], O_RDONLY);
    if (read_fd==-1)
    {
        perror("Error in opening fifo <read>");
        exit(1);
    }

    //Getting bufferSize from travelMonitor.
    int bufferSize;
    read(read_fd,&bufferSize,sizeof(int));

    //Getting size of bloom filters from travelMonitor.
    int sizeOfBloom;
    read(read_fd,&sizeOfBloom,sizeof(int));

    //Getting input_dir's length and name from travelMonitor.
    int input_dir_length;
    read(read_fd,&input_dir_length,sizeof(int));
    printf("LNEGTH = %d\n",input_dir_length);
    char* input_dir = malloc(sizeof(char)*input_dir_length + 1);
    input_dir[0] = '\0';
    int error;
    error = string_pipe("read",read_fd,input_dir,input_dir_length,bufferSize);
    if (error==-1)
    {
        exit(-1);
    }
    
    //Getting the number of countries that will be assigned from travelMonitor.
    int num_countries;
    read(read_fd,&num_countries,sizeof(int));

    //Getting the appropriate countries.
    char** countries = malloc(num_countries * sizeof(char*));
    int country_length;
    for (int i = 0; i < num_countries; i++)
    {
        read(read_fd,&country_length,sizeof(int));
        countries[i] = malloc(sizeof(char)*country_length + 1);
        countries[i][0] = '\0';
        error = string_pipe("read",read_fd,countries[i],country_length,bufferSize);
        if (error==-1)
        {
            exit(-1);
        }
    }

    //Open input_dir and get records from appropriate countries.
    DIR *dir,*sub_dir;
    FILE *fp;
    struct dirent *directory;
    struct dirent *txt;

    //Get and store current date (for error checking).
    char* current_date = malloc(11*sizeof(char));
    if (current_date==NULL)
    {
        perror("Error in malloc");
        exit(1);
    }
    current_date[0] = '\0';
    get_current_date(&current_date);
    //Create a list of pointers to bloom filters with size 'sizeOfBloom'.
    create_filter_list(sizeOfBloom);
    //Create two lists of pointers to skip lists,
    //one list contains vaccinated skip lists - the other not vaccinated.
    create_virus_list(get_levels(ESTIMATED_LINES));
    create_country_list();
    //Create hash table to store valid records.
    create_hash_table(get_buckets(ESTIMATED_LINES));

    for (int i = 0; i < num_countries; i++) //loop countries
    {
        dir = opendir(input_dir); //open input_dir
        if(dir == NULL)
        {
            perror("Error in directory");
            exit(1);
        }
        while ((directory = readdir(dir))) //loop countries inside input_dir
        {
            if(strcmp(directory->d_name,countries[i])==0) //if appropriate country found
            {
                
                // path to country will be : ../../input_dir/country
                char* country_path = malloc(strlen(input_dir)+1+strlen(directory->d_name)+1);
                strcpy(country_path,input_dir);
                strcat(country_path,"/");
                strcat(country_path,directory->d_name);
                sub_dir = opendir(country_path); //open that country
                
                if(sub_dir == NULL)
                {
                    perror("Error in directory");
                    exit(1);
                }
                while ((txt = readdir(sub_dir))) //loop files
                {
                    if((strcmp(txt->d_name,".")==0)||(strcmp(txt->d_name,"..")==0)) //skip ./ and ../
                    {
                        continue;
                    }
                    // path to txt will be : ../../input_dir/Country/Country-x.txt
                    char* txt_path = malloc(strlen(country_path)+1+strlen(txt->d_name)+1);
                    strcpy(txt_path,country_path);
                    strcat(txt_path,"/");
                    strcat(txt_path,txt->d_name);
                    fp = fopen(txt_path,"r");
                    if (fp==NULL)
                    {
                        perror("ERROR ");
                        return -1;
                    }
                    char *temp_id, *temp_fn, *temp_ln, *temp_country, *temp_age, *temp_virus, *temp_vacc_status, *temp_date;
                    char str_rec[100],temp_str[100];
                    
                    while (fgets(str_rec,sizeof(str_rec),fp))
                    {
                        str_rec[strcspn(str_rec, "\n")] = 0; //removing new line

                        strcpy(temp_str,str_rec); //break record into pieces
                        temp_id = strtok(str_rec," ");
                        temp_fn = strtok(NULL," ");
                        temp_ln = strtok(NULL," ");
                        temp_country = strtok(NULL," ");
                        temp_age = strtok(NULL," ");
                        temp_virus = strtok(NULL," ");
                        temp_vacc_status = strtok(NULL," ");
                        temp_date = strtok(NULL," ");

                        if (atoi(temp_age)<=0 || atoi(temp_age)>120) //valid age?
                        {
                            printf("ERROR <<WRONG-AGE>> IN RECORD: %s\n",temp_str);
                            continue;
                        }
                        if ((temp_date==NULL)&&(strcmp(temp_vacc_status,"YES")==0))
                        {
                            printf("ERROR <<YES-DATE>> IN RECORD: %s\n",temp_str);
                            continue;
                        }
                        if(temp_date!=NULL)
                        {
                            
                            if(strlen(temp_date)>11) //invalid date
                            {
                                printf("ERROR <<INVALID-DATE>> IN RECORD: %s\n",temp_str);
                                continue;
                            }
                            int check_date = check_reformat_date(temp_date);
                            if (check_date==-1) //invalid format
                            {
                                printf("ERROR <<INVALID-DATE>> IN RECORD: %s\n",temp_str);
                                continue;
                            }
                            if (check_date>check_reformat_date(current_date)) //future date
                            {
                                printf("ERROR <<CURRENT-DATE>> IN RECORD: %s\n",temp_str);
                                continue;
                            }
                            if (strcmp(temp_vacc_status,"NO")==0) //is 'NO' followed by a date?
                            {
                                printf("ERROR <<NO-DATE>> IN RECORD: %s\n",temp_str);
                                continue;
                            }
                        }

                        Citizen_record record = malloc(sizeof(struct citizen_record)); //will be freed in hash table
                        if (record==NULL)
                        {
                            perror("Error in malloc");
                            exit(1);
                        }
                        record->citizenID = malloc(strlen(temp_id)+1);
                        if (record->citizenID==NULL)
                        {
                            perror("Error in malloc");
                            exit(1);
                        }
                        record->firstName = malloc(strlen(temp_fn)+1);
                        if (record->firstName==NULL)
                        {
                            perror("Error in malloc");
                            exit(1);
                        }
                        record->lastName = malloc(strlen(temp_ln)+1);
                        if (record->lastName==NULL)
                        {
                            perror("Error in malloc");
                            exit(1);
                        }
                        record->country = malloc(strlen(temp_country)+1);
                        if (record->country==NULL)
                        {
                            perror("Error in malloc");
                            exit(1);
                        }
                        strcpy(record->citizenID,temp_id);
                        strcpy(record->firstName,temp_fn);
                        strcpy(record->lastName,temp_ln);
                        strcpy(record->country,temp_country);
                        record->age=atoi(temp_age);

                        int check = insert_hash_table(record);
                        if(check==1)
                        {
                            //check=1 : duplicate id - different name/surname/age/country - Error
                            free(record->citizenID);
                            free(record->firstName);
                            free(record->lastName);
                            free(record->country);
                            free(record);
                            printf("ERROR <<DUPLICATE-ID>> IN RECORD: %s\n",temp_str);
                            continue;
                        }
                        if (check==2)
                        {
                            //check=2 : duplicate record - need to check if given virus has same id or not.
                            //We will search both v_yes / v_no (virus lists) for the given id
                            int found = findid_virus_list(temp_virus,record->citizenID);
                            if (found==1) //found duplicate id in SAME VIRUS
                            {
                                free(record->citizenID);
                                free(record->firstName);
                                free(record->lastName);
                                free(record->country);
                                free(record);
                                printf("ERROR <<ID-VIRUS>> IN RECORD: %s\n",temp_str);
                                continue;
                            }
                            //DIFFERENT VIRUS : free duplicate record (no need to insert record in hash table)
                            //but don't skip the rest! (create virus normally)
                            free(record->citizenID);
                            free(record->firstName);
                            free(record->lastName);
                            free(record->country);
                            free(record);
                        }
                        
                        Skip_list skip_virus = insert_virus_list(temp_virus,temp_vacc_status);
                        insert_skip_list(skip_virus,temp_id,temp_date,temp_country);

                        if (strcmp(temp_vacc_status,"YES")==0)
                        {
                            Bloom_filter bloom_virus = insert_filter_list(temp_virus);
                            set_bloom_filter(bloom_virus,temp_id);
                        } 
                    }
                    fclose(fp);
                    free(txt_path);
                }
                free(country_path);
                closedir(sub_dir);
            }

        }
        closedir(dir);
    }


    //Let travelMonitor know the number of viruses/bloom filters it will get from monitor.
    int bloom_filters_number = num_bloom_filters();
    write(write_fd,&bloom_filters_number,sizeof(int));

    char* bf_array; //will point to the bloom array we are about to send
    char** bloom_virus_names = get_bloom_names();
    for (int i = 0; i < num_bloom_filters(); i++) //loop virus names of bloom filters
    {
        //Sent the name of the virus/bloom filter.
        int virus_name_length = strlen(bloom_virus_names[i]);
        write(write_fd,&virus_name_length,sizeof(int));
        error = string_pipe("write",write_fd,bloom_virus_names[i],virus_name_length,bufferSize);
        if (error==-1)
        {
            exit(-1);
        }

        //Send the bloom array.
        Bloom_filter bloom_filter = get_bloom_filter(bloom_virus_names[i]);
        bf_array = get_bloom_array(bloom_filter);
        error = string_pipe("write",write_fd,bf_array,sizeOfBloom,bufferSize);
        if (error==-1)
        {
            exit(-1);
        }
    }
    for (int i = 0; i < num_bloom_filters(); i++)
    {
        free(bloom_virus_names[i]);
    }
    free(bloom_virus_names);

    printf("Monitor %d ready for queries\n",getpid());




//////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////




    char input[100], temp_input[100];
    char *command, *in1, *in2, *in3, *in4, *in5, *in6, *in7, *in8, *in9;
    do
    {  
        //Read command to execute.
        int command_length;
        int bytes = read(read_fd,&command_length,sizeof(int));
        if (bytes==-1)
        {
            //Read return error, has a signal interrupt occured?
            if (errno==EINTR)
            {
                //Check for SIGINT/SIGQUIT
                if (sigint_flag==1)
                {
                    pid_t pid = getpid();
                    char logfile[30];
                    sprintf(logfile,"./log_files/log_file.%d",pid);
                    int log_fd = open(logfile,O_CREAT | O_WRONLY,0666);
                    int std_out = dup(1); //save stdout
                    int error = dup2(log_fd,1); //redirect stdout to log file from now on
                    if (error==-1)
                    {
                        perror("Error dup2 \n");
                    }
                    print_country_list();
                    printf("TOTAL TRAVEL REQUESTS %d\n",accepted_requests+rejected_requests);
                    printf("ACCEPTED %d\n",accepted_requests);
                    printf("REJECTED %d\n",rejected_requests);
                    dup2(std_out,1); //back to stdout
                    sigint_flag=0;
                    continue;
                }
                //Check for SIGUSR1
                if (sigusr1_flag==1)
                {
                    //Free every data structure and read again all files including any new if they exist.
                    delete_hash_table();
                    delete_virus_list();
                    delete_filter_list();
                    delete_country_list();

                    //Create a list of pointers to bloom filters with size 'sizeOfBloom'.
                    create_filter_list(sizeOfBloom);
                    //Create two lists of pointers to skip lists,
                    //one list contains vaccinated skip lists - the other not vaccinated.
                    create_virus_list(get_levels(ESTIMATED_LINES));
                    create_country_list();
                    //Create hash table to store valid records.
                    create_hash_table(get_buckets(ESTIMATED_LINES));

                    for (int i = 0; i < num_countries; i++) //loop countries
                    {
                        dir = opendir(input_dir); //open input_dir
                        if(dir == NULL)
                        {
                            perror("Error in directory");
                            exit(1);
                        }
                        while ((directory = readdir(dir))) //loop countries inside input_dir
                        {
                            if(strcmp(directory->d_name,countries[i])==0) //if appropriate country found
                            {
                                
                                // path to country will be : ../../input_dir/country
                                char* country_path = malloc(strlen(input_dir)+1+strlen(directory->d_name)+1);
                                strcpy(country_path,input_dir);
                                strcat(country_path,"/");
                                strcat(country_path,directory->d_name);
                                sub_dir = opendir(country_path); //open that country
                                
                                if(sub_dir == NULL)
                                {
                                    perror("Error in directory");
                                    exit(1);
                                }
                                while ((txt = readdir(sub_dir))) //loop files
                                {
                                    if((strcmp(txt->d_name,".")==0)||(strcmp(txt->d_name,"..")==0)) //skip ./ and ../
                                    {
                                        continue;
                                    }
                                    // path to txt will be : ../../input_dir/Country/Country-x.txt
                                    char* txt_path = malloc(strlen(country_path)+1+strlen(txt->d_name)+1);
                                    strcpy(txt_path,country_path);
                                    strcat(txt_path,"/");
                                    strcat(txt_path,txt->d_name);
                                    fp = fopen(txt_path,"r");
                                    if (fp==NULL)
                                    {
                                        perror("ERROR ");
                                        return -1;
                                    }
                                    char *temp_id, *temp_fn, *temp_ln, *temp_country, *temp_age, *temp_virus, *temp_vacc_status, *temp_date;
                                    char str_rec[100],temp_str[100];
                                    
                                    while (fgets(str_rec,sizeof(str_rec),fp))
                                    {
                                        str_rec[strcspn(str_rec, "\n")] = 0; //removing new line

                                        strcpy(temp_str,str_rec); //break record into pieces
                                        temp_id = strtok(str_rec," ");
                                        temp_fn = strtok(NULL," ");
                                        temp_ln = strtok(NULL," ");
                                        temp_country = strtok(NULL," ");
                                        temp_age = strtok(NULL," ");
                                        temp_virus = strtok(NULL," ");
                                        temp_vacc_status = strtok(NULL," ");
                                        temp_date = strtok(NULL," ");

                                        if (atoi(temp_age)<=0 || atoi(temp_age)>120) //valid age?
                                        {
                                            printf("ERROR <<WRONG-AGE>> IN RECORD: %s\n",temp_str);
                                            continue;
                                        }
                                        if ((temp_date==NULL)&&(strcmp(temp_vacc_status,"YES")==0))
                                        {
                                            printf("ERROR <<YES-DATE>> IN RECORD: %s\n",temp_str);
                                            continue;
                                        }
                                        if(temp_date!=NULL)
                                        {
                                            
                                            if(strlen(temp_date)>11) //invalid date
                                            {
                                                printf("ERROR <<INVALID-DATE>> IN RECORD: %s\n",temp_str);
                                                continue;
                                            }
                                            int check_date = check_reformat_date(temp_date);
                                            if (check_date==-1) //invalid format
                                            {
                                                printf("ERROR <<INVALID-DATE>> IN RECORD: %s\n",temp_str);
                                                continue;
                                            }
                                            if (check_date>check_reformat_date(current_date)) //future date
                                            {
                                                printf("ERROR <<CURRENT-DATE>> IN RECORD: %s\n",temp_str);
                                                continue;
                                            }
                                            if (strcmp(temp_vacc_status,"NO")==0) //is 'NO' followed by a date?
                                            {
                                                printf("ERROR <<NO-DATE>> IN RECORD: %s\n",temp_str);
                                                continue;
                                            }
                                        }

                                        Citizen_record record = malloc(sizeof(struct citizen_record)); //will be freed in hash table
                                        if (record==NULL)
                                        {
                                            perror("Error in malloc");
                                            exit(1);
                                        }
                                        record->citizenID = malloc(strlen(temp_id)+1);
                                        if (record->citizenID==NULL)
                                        {
                                            perror("Error in malloc");
                                            exit(1);
                                        }
                                        record->firstName = malloc(strlen(temp_fn)+1);
                                        if (record->firstName==NULL)
                                        {
                                            perror("Error in malloc");
                                            exit(1);
                                        }
                                        record->lastName = malloc(strlen(temp_ln)+1);
                                        if (record->lastName==NULL)
                                        {
                                            perror("Error in malloc");
                                            exit(1);
                                        }
                                        record->country = malloc(strlen(temp_country)+1);
                                        if (record->country==NULL)
                                        {
                                            perror("Error in malloc");
                                            exit(1);
                                        }
                                        strcpy(record->citizenID,temp_id);
                                        strcpy(record->firstName,temp_fn);
                                        strcpy(record->lastName,temp_ln);
                                        strcpy(record->country,temp_country);
                                        record->age=atoi(temp_age);

                                        int check = insert_hash_table(record);
                                        if(check==1)
                                        {
                                            //check=1 : duplicate id - different name/surname/age/country - Error
                                            free(record->citizenID);
                                            free(record->firstName);
                                            free(record->lastName);
                                            free(record->country);
                                            free(record);
                                            printf("ERROR <<DUPLICATE-ID>> IN RECORD: %s\n",temp_str);
                                            continue;
                                        }
                                        if (check==2)
                                        {
                                            //check=2 : duplicate record - need to check if given virus has same id or not.
                                            //We will search both v_yes / v_no (virus lists) for the given id
                                            int found = findid_virus_list(temp_virus,record->citizenID);
                                            if (found==1) //found duplicate id in SAME VIRUS
                                            {
                                                free(record->citizenID);
                                                free(record->firstName);
                                                free(record->lastName);
                                                free(record->country);
                                                free(record);
                                                printf("ERROR <<ID-VIRUS>> IN RECORD: %s\n",temp_str);
                                                continue;
                                            }
                                            //DIFFERENT VIRUS : free duplicate record (no need to insert record in hash table)
                                            //but don't skip the rest! (create virus normally)
                                            free(record->citizenID);
                                            free(record->firstName);
                                            free(record->lastName);
                                            free(record->country);
                                            free(record);
                                        }
                                        
                                        Skip_list skip_virus = insert_virus_list(temp_virus,temp_vacc_status);
                                        insert_skip_list(skip_virus,temp_id,temp_date,temp_country);

                                        if (strcmp(temp_vacc_status,"YES")==0)
                                        {
                                            Bloom_filter bloom_virus = insert_filter_list(temp_virus);
                                            set_bloom_filter(bloom_virus,temp_id);
                                        } 
                                    }
                                    fclose(fp);
                                    free(txt_path);
                                }
                                free(country_path);
                                closedir(sub_dir);
                            }

                        }
                        closedir(dir);
                    }


                    //Let travelMonitor know the number of viruses/bloom filters it will get from monitor.
                    int bloom_filters_number = num_bloom_filters();
                    write(write_fd,&bloom_filters_number,sizeof(int));

                    char* bf_array; //will point to the bloom array we are about to send
                    char** bloom_virus_names = get_bloom_names();
                    for (int i = 0; i < num_bloom_filters(); i++) //loop virus names of bloom filters
                    {
                        //Sent the name of the virus/bloom filter.
                        int virus_name_length = strlen(bloom_virus_names[i]);
                        write(write_fd,&virus_name_length,sizeof(int));
                        error = string_pipe("write",write_fd,bloom_virus_names[i],virus_name_length,bufferSize);
                        if (error==-1)
                        {
                            exit(-1);
                        }

                        //Send the bloom array.
                        Bloom_filter bloom_filter = get_bloom_filter(bloom_virus_names[i]);
                        bf_array = get_bloom_array(bloom_filter);
                        error = string_pipe("write",write_fd,bf_array,sizeOfBloom,bufferSize);
                        if (error==-1)
                        {
                            exit(-1);
                        }
                    }
                    for (int i = 0; i < num_bloom_filters(); i++)
                    {
                        free(bloom_virus_names[i]);
                    }
                    free(bloom_virus_names);

                    sigusr1_flag=0;
                    continue;
                }
                
            }
        }
        
        command = malloc(sizeof(char)*command_length + 1); 
        command[0] = '\0';

        string_pipe("read",read_fd,command,command_length,bufferSize);

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////    /travelRequest    /////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

        if (strcmp(command,"/travelRequest")==0)
        {
            //Get virus.
            int virus_length;
            read(read_fd,&virus_length,sizeof(int));
            char* virus = malloc(sizeof(char)*virus_length + 1);
            virus[0] = '\0';
            string_pipe("read",read_fd,virus,virus_length,bufferSize);
            Virus_list virus_list = get_virus_list("YES");
            Skip_list skip_list = get_skip_list(virus_list,virus);
            free(virus);

            //Get key.
            int key_length;
            read(read_fd,&key_length,sizeof(int));
            char* key = malloc(sizeof(char)*key_length + 1);
            key[0] = '\0';
            string_pipe("read",read_fd,key,key_length,bufferSize);

            //Search skip list for citizen id.
            int found = search_skip_list(skip_list,key);
            write(write_fd,&found,sizeof(int));
            char date_vaccinated[SIZE_OF_DATE];
            if (found == 1) //send YES and a date
            {
                string_pipe("write",write_fd,"YES",sizeof("YES"),bufferSize);
                strcpy(date_vaccinated,get_date_skip_list(skip_list,key));
                string_pipe("write",write_fd,date_vaccinated,SIZE_OF_DATE,bufferSize);
            }
            else if (found == 0) //send NO
            {
                string_pipe("write",write_fd,"NO",sizeof("NO"),bufferSize);
            }
            //Waiting travelMonitor to inform me if request approved or rejected.
            char approved;
            read(read_fd,&approved,sizeof(char));
            if (approved=='y')
            {
                accepted_requests++;
            }
            else if (approved=='n')
            {
                rejected_requests++;
            }
            free(key);
            free(command);
            continue;
        }

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////    /searchVaccinationStatus    ///////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

        else if (strcmp(command,"/searchVaccinationStatus")==0)
        {
            //Get id.
            int key_length;
            read(read_fd,&key_length,sizeof(int));
            char* key = malloc(sizeof(char)*key_length + 1);
            key[0] = '\0';
            string_pipe("read",read_fd,key,key_length,bufferSize);
            Citizen_record rec = search_hash_table(key); //search for key in hash table
            int found=0;
            if (rec!=NULL) //citizen found, inform the parent
            {
                found=1;
                write(write_fd,&found,sizeof(int));
            }
            else //monitor doesn't have citizen, continue
            {
                write(write_fd,&found,sizeof(int));
                free(key);
                free(command);
                continue;
            }

            Skip_list skip_list;
            int count=0; //counts the number of skip lists that will contain the key we are looking for
            Skip_list* table_of_skip_lists = skip_lists_key(&count,key); //get the needed skip lists
            
            //About to send the citizen info to the parent.  
            
            //Send the size of the attributes.
            int fname_length = strlen(rec->firstName);
            int lname_length = strlen(rec->lastName);
            int country_length = strlen(rec->country);
            write(write_fd,&fname_length,sizeof(int));
            write(write_fd,&lname_length,sizeof(int));
            write(write_fd,&country_length,sizeof(int));

            //Send the atteributes.
            string_pipe("write",write_fd,rec->firstName,fname_length,bufferSize);
            string_pipe("write",write_fd,rec->lastName,lname_length,bufferSize);
            string_pipe("write",write_fd,rec->country,country_length,bufferSize);
            write(write_fd,&rec->age,sizeof(int));

            //Send the number of viruses that parent should expect.
            write(write_fd,&count,sizeof(int));

            //Send virus names, status, dates.
            for (int i = 0; i < count; i++)
            {
                int virus_length = strlen(get_virus(table_of_skip_lists[i]));
                write(write_fd,&virus_length,sizeof(int));
                string_pipe("write",write_fd,get_virus(table_of_skip_lists[i]),virus_length,bufferSize);
                int status_length = strlen(get_vacc(table_of_skip_lists[i]));
                write(write_fd,&status_length,sizeof(int));
                string_pipe("write",write_fd,get_vacc(table_of_skip_lists[i]),status_length,bufferSize);
                if (strcmp(get_vacc(table_of_skip_lists[i]),"YES")==0)
                {
                    string_pipe("write",write_fd,get_date_skip_list(table_of_skip_lists[i],key),SIZE_OF_DATE,bufferSize); 
                }
            }
            
            free(table_of_skip_lists);
            free(key);
            free(command);
            continue;
        }  
    }while(1);

    //Monitor never gets to free its stuff it exits with SIGKILL from the parent.
    //Freeing stuff.

    for (int i = 0; i < num_countries; i++)
    {
        free(countries[i]);
    }
    free(countries);
    close(write_fd);
    close(read_fd);
    free(input_dir);
    free(current_date);
    delete_hash_table();
    delete_virus_list();
    delete_filter_list();
    delete_country_list();  

}
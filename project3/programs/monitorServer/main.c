#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <pthread.h>
#include <fcntl.h>
#include <dirent.h>
#include <errno.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include "virus_list.h"
#include "filter_list.h"
#include "various_functions.h"
#include "country_list.h"
#include "list.h"

#define ESTIMATED_LINES 5 //for hash table of records and skip lists
#define SIZE_OF_DATE 10 //dd-mm-yyyy

pthread_mutex_t cyclic_mutex;
pthread_mutex_t update_mutex;
pthread_cond_t cond_nonempty;
pthread_cond_t cond_nonfull;

typedef struct 
{
    char** cyclic_Buffer;
    int start;
    int end;
    int count;
    int num_items;
    int cyclicBufferSize;
} cyclic_buf_t;

//Thread obtains path from cyclic buffer, reads txt file and then updates data structures.
void *thread_initializer(void* argv)
{
    cyclic_buf_t *cyclic_buf = argv;
    while (cyclic_buf->num_items >= 0 || cyclic_buf->count > 0)
    {
        pthread_mutex_lock(&cyclic_mutex);
        while (cyclic_buf->count <= 0) 
        {
            pthread_cond_wait(&cond_nonempty, &cyclic_mutex);
        }
        
        //Thread exiting?
        if (strcmp(cyclic_buf->cyclic_Buffer[cyclic_buf->start],"exit")==0)
        {
            free(cyclic_buf->cyclic_Buffer[cyclic_buf->start]);
            cyclic_buf->start = (cyclic_buf->start + 1) % cyclic_buf->cyclicBufferSize;
            cyclic_buf->count--;
            pthread_mutex_unlock(&cyclic_mutex);
            pthread_cond_signal(&cond_nonfull);
            pthread_exit(NULL);
        }

        char* txt_path = malloc(strlen(cyclic_buf->cyclic_Buffer[cyclic_buf->start])+1);
        txt_path[0] = '\0';
        strcpy(txt_path,cyclic_buf->cyclic_Buffer[cyclic_buf->start]);
        
        FILE *fp = fopen(txt_path,"r");
        if (fp==NULL)
        {
            perror("ERROR ");
            return -1;
        }
        free(txt_path);

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
        free(cyclic_buf->cyclic_Buffer[cyclic_buf->start]);
        cyclic_buf->start = (cyclic_buf->start + 1) % cyclic_buf->cyclicBufferSize;
        cyclic_buf->count--;
        pthread_mutex_unlock(&cyclic_mutex);
        pthread_cond_signal(&cond_nonfull);
        usleep(100000);
    }
}




/////////////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////                                              /////////////////////////////////
//////////////////////////////          SERVER PROCESS STARTING             /////////////////////////////////
//////////////////////////////                                              /////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////////////




int main(int argc,char *argv[])
{
    printf("<SERVER %d : Starting>\n",getpid());

    cyclic_buf_t cyc_buf, *cyclic_buf;
    cyclic_buf=&cyc_buf;

    //Monitor will keep track of travel requests (for log file).
    int rejected_requests=0;
    int accepted_requests=0;

    int port = atoi(argv[1]);
    int numThreads = atoi(argv[2]);
    int socketBufferSize = atoi(argv[3]);
    cyclic_buf->cyclicBufferSize = atoi(argv[4]);
    int sizeOfBloom = atoi(argv[5]);


    //Find how many countries are assigned to this monitor-server
    int c=6;
    while (argv[c]!=NULL)
    {
        c++;
    }
    int num_countries=c-6;

    //Save the country paths for later use.
    char** path_countries = malloc(num_countries * sizeof(char*));
    c=6;
    for (int i = 0; i < num_countries; i++)
    {
        path_countries[i] = malloc(strlen(argv[c]) + 1);
        strcpy(path_countries[i],argv[c]);
        c++;
    }

    struct sockaddr_in server, client;
    struct sockaddr *serverptr=(struct sockaddr *)&server;
    struct sockaddr *clientptr=(struct sockaddr *)&client;
    struct hostent *rem;

    int reuse_addr = 1;

    int sock = socket(PF_INET, SOCK_STREAM, 0);
    if (sock < 0)
    {
        printf("Error in socket\n");
        exit(1);
    }

    char host_name[100];
    gethostname(host_name,sizeof(host_name));
    rem = gethostbyname(host_name);
    if (rem == NULL)
    {
        printf("Error in gethostbyname\n");
        exit(1);
    }
    
    server.sin_family = AF_INET;
    server.sin_addr.s_addr = htonl(INADDR_ANY);
    server.sin_port = htons(port);
    
    setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &reuse_addr, sizeof(reuse_addr));
    
    int bind_error = bind(sock, serverptr, sizeof(server));
    if (bind_error < 0)
    {
        perror("Error in bind");
        exit(1);
    }

    int listen_error = listen(sock,1);
    if (listen_error < 0)
    {
        perror("Error in listen");
        exit(1);
    }

    while (1) 
    {
        socklen_t clientlen = sizeof(client);
        /* accept connection */
    	int new_sock = accept(sock, clientptr, &clientlen);
        if (new_sock < 0)
        {
            perror("Error in accept\n");
            exit(1);
        }

    	/* Find client's address */
        struct hostent *cl_rem = gethostbyaddr((char *) &client.sin_addr.s_addr, sizeof(client.sin_addr.s_addr), client.sin_family);
        if (cl_rem == NULL)
        {
            perror("Error in gethostbyaddr\n");
            exit(1);
        }
        printf("%d Accepted connection from %s\n",getpid(),cl_rem->h_name);
    	pid_t pid = fork(); //create a new process to handle accepted connection
        if (pid==-1)
        {
            perror("Error in fork\n"); 
            exit(1);
        }
        if (pid>0)
        {
            int close_error = close(new_sock);
            if (close_error==-1)
            {
                perror("Error in close\n");
                exit(1);
            }
            pid_t wpid;
            while ((wpid = wait(NULL)) > 0);

            for (int i = 0; i < num_countries; i++)
            {
                free(path_countries[i]);
            }
            
            free(path_countries);

            exit(0);
        }
        if (pid==0)
        {
            int close_error = close(sock);
            if (close_error==-1)
            {
                perror("Error in close\n");
                exit(1);
            }

            
            cyclic_buf->cyclic_Buffer = malloc(cyclic_buf->cyclicBufferSize *sizeof(char*));
            if (cyclic_buf->cyclic_Buffer==NULL)
            {
                perror("Error in malloc\n");
                exit(1);
            }
            cyclic_buf->start=0;
            cyclic_buf->end=-1;
            cyclic_buf->count=0;
            cyclic_buf->num_items=0;


            //Loop all files of given countries to get the total number of txts.
            List country_numtxt_list = create_list(); //will keep track of how many txt files each country has
            DIR *dir;
            struct dirent *txt;
            for (int i = 0; i < num_countries; i++)
            {
                dir = opendir(path_countries[i]); //open country directory
                insert_list(country_numtxt_list,path_countries[i],"0"); //initialize country's num_txts to 0
                if(dir == NULL)
                {
                    perror("Error in directory");
                    exit(1);
                }
                while ((txt = readdir(dir))) //loop files
                {
                    if((strcmp(txt->d_name,".")==0)||(strcmp(txt->d_name,"..")==0)) //skip ./ and ../
                    {
                        continue;
                    }
                    cyclic_buf->num_items++;

                    //Updating the number of txts for this country.
                    int num_txts = get_value_list(country_numtxt_list,path_countries[i]);
                    char* str_num_txts = malloc(9);
                    num_txts++;
                    sprintf(str_num_txts,"%d",num_txts);
                    update_list(country_numtxt_list,path_countries[i],str_num_txts);
                    free(str_num_txts);
                }
                closedir(dir);
            }

            pthread_mutex_init(&cyclic_mutex, 0);
            pthread_cond_init(&cond_nonempty, 0);
            pthread_cond_init(&cond_nonfull, 0);


            //Create a list of pointers to bloom filters with size 'sizeOfBloom'.
            create_filter_list(sizeOfBloom);
            //Create two lists of pointers to skip lists,
            //one list contains vaccinated skip lists - the other not vaccinated.
            create_virus_list(get_levels(ESTIMATED_LINES));
            create_country_list();
            //Create hash table to store valid records.
            create_hash_table(get_buckets(ESTIMATED_LINES));


            //Create 'numThreads' threads to assign the txt files.
            pthread_t thread_ids[numThreads];
            for (int i = 0; i < numThreads; i++)
            {
                if (pthread_create(&thread_ids[i], NULL, thread_initializer, cyclic_buf) != 0)
                {
                    perror("Error in pthread_create");
                    exit(1);
                }
            }

            //Loop again to share txt-paths with threads.
            for (int i = 0; i < num_countries; i++)
            {
                dir = opendir(path_countries[i]); //open country directory
                
                if(dir == NULL)
                {
                    perror("Error in directory");
                    exit(1);
                }
                while ((txt = readdir(dir))) //loop files
                {
                    if((strcmp(txt->d_name,".")==0)||(strcmp(txt->d_name,"..")==0)) //skip ./ and ../
                    {
                        continue;
                    }
                    
                    //Path to txt will be : ../../input_dir/Country/Country-x.txt
                    char* txt_path = malloc(strlen(path_countries[i])+1+strlen(txt->d_name)+1);
                    strcpy(txt_path,path_countries[i]);
                    strcat(txt_path,"/");
                    strcat(txt_path,txt->d_name);

                    pthread_mutex_lock(&cyclic_mutex);
                    while (cyclic_buf->count >= cyclic_buf->cyclicBufferSize) 
                    {
                        pthread_cond_wait(&cond_nonfull, &cyclic_mutex);
                    }
                    
                    cyclic_buf->end = (cyclic_buf->end + 1) % cyclic_buf->cyclicBufferSize;
                    
                    cyclic_buf->cyclic_Buffer[cyclic_buf->end] = malloc(strlen(txt_path)+1);
                    if (cyclic_buf->cyclic_Buffer[cyclic_buf->end] == NULL)
                    {
                        perror("Error in malloc");
                    }
                    
                    strcpy(cyclic_buf->cyclic_Buffer[cyclic_buf->end],txt_path);
                    free(txt_path);
                    cyclic_buf->count++;
                    
                    cyclic_buf->num_items--;
                    
                    pthread_mutex_unlock(&cyclic_mutex);
                    pthread_cond_signal(&cond_nonempty);
                    usleep(100000);
                }
                closedir(dir);
            }
            usleep(100000);




/////////////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////                                              /////////////////////////////////
//////////////////////////////        SENT BLOOM FILTERS TO CLIENT          /////////////////////////////////
//////////////////////////////                                              /////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////////////




            //Send the number of bloom filters to client.
            int bloom_filters_number = num_bloom_filters();
            write(new_sock,&bloom_filters_number,sizeof(int));
            char** bloom_virus_names = get_bloom_names();
            int bytes;
            for (int i = 0; i < bloom_filters_number; i++) //loop virus names of bloom filters
            {
                //Sent the name-length and the name of the virus/bloom filter.
                int virus_name_length = strlen(bloom_virus_names[i]);
                write(new_sock,&virus_name_length,sizeof(int));
                string_pipe("write",new_sock,bloom_virus_names[i],virus_name_length,socketBufferSize);
                
                //Send the bloom array.
                Bloom_filter bloom_filter = get_bloom_filter(bloom_virus_names[i]);
                if (bloom_filter == NULL)
                {
                    printf("Error in get_bloom_filter()\n");
                    exit(1);
                }
                char* bf_array;
                bf_array=get_bloom_array(bloom_filter);
                if (bf_array == NULL)
                {
                    printf("Error in get_bloom_array()\n");
                    exit(1);
                }              
                string_pipe("write",new_sock,bf_array,sizeOfBloom,socketBufferSize);
            }     




/////////////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////                                              /////////////////////////////////
//////////////////////////////        READING COMMANDS FROM CLIENT          /////////////////////////////////
//////////////////////////////                                              /////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////////////




            printf("<SERVER %d : Ready for queries.>\n",getpid());
            char input[100], temp_input[100];
            char *command, *in1, *in2, *in3, *in4, *in5, *in6, *in7, *in8, *in9;
            do
            {  
                //Read command to execute.
                int command_length;
                int bytes = read(new_sock,&command_length,sizeof(int));
                if (bytes < 0)
                {
                    perror("Error in read");
                    exit(1);
                }
                
                command = malloc(sizeof(char)*command_length + 1); 
                command[0] = '\0';
                string_pipe("read",new_sock,command,command_length,socketBufferSize);


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////    /travelRequest    /////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////


                if (strcmp(command,"/travelRequest")==0)
                {
                    //Get virus.
                    int virus_length;
                    read(new_sock,&virus_length,sizeof(int));
                    char* virus = malloc(sizeof(char)*virus_length + 1);
                    virus[0] = '\0';
                    string_pipe("read",new_sock,virus,virus_length,socketBufferSize);
                    Virus_list virus_list = get_virus_list("YES");
                    Skip_list skip_list = get_skip_list(virus_list,virus);
                    free(virus);

                    //Get key.
                    int key_length;
                    read(new_sock,&key_length,sizeof(int));
                    char* key = malloc(sizeof(char)*key_length + 1);
                    key[0] = '\0';
                    string_pipe("read",new_sock,key,key_length,socketBufferSize);

                    //Search skip list for citizen id.
                    int found = search_skip_list(skip_list,key);
                    write(new_sock,&found,sizeof(int));
                    char date_vaccinated[SIZE_OF_DATE];
                    if (found == 1) //send YES and a date
                    {
                        string_pipe("write",new_sock,"YES",sizeof("YES"),socketBufferSize);
                        strcpy(date_vaccinated,get_date_skip_list(skip_list,key));
                        string_pipe("write",new_sock,date_vaccinated,SIZE_OF_DATE,socketBufferSize);
                    }
                    else if (found == 0) //send NO
                    {
                        string_pipe("write",new_sock,"NO",sizeof("NO"),socketBufferSize);
                    }
                    //Waiting travelMonitor to inform me if request approved or rejected.
                    char approved;
                    read(new_sock,&approved,sizeof(char));
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
////////////////////////////////////////////    /addVaccinationRecords    ////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////



                else if (strcmp(command,"/addVaccinationRecords")==0)
                {
                    //Get country from travelMonitor-client.
                    int path_country_length;
                    read(new_sock,&path_country_length,sizeof(int));
                    char* path_country = malloc(sizeof(char)*path_country_length + 1);
                    path_country[0] = '\0';
                    string_pipe("read",new_sock,path_country,path_country_length,socketBufferSize);
                    
                    //Find new number of txt-files.
                    dir = opendir(path_country); //open country directory
                    if(dir == NULL)
                    {
                        perror("Error in directory");
                        exit(1);
                    }
                    int new_num_txts=0;
                    while ((txt = readdir(dir))) //loop files
                    {
                        if((strcmp(txt->d_name,".")==0)||(strcmp(txt->d_name,"..")==0)) //skip ./ and ../
                        {
                            continue;
                        }
                        new_num_txts++;
                    }
                    closedir(dir);

                    //Find old number of txt-files.
                    int old_num_txts = get_value_list(country_numtxt_list,path_country);

                    //Find number of just added txt-files..
                    cyclic_buf->num_items = new_num_txts - old_num_txts;
                    if (cyclic_buf->num_items == 0)
                    {
                        int added = 0;
                        write(new_sock,&added,sizeof(int));
                        free(path_country);
                        free(command);
                        continue;
                    }

                    //Updating the number of txts.
                    char* str_new_num_txts = malloc(9);
                    sprintf(str_new_num_txts,"%d",new_num_txts);
                    update_list(country_numtxt_list,path_country,str_new_num_txts);
                    free(str_new_num_txts);

                    cyclic_buf->start=0;
                    cyclic_buf->end=-1;
                    cyclic_buf->count=0;
                    
                    //Share new txt-paths with threads.
                    
                    dir = opendir(path_country); //open country directory
                    if(dir == NULL)
                    {
                        perror("Error in directory");
                        exit(1);
                    }
                    while ((txt = readdir(dir))) //loop files
                    {
                        
                        if((strcmp(txt->d_name,".")==0)||(strcmp(txt->d_name,"..")==0)) //skip ./ and ../
                        {
                            continue;
                        }

                        //Determine the number of the txt-file to know if it is new or old.
                        int txtnum1=0; //place of '-' in Country-x.txt
                        int txtnum2=0; //place of '.' in Country-x.txt
                        int digits=0;
                        for (int i = 0; i < strlen(txt->d_name); i++)
                        {
                            if (txt->d_name[i]=='-')
                            {
                                txtnum1=i;
                            }
                            if (txt->d_name[i]=='.')
                            {
                                txtnum2=i;
                            }
                        }
                        char* number = malloc(9);
                        number[0]='\0';
                        int pos=0;
                        for (int i = txtnum1+1; i < txtnum2; i++)
                        {
                            
                            number[pos++] = txt->d_name[i];
                        }
                        number[pos] = '\0';

                        //If we have Country-y.txt where y <= old number of txts, then we 've seen this file before, so skip it
                        if (atoi(number) <= old_num_txts)
                        {
                            free(number);
                            continue;
                        }
                        free(number);

                        //Path to txt will be : ../../input_dir/Country/Country-x.txt
                        char* txt_path = malloc(strlen(path_country)+1+strlen(txt->d_name)+1);
                        strcpy(txt_path,path_country);
                        strcat(txt_path,"/");
                        strcat(txt_path,txt->d_name);

                        pthread_mutex_lock(&cyclic_mutex);
                        while (cyclic_buf->count >= cyclic_buf->cyclicBufferSize) 
                        {
                            pthread_cond_wait(&cond_nonfull, &cyclic_mutex);
                        }
                        
                        cyclic_buf->end = (cyclic_buf->end + 1) % cyclic_buf->cyclicBufferSize;
                        
                        cyclic_buf->cyclic_Buffer[cyclic_buf->end] = malloc(strlen(txt_path)+1);
                        if (cyclic_buf->cyclic_Buffer[cyclic_buf->end] == NULL)
                        {
                            perror("Error in malloc");
                        }
                        
                        strcpy(cyclic_buf->cyclic_Buffer[cyclic_buf->end],txt_path);
                        free(txt_path);
                        cyclic_buf->count++;
                        cyclic_buf->num_items--;
                        
                        pthread_mutex_unlock(&cyclic_mutex);
                        pthread_cond_signal(&cond_nonempty);
                        usleep(100000);
                    }
                    closedir(dir);
                    usleep(10000);
                    
                    //Inform the parent that new txt-files have been added.
                    int added = 1;
                    write(new_sock,&added,sizeof(int));

                    //Send the number of bloom filters to client.
                    int bloom_filters_number = num_bloom_filters();
                    write(new_sock,&bloom_filters_number,sizeof(int));
                    char** bloom_virus_names = get_bloom_names();
                    int bytes;
                    for (int i = 0; i < bloom_filters_number; i++) //loop virus names of bloom filters
                    {
                        //Sent the name-length and the name of the virus/bloom filter.
                        int virus_name_length = strlen(bloom_virus_names[i]);
                        write(new_sock,&virus_name_length,sizeof(int));
                        string_pipe("write",new_sock,bloom_virus_names[i],virus_name_length,socketBufferSize);
                        
                        //Send the bloom array.
                        Bloom_filter bloom_filter = get_bloom_filter(bloom_virus_names[i]);
                        if (bloom_filter == NULL)
                        {
                            printf("Error in get_bloom_filter()\n");
                            exit(1);
                        }
                        char* bf_array;
                        bf_array=get_bloom_array(bloom_filter);
                        if (bf_array == NULL)
                        {
                            printf("Error in get_bloom_array()\n");
                            exit(1);
                        }              
                        string_pipe("write",new_sock,bf_array,sizeOfBloom,socketBufferSize);
                    }
                    for (int i = 0; i < num_bloom_filters(); i++)
                    {
                        free(bloom_virus_names[i]);
                    }
                    free(bloom_virus_names);
                    free(path_country);
                    free(command);
                    continue;
                }




// //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ///////////////////////////////////////////    /searchVaccinationStatus    ///////////////////////////////////////////////
// //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////




                else if (strcmp(command,"/searchVaccinationStatus")==0)
                {
                    //Get id.
                    int key_length;
                    read(new_sock,&key_length,sizeof(int));
                    char* key = malloc(sizeof(char)*key_length + 1);
                    key[0] = '\0';
                    string_pipe("read",new_sock,key,key_length,socketBufferSize);
                    Citizen_record rec = search_hash_table(key); //search for key in hash table
                    int found=0;
                    if (rec!=NULL) //citizen found, inform the parent
                    {
                        found=1;
                        write(new_sock,&found,sizeof(int));
                    }
                    else //monitor doesn't have citizen, continue
                    {
                        write(new_sock,&found,sizeof(int));
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
                    write(new_sock,&fname_length,sizeof(int));
                    write(new_sock,&lname_length,sizeof(int));
                    write(new_sock,&country_length,sizeof(int));

                    //Send the atteributes.
                    string_pipe("write",new_sock,rec->firstName,fname_length,socketBufferSize);
                    string_pipe("write",new_sock,rec->lastName,lname_length,socketBufferSize);
                    string_pipe("write",new_sock,rec->country,country_length,socketBufferSize);
                    write(new_sock,&rec->age,sizeof(int));

                    //Send the number of viruses that parent should expect.
                    write(new_sock,&count,sizeof(int));

                    //Send virus names, status, dates.
                    for (int i = 0; i < count; i++)
                    {
                        int virus_length = strlen(get_virus(table_of_skip_lists[i]));
                        write(new_sock,&virus_length,sizeof(int));
                        string_pipe("write",new_sock,get_virus(table_of_skip_lists[i]),virus_length,socketBufferSize);
                        int status_length = strlen(get_vacc(table_of_skip_lists[i]));
                        write(new_sock,&status_length,sizeof(int));
                        string_pipe("write",new_sock,get_vacc(table_of_skip_lists[i]),status_length,socketBufferSize);
                        if (strcmp(get_vacc(table_of_skip_lists[i]),"YES")==0)
                        {
                            string_pipe("write",new_sock,get_date_skip_list(table_of_skip_lists[i],key),SIZE_OF_DATE,socketBufferSize); 
                        }
                    }
                    
                    free(table_of_skip_lists);
                    free(key);
                    free(command);
                    continue;
                }




///////////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////    /exit    ///////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////////




                else if (strcmp(command,"/exit")==0)
                {
                    
                    cyclic_buf->num_items = numThreads;
                    for (size_t i = 0; i < numThreads; i++)
                    {
                        pthread_mutex_lock(&cyclic_mutex);
                        while (cyclic_buf->count >= cyclic_buf->cyclicBufferSize) 
                        {
                            pthread_cond_wait(&cond_nonfull, &cyclic_mutex);
                        }
                        
                        cyclic_buf->end = (cyclic_buf->end + 1) % cyclic_buf->cyclicBufferSize;
                        
                        cyclic_buf->cyclic_Buffer[cyclic_buf->end] = malloc(strlen("exit")+1);
                        if (cyclic_buf->cyclic_Buffer[cyclic_buf->end] == NULL)
                        {
                            perror("Error in malloc");
                        }
                        
                        strcpy(cyclic_buf->cyclic_Buffer[cyclic_buf->end],"exit");
                        cyclic_buf->count++;
                        cyclic_buf->num_items--;
                        
                        pthread_mutex_unlock(&cyclic_mutex);
                        pthread_cond_signal(&cond_nonempty);   
                        usleep(100000);
                    }

                    free(command);

                    //Wait threads and destroy mutex/cond variables.
                    for (int i = 0; i < numThreads; i++)
                    {
                        if (pthread_join(thread_ids[i], NULL) != 0)
                        {
                            perror("Error in pthread_join");
                            exit(1);
                        }
                    }
                    free(cyclic_buf->cyclic_Buffer);
                    pthread_mutex_destroy(&cyclic_mutex);
                    pthread_cond_destroy(&cond_nonempty);
                    pthread_cond_destroy(&cond_nonfull);

                    //Print stats in log file.
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

                    for (int i = 0; i < num_countries; i++)
                    {
                        free(path_countries[i]);
                        c++;
                    }
                    free(path_countries);

                    for (int i = 0; i < num_bloom_filters(); i++)
                    {
                        free(bloom_virus_names[i]);
                    }
                    free(bloom_virus_names);
                    
                    printf("<SERVER %d : Frees stuff and exits>\n",getpid());

                    delete_list(country_numtxt_list);
                    delete_filter_list();
                    delete_hash_table();
                    delete_virus_list();
                    delete_country_list();

                    close_error = close(new_sock);
                    if (close_error==-1)
                    {
                        printf("Error in close\n");
                    }
                    exit(0);
                }
            } while (1);
        }
    }
}
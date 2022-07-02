#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "virus_list.h"
#include "filter_list.h"
#include "various_functions.h"
#include "country_list.h"

int main(int argc,char *argv[])
{
    if ((argc!=5)||(argv[1][0]!='-')||(argv[3][0]!='-')) //must have flags
    {
        printf("Error in arguments. Execute program as: ./vaccineMonitor -c citizenRecordsFile -b bloomSize\n");
        return (-1);
    }
    char* input_file;
    int bloom_size;
    if ((argv[1][1]=='c')&&(argv[3][1]=='b')) //checking the order of arguments/flags
    {
        input_file = malloc(strlen("../files/")+strlen(argv[2])+1);
        if (input_file==NULL)
        {
            perror("Error in malloc");
            exit(1);
        }
        
        strcpy(input_file,"../files/"); //copy the path of the input files and
        strcat(input_file,argv[2]); //append the name of our input file
        bloom_size = atoi(argv[4]);
    }
    else if ((argv[1][1]=='b')&&(argv[3][1]=='c'))
    {
        input_file = malloc(strlen("../files/")+strlen(argv[4])+1);
        if (input_file==NULL)
        {
            perror("Error in malloc");
            exit(1);
        }
        strcpy(input_file,"../files/");
        strcat(input_file,argv[4]);
        bloom_size = atoi(argv[2]);
    }
    else
    {
        printf("Error in arguments. Execute program as: ./vaccineMonitor -c citizenRecordsFile -b bloomSize\n");
        return (-1);
    }

    srand(time(NULL));
    //Get and store current date.
    char* current_date = malloc(11*sizeof(char));
    if (current_date==NULL)
    {
        perror("Error in malloc");
        exit(1);
    }
    current_date[0] = '\0';
    get_current_date(&current_date);
    //Count lines to use in hash table and skip lists.
    int records = count_lines(input_file);

    //Create a list of pointers to bloom filters.
    create_filter_list(bloom_size);
    //Create two lists of pointers to skip lists,
    //one list contains vaccinated skip lists - the other not vaccinated.
    create_virus_list(get_levels(records));
    create_country_list();
    //Create hash table to store valid records.
    create_hash_table(get_buckets(records));
    
    FILE *fp = fopen(input_file,"r");
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
            if (strcmp(temp_vacc_status,"NO")==0) //is'NO' followed by a date?
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
        strcpy(record->citizenID,temp_id);
        strcpy(record->firstName,temp_fn);
        strcpy(record->lastName,temp_ln);
        record->age=atoi(temp_age);

        int check = insert_hash_table(record);
        if(check==1)
        {
            //check=1 : duplicate id - different name/surname/age/country - Error
            free(record->citizenID);
            free(record->firstName);
            free(record->lastName);
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
                free(record);
                printf("ERROR <<ID-VIRUS>> IN RECORD: %s\n",temp_str);
                continue;
            }
            //DIFFERENT VIRUS : free duplicate record (no need to insert record in hash table)
            //but don't skip the rest! (create virus normally)
            free(record->citizenID);
            free(record->firstName);
            free(record->lastName);
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


//////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////

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
/////////////////////////////////////    /vaccineStatusBloom    /////////////////////////////////////////
        if (strcmp(command,"/vaccineStatusBloom")==0)
        {
            in1 = strtok(NULL," ");
            in2 = strtok(NULL," ");
            in3 = strtok(NULL," ");
            if ((in1==NULL)||(in2==NULL)||(in3!=NULL))
            {
                printf("WRONG INPUT! Try again as : /vaccineStatusBloom citizenID virusName\n");
                continue;
            }

            if(search_hash_table(in1)==NULL)
            {
                printf("WRONG INPUT! Citizen does not exist.\n");
                continue;
            }
            Virus_list vyes = get_virus_list("YES");
            Virus_list vno = get_virus_list("NO");
            if ((get_skip_list(vyes,in2)==NULL)&&((get_skip_list(vno,in2)==NULL))) //if no virus is given
            {
                printf("WRONG INPUT! Virus does not exist.\n");
                continue;
            }
            //if virus exists but NOT for any vaccinated citizen then citizen definitely NOT VACCINATED
            if ((get_skip_list(vyes,in2)==NULL)&&((get_skip_list(vno,in2)!=NULL)))
            {
                printf("NOT VACCINATED\n");
                continue;
            }
            //Virus exists so proceed.
            Bloom_filter bloom_virus = get_bloom_filter(in2);
            int check = check_bloom_filter(bloom_virus,in1);
            if (check==1)
            {
                printf("MAYBE\n");
            }
            else if (check==0)
            {
                printf("NOT VACCINATED\n");
            }   
        }
/////////////////////////////////////    /vaccineStatus    /////////////////////////////////////////
        else if (strcmp(command,"/vaccineStatus")==0)
        {
            in1 = strtok(NULL," ");
            in2 = strtok(NULL," ");
            in3 = strtok(NULL," ");
            if ((in1!=NULL)&&(in2==NULL)) //citizen id given without a virus
            {
                if(search_hash_table(in1)==NULL)
                {
                    printf("WRONG INPUT! Citizen does not exist.\n");
                    continue;
                }    
                printid_virus_list(in1);
                continue;
            }
            
            if ((in1==NULL)||(in2==NULL)||(in3!=NULL))
            {
                printf("WRONG INPUT! Try again as : /vaccineStatus citizenID [virusName].\n");
                continue;
            }
            if(search_hash_table(in1)==NULL)
            {
                printf("WRONG INPUT! Citizen does not exist.\n");
                continue;
            }
            Virus_list vyes = get_virus_list("YES");
            Virus_list vno = get_virus_list("NO");
            if ((get_skip_list(vyes,in2)==NULL)&&((get_skip_list(vno,in2)==NULL))) //if no virus is given
            {
                printf("WRONG INPUT! Virus does not exist.\n");
                continue;
            }
            
            Skip_list skip_virus = get_skip_list(vyes,in2);
            if (skip_virus!=NULL)
            {
                char* date = get_date_skip_list(skip_virus,in1);
                if (date!=NULL)
                {
                    printf("VACCINATED ON %s\n",date);
                    continue;
                }
                else
                {
                    printf("NOT VACCINATED\n");
                    continue;
                }
            }
            skip_virus = get_skip_list(vno,in2);
            if (skip_virus!=NULL)
            {
                char* date = get_date_skip_list(skip_virus,in1);
                if (date==NULL)
                {
                    printf("NOT VACCINATED\n");
                    continue;
                }
                else //we are in a 'no skip list', this will never be true
                {
                    printf("An unexpected error occurred."); //:-D
                    exit(0);
                }
            }
        }
/////////////////////////////////    /populationStatus + /popStatusByAge  /////////////////////////////////////
        else if ((strcmp(command,"/populationStatus")==0)||(strcmp(command,"/popStatusByAge")==0))
        {
            in1 = strtok(NULL," ");
            in2 = strtok(NULL," ");
            in3 = strtok(NULL," ");
            in4 = strtok(NULL," ");
            in5 = strtok(NULL," ");
            //less than one argument (just the command) or more than 5 arguments
            if ((in5!=NULL)||(in1==NULL))
            {
                if (strcmp(command,"/populationStatus")==0)
                {
                    printf("WRONG INPUT! Try again as : /populationStatus [country] virusName date1 date2.\n");
                }
                else if (strcmp(command,"/popStatusByAge")==0)
                {
                    printf("WRONG INPUT! Try again as : /popStatusByAge [country] virusName date1 date2.\n");
                }
                continue;
            }
            //if a country is given
            if (search_country_list(in1)!=NULL)
            {
                Virus_list vyes = get_virus_list("YES");
                Virus_list vno = get_virus_list("NO");
                if ((get_skip_list(vyes,in2)==NULL)&&((get_skip_list(vno,in2)==NULL))) //if no virus is given
                {
                    printf("WRONG INPUT! Virus does not exist.\n");
                    continue;
                }
                if (in3==NULL) //case1: country virus
                {
                    Skip_list virus_yes = get_skip_list(vyes,in2);
                    Skip_list virus_no = get_skip_list(vno,in2);
                    if (strcmp(command,"/populationStatus")==0)
                    {
                        int count_vaccinated = cv_population_status(virus_yes,in1);
                        int count_not_vaccinated = cv_population_status(virus_no,in1);
                        if ((count_vaccinated==0)&&(count_not_vaccinated==0))
                        {
                            printf("%s %d no citizen found\n",in1,count_vaccinated);
                            continue;
                        }
                        printf("%s %d %.2f%%\n",in1,count_vaccinated,100*count_vaccinated/(float)(count_vaccinated+count_not_vaccinated));
                    }
                    else if (strcmp(command,"/popStatusByAge")==0)
                    {
                        cv_pop_status_by_age(virus_yes,virus_no,in1);
                    } 
                    continue;
                }
                else if((in3!=NULL)&&(in4==NULL)) //no second date
                {
                    printf("WRONG INPUT! A second date must be given\n");
                    continue;
                }
                else //case2: country virus dates
                {
                    int date1 = check_reformat_date(in3);
                    int date2 = check_reformat_date(in4);
                    if((date1==-1)||(date2==-1))
                    {
                        printf("WRONG INPUT! Invalid date! Must be given as dd-mm-yyyy.\n");
                        continue;
                    }
                    if (date1>date2)
                    {
                        printf("WRONG INPUT! Second date must be greater or equal to the first one!\n");
                        continue;
                    }
                    if (date2>check_reformat_date(current_date))
                    {
                        printf("WRONG INPUT! Second date cannot be greater than current date!\n");
                        continue;
                    }
                 
                    Skip_list virus_yes = get_skip_list(vyes,in2);
                    Skip_list virus_no = get_skip_list(vno,in2);
                    if (strcmp(command,"/populationStatus")==0)
                    {
                        int count_vaccinated = cvd_population_status(virus_yes,in1,date1,date2);
                        int count_not_vaccinated = cvd_population_status(virus_no,in1,date1,date2);
                        if ((count_vaccinated==0)&&(count_not_vaccinated==0))
                        {
                            printf("%s %d no citizen found\n",in1,count_vaccinated);
                            continue;
                        }
                        printf("%s %d %.2f%%\n",in1,count_vaccinated,100*count_vaccinated/(float)(count_vaccinated+count_not_vaccinated));
                    }
                    else if (strcmp(command,"/popStatusByAge")==0)
                    {
                        cvd_pop_status_by_age(virus_yes,virus_no,in1,date1,date2);
                    }
                    continue; 
                }  
            }
            //if a country is not given
            else if ((search_country_list(in1)==NULL))
            {
                Virus_list vyes = get_virus_list("YES");
                Virus_list vno = get_virus_list("NO");
                //attempt to give one word that is not a virus
                if ((get_skip_list(vyes,in1)==NULL)&&(get_skip_list(vno,in1)==NULL)&&(in2==NULL)) 
                {
                    printf("WRONG INPUT! Virus does not exist.\n");
                    continue;
                }
                //attempt to give false country followed by virus
                else if ((get_skip_list(vyes,in2)!=NULL)||(get_skip_list(vno,in2)!=NULL))
                {
                    printf("WRONG INPUT! Country does not exist.\n");
                    continue;
                }
                //attempt to give one virus
                else if ((get_skip_list(vyes,in1)!=NULL)||(get_skip_list(vno,in1)!=NULL))
                {
                    if (in2==NULL) //case3: virus
                    {
                        if (strcmp(command,"/populationStatus")==0)
                        {
                            v_population_status(in1);
                        }
                        else if (strcmp(command,"/popStatusByAge")==0)
                        {
                            v_pop_status_by_age(in1);
                        }
                        continue;
                    }
                    else if((in2!=NULL)&&(in3==NULL))
                    {
                        printf("WRONG INPUT! A second date must be given\n");
                        continue;
                    }
                    else //case4: virus dates
                    {
                        int date1 = check_reformat_date(in2);
                        int date2 = check_reformat_date(in3);
                        if((date1==-1)||(date2==-1))
                        {
                            printf("WRONG INPUT! Invalid date! Must be given as dd-mm-yyyy.\n");
                            continue;
                        }
                        if (date1>date2)
                        {
                            printf("WRONG INPUT! Second date must be greater or equal to the first one!\n");
                            continue;
                        }
                        if (date2>check_reformat_date(current_date))
                        {
                            printf("WRONG INPUT! Second date cannot be greater than current date!\n");
                            continue;
                        }
                        if (strcmp(command,"/populationStatus")==0)
                        {
                            vd_population_status(in1,date1,date2);
                        }
                        else if (strcmp(command,"/popStatusByAge")==0)
                        {
                            vd_pop_status_by_age(in1, date1, date2);
                        }
                        continue;
                    }
                }
                //attempt to give false country followed by false virus
                else if ((get_skip_list(vyes,in2)==NULL)&&(get_skip_list(vno,in2)==NULL))
                {
                    printf("WRONG INPUT! Country does not exist. Virus does not exist.\n");
                    continue;
                }
            }
        }
//////////////////////////////////////////    /insertCitizenRecord    //////////////////////////////////////////////
        else if (strcmp(command,"/insertCitizenRecord")==0)
        {
            in1 = strtok(NULL," "); //citizen id
            in2 = strtok(NULL," "); //first name
            in3 = strtok(NULL," "); //last name
            in4 = strtok(NULL," "); //country
            in5 = strtok(NULL," "); //age
            in6 = strtok(NULL," "); //virus
            in7 = strtok(NULL," "); //YES/NO
            in8 = strtok(NULL," "); //[date]
            in9 = strtok(NULL," "); //NULL

            if ((in9!=NULL)||(in7==NULL))
            {
                printf("WRONG INPUT! Try again as : /insertCitizenRecord citizenID firstName lastName country age virusName YES/NO [date].\n");
                continue;
            }
            Virus_list vyes = get_virus_list("YES");
            Virus_list vno = get_virus_list("NO");
            Skip_list virus_yes = get_skip_list(vyes,in6);
            //We need to search for the citizen in the 'YES skip list' to see if he is already vaccinated.
            if (search_skip_list(virus_yes,in1)==1)
            {
                printf("ERROR: CITIZEN %s ALREADY VACCINATED ON %s\n",in1,get_date_skip_list(virus_yes,in1));
                continue;
            }
            
            if (atoi(in5)<=0 || atoi(in5)>120) //valid age?
            {
                printf("ERROR <<WRONG-AGE>> Age must be between 1 and 120.\n");
                continue;
            }
            if ((in8==NULL)&&(strcmp(in7,"YES")==0))
            {
                printf("ERROR <<YES-DATE>> ""YES"" must be followed by a date.\n");
                continue;
            }
            if(in8!=NULL)
            {
                if(strlen(in8)>11) //invalid date
                {
                    printf("ERROR <<INVALID-DATE>> Format must be given as dd-mm-yyyy. Try again.\n");
                    continue;
                }
                int check_date = check_reformat_date(in8);
                if (check_date==-1) //invalid format
                {
                    printf("ERROR <<INVALID-DATE>> Format must be given as dd-mm-yyyy. Try again.\n");
                    continue;
                }
                if (check_date>check_reformat_date(current_date)) //future date
                {
                    printf("WRONG INPUT! Date cannot be greater than current date!\n");
                    continue;
                }
                if (strcmp(in7,"NO")==0) //is 'NO' followed by a date?
                {
                    printf("ERROR <<NO-DATE>> ""NO"" cannot be followed by a date! Try again.\n");
                    continue;
                }
            }

            Citizen_record record = malloc(sizeof(struct citizen_record));
            if (record==NULL)
            {
                perror("Error in malloc");
                exit(1);
            }
            record->citizenID = malloc(strlen(in1)+1);
            if (record->citizenID==NULL)
            {
                perror("Error in malloc");
                exit(1);
            }
            record->firstName = malloc(strlen(in2)+1);
            if (record->firstName==NULL)
            {
                perror("Error in malloc");
                exit(1);
            }
            record->lastName = malloc(strlen(in3)+1);
            if (record->lastName==NULL)
            {
                perror("Error in malloc");
                exit(1);
            }
            strcpy(record->citizenID,in1);
            strcpy(record->firstName,in2);
            strcpy(record->lastName,in3);
            record->age=atoi(in5);

            int check = insert_hash_table(record);
            if(check==1)
            {
                //check=1 : duplicate id - different name/surname/age/country - Error
                free(record->citizenID);
                free(record->firstName);
                free(record->lastName);
                free(record);
                printf("ERROR <<DUPLICATE-ID>>\n");
                continue;
            }
            if (check==2)
            {
                //check=2 : duplicate record - virus might be the SAME or DIFFERENT.
                //Need to check if citizen is NOT vaccinated (already checked if he IS vaccinated)
                //for this virus. We will search v_no virus list.
                Skip_list virus_no = get_skip_list(vno,in6);
                if (search_skip_list(virus_no,in1)==1)
                {
                    //Citizen has vaccinated status = NO for this virus. We need to remove him from 'NO skip list'.
                    remove_skip_list(virus_no,in1);
                }
                //Free record with different or same virus (no need to insert record in hash table).
                free(record->citizenID);
                free(record->firstName);
                free(record->lastName);
                free(record);
            }
            Skip_list skip_virus = insert_virus_list(in6,in7);
            insert_skip_list(skip_virus,in1,in8,in4);

            if (strcmp(in7,"YES")==0)
            {
                Bloom_filter bloom_virus = insert_filter_list(in6);
                set_bloom_filter(bloom_virus,in1);
            }
            continue;  
        }
////////////////////////////////////////////    /vaccinateNow     ////////////////////////////////////////////////
        else if (strcmp(command,"/vaccinateNow")==0)
        {
            in1 = strtok(NULL," "); //citizen id
            in2 = strtok(NULL," "); //first name
            in3 = strtok(NULL," "); //last name
            in4 = strtok(NULL," "); //country
            in5 = strtok(NULL," "); //age
            in6 = strtok(NULL," "); //virus
            in7 = strtok(NULL," "); //NULL

            if ((in7!=NULL)||(in6==NULL))
            {
                printf("WRONG INPUT! Try again as : /vaccinateNow citizenID firstName lastName country age virusName.\n");
                continue;
            }
            Virus_list vyes = get_virus_list("YES");
            Virus_list vno = get_virus_list("NO");
            Skip_list virus_yes = get_skip_list(vyes,in6);
            //We need to search for the citizen in the 'YES skip list' to see if he is already vaccinated.
            if (search_skip_list(virus_yes,in1)==1)
            {
                printf("ERROR: CITIZEN %s ALREADY VACCINATED ON %s\n",in1,get_date_skip_list(virus_yes,in1));
                continue;
            }
            
            if (atoi(in5)<=0 || atoi(in5)>120) //valid age?
            {
                printf("ERROR <<WRONG-AGE>> Age must be between 1 and 120.\n");
                continue;
            }

            Citizen_record record = malloc(sizeof(struct citizen_record));
            if (record==NULL)
            {
                perror("Error in malloc");
                exit(1);
            }
            record->citizenID = malloc(strlen(in1)+1);
            if (record->citizenID==NULL)
            {
                perror("Error in malloc");
                exit(1);
            }
            record->firstName = malloc(strlen(in2)+1);
            if (record->firstName==NULL)
            {
                perror("Error in malloc");
                exit(1);
            }
            record->lastName = malloc(strlen(in3)+1);
            if (record->lastName==NULL)
            {
                perror("Error in malloc");
                exit(1);
            }
            strcpy(record->citizenID,in1);
            strcpy(record->firstName,in2);
            strcpy(record->lastName,in3);
            record->age=atoi(in5);

            int check = insert_hash_table(record);
            if(check==1)
            {
                //check=1 : duplicate id - different name/surname/age/country - Error
                free(record->citizenID);
                free(record->firstName);
                free(record->lastName);
                free(record);
                printf("ERROR <<DUPLICATE-ID>>\n");
                continue;
            }
            if (check==2)
            {
                //check=2 : duplicate record - virus might be the SAME or DIFFERENT.
                //Need to check if citizen is NOT vaccinated (already checked if he IS vaccinated)
                //for this virus. We will search v_no virus list.
                Skip_list virus_no = get_skip_list(vno,in6);
                if (search_skip_list(virus_no,in1)==1)
                {
                    //Citizen has vaccinated status = NO for this virus. We need to remove him from 'NO skip list'.
                    remove_skip_list(virus_no,in1);
                }
                //Free record with different or same virus (no need to insert record in hash table).
                free(record->citizenID);
                free(record->firstName);
                free(record->lastName);
                free(record);
            }
            Skip_list skip_virus = insert_virus_list(in6,"YES");
            insert_skip_list(skip_virus,in1,current_date,in4);

            Bloom_filter bloom_virus = insert_filter_list(in6);
            set_bloom_filter(bloom_virus,in1);

            continue; 
        }
////////////////////////////////////////    /list-nonVaccinated-Persons     ////////////////////////////////////////////
        else if (strcmp(command,"/list-nonVaccinated-Persons")==0)
        {
            in1 = strtok(NULL," "); //virus name
            in2 = strtok(NULL," "); //NULL

            if ((in2!=NULL)||(in1==NULL))
            {
                printf("WRONG INPUT! Try again as :/list-nonVaccinated-Persons virusName.\n");
                continue;
            }

            Skip_list no_virus = get_skip_list(get_virus_list("NO"),in1);
            Skip_list yes_virus = get_skip_list(get_virus_list("YES"),in1);
            if ((no_virus==NULL)&&(yes_virus==NULL))
            {
                printf("WRONG INPUT! Virus does not exist.\n");
                continue;
            }
            if ((no_virus==NULL)&&(yes_virus!=NULL))
            {
                printf("%s has only vaccinated citizens.\n",in1);
                continue;
            }
            print_skip_list(no_virus);
            continue; 
        }

/////////////////////////////////////    /exit    /////////////////////////////////////////
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
            "\t1) /vaccineStatusBloom citizenID virusName\n"
            "\t2) /vaccineStatus citizenID virusName\n"
            "\t3) /vaccineStatus citizenID\n"
            "\t4) /populationStatus [country] virusName date1 date2\n"
            "\t5) /popStatusByAge [country] virusName date1 date2\n"
            "\t6) /insertCitizenRecord citizenID firstName lastName country age virusName YES/NO [date]\n"
            "\t7) /vaccinateNow citizenID firstName lastName country age virusName\n"
            "\t8) /list-nonVaccinated-Persons virusName\n"
            "\t9) /exit\n");
        }

        else
        {
            printf("WRONG INPUT! Command does not exist. Try /info to see a list of commands.\n");
            continue;
        }
    }while(1);
    
    delete_hash_table();
    delete_virus_list();
    delete_filter_list();
    delete_country_list();
    free(current_date);
    free(input_file);
    return 0;
}
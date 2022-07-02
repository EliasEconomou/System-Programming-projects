#!/bin/bash    

#Function to create random dates.
random_date ()
{
    local dd=$((1 + RANDOM % 30))
    local mm=$((1 + RANDOM % 12))
    local yyyy=$((1990 + RANDOM % 32))
    dd_size=${#dd}
    mm_size=${#mm}
    #Append 0 before one-digit days or months.
    if ((dd_size == 1))
    then
        dd="0${dd}"
    fi
    if ((mm_size == 1))
    then
        mm="0${mm}"
    fi
    date="$dd-$mm-$yyyy"
    echo "$date"
}



if (($# != 4)) #number of arguments must be 4
then
    echo "WRONG INPUT! Try again as : ./testFile.sh virusesFile countriesFile numLines duplicatesAllowed.";
    exit 1
fi

if [[ -d "../files" ]] #if directory 'files' already exist
then
    cd ../files #move in 'files' directory
else
    echo "ERROR : Directory files is not there.";
    exit 1
fi

if [[ ! -f $1 ]] #a virusesFile must exist
then
    echo "ERROR : virusFile not found.";
    exit 1
fi

if [[ ! -f $2 ]] #a countriesFile must exist
then
    echo "ERROR : countriesFile not found.";
    exit 1
fi

if (($3 < 1)) #numLines can't be less than 1
then
    echo "ERROR : numLines must be greater than 0.";
    exit 1
fi

if [[ -f inputFile ]] #if inputFile already exist remove it
then
    rm inputFile
fi
touch inputFile

#Create two tables to contain viruses and countries listed in the files.
v_size=0
while IFS= read -r virus || [[ "$virus" ]]
do
    v_size=$(($v_size + 1))
    viruses+=("$virus")
done < virusesFile
c_size=0
while IFS= read -r country || [[ "$country" ]]
do
    c_size=$(($c_size + 1))
    countries+=("$country")
done < countriesFile

name_chars="abcdefghijklmonpqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ"; #characters available for names

#Duplicate keys not allowed - duplicatesAllowed = 0
if (($4 == 0))
then
    #Create a table of 'numLines' shuffled numbers from 0 to 9999.
    records=($(shuf -i 0-9999 -n $3))
    for citizen_id in "${records[@]}" #loop for every number
    do
        id_size=${#citizen_id} #make all numbers having 4 digits size
        if ((id_size==1))
        then
            citizen_id="000$citizen_id"
        elif ((id_size==2))
        then
            citizen_id="00$citizen_id"
        elif ((id_size==3))
        then
            citizen_id="0$citizen_id"
        fi

        #Create random first and last name of size 3 to 12.
        fn_size=$((3 + $RANDOM % 10))
        first_name=$(name=""; for ((i=0;i<fn_size;i++)); do char=${name_chars:$RANDOM % ${#name_chars}:1}; name+=$char; done; 
        echo $name)
        ln_size=$((3 + $RANDOM % 10))
        last_name=$(name=""; for ((i=0;i<ln_size;i++)); do char=${name_chars:$RANDOM % ${#name_chars}:1}; name+=$char; done; 
        echo $name)
        
        #Pick a random country and virus from previous created tables
        pick_country=$((RANDOM % $c_size)) 
        country=${countries[pick_country]}
        pick_virus=$((RANDOM % $v_size))
        virus_name=${viruses[pick_virus]}
        
        false_age=$((RANDOM % 200))
        if ((false_age < 198)) #1/200 possibility to be a false age - inconsistent
        then
            false_age=0
        fi
        age=$((1 + RANDOM % 120 + false_age))
        
        has_date=$((RANDOM % 200)) #possibility to have a date - for inconsistency - debug
        pick_vacc=$((RANDOM % 20))
        date=" "
        if ((pick_vacc < 10))
        then
            vacc_status=YES
        else
            vacc_status=NO
        fi
        if [[ $vacc_status = "YES" ]]
        then
            if ((has_date < 199)) #if YES => 199/200 possibility to have date - consistent
            then
                date="$(random_date)"
            fi
        fi
        if [[ $vacc_status = "NO" ]]
        then
            if ((has_date > 198)) #if NO => 1/200 possibility to have date - inconsistent
            then
                date="$(random_date)"
            fi
        fi
        line="$citizen_id $first_name $last_name $country $age $virus_name $vacc_status $date"
        echo "$line" >> inputFile
    done
#Duplicate keys allowed - duplicatesAllowed != 0
else
    #Loop as many times as the number of lines given.
    for ((i = 0; i < $3; i++))
    do
        citizen_id=$(($RANDOM % 10000))
        id_size=${#citizen_id} #make all numbers having 4 digits size
        if ((id_size==1))
        then
            citizen_id="000$citizen_id"
        elif ((id_size==2))
        then
            citizen_id="00$citizen_id"
        elif ((id_size==3))
        then
            citizen_id="0$citizen_id"
        fi
        
        #Possibility of consistent duplicate ids (same fn-ln-country-age) will be 90%. Even
        #a 'at first' consistent citizen record might be inconsistent if duplicate record 
        #refers to the same virus (smaller chance).
        same_citizen=$((RANDOM % 10))
        #Searching column-1 (ids) for current citizen_id - if we find it pass (fn-ln-country-age)
        #to citizen.
        citizen=$(awk -vcol="$1" -vsearch="$citizen_id" '{if($1 == search){print $2,$3,$4,$5;exit;}}' inputFile)
        if [[ ! -z "$citizen" ]]
        then
            if ((same_citizen > 0)) #(9/10 chance same citizen attributes - consistent)
            then
                #(fn-ln-country-age) will be the same
                
                pick_virus=$((RANDOM % $v_size))
                virus_name=${viruses[pick_virus]}

                has_date=$((RANDOM % 200)) #possibility to have a date - for inconsistency - debug
                pick_vacc=$((RANDOM % 20))
                date=" "
                if ((pick_vacc < 10))
                then
                    vacc_status=YES
                else
                    vacc_status=NO
                fi
                if [[ $vacc_status = "YES" ]]
                then
                    if ((has_date < 199)) #if YES => 199/200 possibility to have date - consistent
                    then
                        date="$(random_date)"
                    fi
                fi
                if [[ $vacc_status = "NO" ]]
                then
                    if ((has_date > 198)) #if NO => 1/200 possibility to have date - inconsistent
                    then
                        date="$(random_date)"
                    fi
                fi
                line="$citizen_id $citizen $virus_name $vacc_status $date"
                echo $line >> inputFile
            elif ((same_citizen == 0)) #(1/10 chance different citizen attributes - inconsistent)
            then  
                #Create random first and last name of size 3 to 12.
                fn_size=$((3 + $RANDOM % 10))
                first_name=$(name=""; for ((i=0;i<fn_size;i++)); do char=${name_chars:$RANDOM % ${#name_chars}:1}; name+=$char; done; 
                echo $name)
                ln_size=$((3 + $RANDOM % 10))
                last_name=$(name=""; for ((i=0;i<ln_size;i++)); do char=${name_chars:$RANDOM % ${#name_chars}:1}; name+=$char; done; 
                echo $name)

                #Pick a random country and virus from previous created tables
                pick_country=$((RANDOM % $c_size))
                country=${countries[pick_country]}
                pick_virus=$((RANDOM % $v_size))
                virus_name=${viruses[pick_virus]}
                
                false_age=$((RANDOM % 200))
                if ((false_age < 198)) #1/200 possibility to be a false age - inconsistent
                then
                    false_age=0
                fi
                age=$((1 + RANDOM % 120 + false_age))

                has_date=$((RANDOM % 200)) #possibility to have a date - for inconsistency - debug
                pick_vacc=$((RANDOM % 20))
                date=" "
                if ((pick_vacc < 10))
                then
                    vacc_status=YES
                else
                    vacc_status=NO
                fi
                if [[ $vacc_status = "YES" ]]
                then
                    if ((has_date < 199)) #if YES => 199/200 possibility to have date - consistent
                    then
                        date="$(random_date)"
                    fi
                fi
                if [[ $vacc_status = "NO" ]]
                then
                    if ((has_date > 198)) #if NO => 1/200 possibility to have date - inconsistent
                    then
                        date="$(random_date)"
                    fi
                fi
                line="$citizen_id $first_name $last_name $country $age $virus_name $vacc_status $date"
                echo $line >> inputFile
            fi
        else
      		#Create random first and last name of size 3 to 12.
        	fn_size=$((3 + $RANDOM % 10))
        	first_name=$(name=""; for ((i=0;i<fn_size;i++)); do char=${name_chars:$RANDOM % ${#name_chars}:1}; name+=$char; done; 
        	echo $name)
        	ln_size=$((3 + $RANDOM % 10))
        	last_name=$(name=""; for ((i=0;i<ln_size;i++)); do char=${name_chars:$RANDOM % ${#name_chars}:1}; name+=$char; done; 
        	echo $name)

            #Pick a random country and virus from previous created tables
            pick_country=$((RANDOM % $c_size))
            country=${countries[pick_country]}
            pick_virus=$((RANDOM % $v_size))
            virus_name=${viruses[pick_virus]}
            
            false_age=$((RANDOM % 200))
            if ((false_age < 198)) #1/200 possibility to be a false age - inconsistent
            then
                false_age=0
            fi
            age=$((1 + RANDOM % 120 + false_age))

            has_date=$((RANDOM % 200)) #possibility to have a date - for inconsistency - debug
            pick_vacc=$((RANDOM % 20))
            date=" "
            if ((pick_vacc < 10))
            then
                vacc_status=YES
            else
                vacc_status=NO
            fi
            if [[ $vacc_status = "YES" ]]
            then
                if ((has_date < 199)) #if YES => 199/200 possibility to have date - consistent
                then
                    date="$(random_date)"
                fi
            fi
            if [[ $vacc_status = "NO" ]]
            then
                if ((has_date > 198)) #if NO => 1/200 possibility to have date - inconsistent
                then
                    date="$(random_date)"
                fi
            fi
            line="$citizen_id $first_name $last_name $country $age $virus_name $vacc_status $date"
            echo $line >> inputFile
        fi
    done
fi
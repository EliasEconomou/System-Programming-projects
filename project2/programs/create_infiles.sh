#!/bin/bash


##########################################################
################### Checking arguments ###################
##########################################################

if (($# != 3)) #number of arguments must be 3
then
    echo "WRONG INPUT! Try again as : ./create_infiles.sh inputFile input_dir numFilesPerDirectory.";
    exit 1
fi

if (($3 < 1)) #numFilesPerDirectory must be greater than 0
then
    echo "ERROR : numFilesPerDirectory must be greater than 0.";
    exit 1
fi

cd ../

if [[ ! -f $1 ]] #if inputFile doesn't exist
then
    echo "ERROR : File $1 doesn't exist.";
    exit 1
fi

if [[ -d $2 ]] #if directory (input_dir) already exists
then
    echo "ERROR : Directory $2 already exists.";
    exit 1
else
    mkdir $2
fi


#########################################################
################### Extract countries ###################
#########################################################

#Create a table to contain all distinct countries listed in the inputFile.
while IFS= read -r line || [[ "$line" ]]
do
    country=$(echo $line | awk '{print $4}') #extract country from record
    if [[ "${countries[@]}" =~ "${country}" ]] #if country does not exist in countries, add it
    then
        continue
    fi
    countries+=("$country")
done < $1
#printf '%s\n' "${countries[@]}" #print all distinct countries


#################################################################################
################### Create subdirectories and country-n files ###################
#################################################################################

cd $2 #get inside input_dir
for country in "${countries[@]}" #create country subdirectories
do
    mkdir $country
    cd $country
    for ((i = 1; i <= $3; i++)) #create files for every country
    do
        name="$country-$i.txt"
        touch $name
    done
    cd ..
done


###############################################################
################### Fill files with records ###################
###############################################################

cd ..
if [[ -d "temp_dir" ]] #if temp_dir already exists remove it
then
    rm -r temp_dir
fi
mkdir temp_dir
awk '{print>"temp_dir/"$4}' $1 #split records into files by country

for country in "${countries[@]}" #for every country
do
    num_line=0
    cd $2/$country
    while IFS= read -r line || [[ "$line" ]] #get line
    do
        file=$(( $num_line  % $3 + 1 )) #write it to appropriate file using round robin
        name="$country-$file.txt"
        echo $line >> $name
        num_line=$(($num_line + 1))
    done < ../../temp_dir/$country
    cd ../..
done

rm -r temp_dir

#####################################################################
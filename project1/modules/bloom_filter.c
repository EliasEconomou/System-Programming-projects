#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "bloom_filter.h"

#define HASH_FUNCTIONS 16

struct bloom_filter
{
    char* virus_name;
    int bloom_size;
    int *bloom_array;
};


//Creates the bloom filter and initializes all bits to 0.
Bloom_filter create_bloom_filter(int bloom_size,char* virus_name)
{
    Bloom_filter bloom_filter = malloc(sizeof(struct bloom_filter));
    if (bloom_filter==NULL)
    {
        perror("Error in malloc");
        exit(1);
    }
    bloom_filter->virus_name = malloc(strlen(virus_name)+1);
    if (bloom_filter->virus_name==NULL)
    {
        perror("Error in malloc");
        exit(1);
    }
    strcpy(bloom_filter->virus_name,virus_name);
    bloom_filter->bloom_size=bloom_size;
    bloom_filter->bloom_array=malloc(bloom_size);
    if (bloom_filter->bloom_array==NULL)
    {
        perror("Error in malloc");
        exit(1);
    }
    //size of array will be 100.000bytes/4bytes/int=25.000integers
    int num_of_ints=bloom_size/sizeof(int);
    //initialize bits of the array to 0
    for(int i=0;i<num_of_ints;i++) 
    {
        bloom_filter->bloom_array[i]=bloom_filter->bloom_array[i]&0;
    }
    return bloom_filter;
}


//Given an item sets the apropriate bit of bloom filter to 1.
void set_bloom_filter(Bloom_filter bloom_filter,char* key)
{
    int int_bits=sizeof(int)*8; //an integer is this many bits
    for (int i=0; i<HASH_FUNCTIONS; i++)
    {
        //the integer in the array of bits, that needs changing
        int pos_in_array=hash_i((unsigned char*)key,i)%(bloom_filter->bloom_size/sizeof(int));
        int pos_in_int=pos_in_array%int_bits; //position of the bit in 'this' integer
        unsigned int set_bit = 1; //...00000001
        set_bit = set_bit << pos_in_int; //shift set_bit to the right position in the integer
        // Now set the appropriate bit to '1'
        bloom_filter->bloom_array[pos_in_array] = bloom_filter->bloom_array[pos_in_array]|set_bit;
    }
}


//Returns 1 if item exists and 0 if it doesn't.
int check_bloom_filter(Bloom_filter bloom_filter,char* key)
{
    int int_bits=sizeof(int)*8; //an integer is this many bits
    for (int i=0; i<HASH_FUNCTIONS; i++)
    {
        //the integer in the array of bits, that needs changing
        int pos_in_array=hash_i((unsigned char*)key,i)%(bloom_filter->bloom_size/sizeof(int));
        int pos_in_int=pos_in_array%int_bits; //position of the bit in 'this' integer
        unsigned int check_bit = 1; //...00000001
        check_bit = check_bit << pos_in_int; //shift check_bit to the right position in the integer 
        //if bit is zero item doesn't exist so return 0
        if ((bloom_filter->bloom_array[pos_in_array]&check_bit)==0)
        {
            return 0;
        }  
    }
    //all bits have been tested to 1 so item exists
    return 1;
}


//Free bloom filter.
void delete_bloom_filter(Bloom_filter bloom_filter)
{
    free(bloom_filter->virus_name);
    free(bloom_filter->bloom_array);
    free(bloom_filter);
}
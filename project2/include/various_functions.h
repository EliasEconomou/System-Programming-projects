#ifndef VARIOUS_FUNCTIONS_H
#define VARIOUS_FUNCTIONS_H

//Read/write string from/to pipe.
int string_pipe(char* function, int fd, char* string, int string_length, int buffer_size);

//String comparison function for qsort.
int string_cmp(const void *string1, const void *string2);

//Used to count all records of the given file.
int count_lines(char* input_file);

//Used for hash table creation.
int get_buckets(int records);

//Used for skip list creation. A skip list can have at most all records refer to the same virus.
int get_levels(int records);

//Get current date.
void get_current_date(char** current_date);

//Check if a date is valid and if it is, return a reformatted integer - else return -1
//(for example: if date is 17-10-2007 return 20071017)
int check_reformat_date(char* date);


#endif
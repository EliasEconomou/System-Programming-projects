#ifndef STAT_RECORD_H
#define STAT_RECORD_H

typedef struct stat_record
{
    char* country_name;
    char* date;
    char approved; //'y' or 'n'
}*Stat_record;


#endif
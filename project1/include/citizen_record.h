#ifndef CITIZEN_RECORD_H
#define CITIZEN_RECORD_H

typedef struct citizen_record
{
    char* citizenID;
    char* firstName;
    char* lastName;
    int age;
}*Citizen_record;


#endif
# from lesson 'domes dedomenwn-xatzikokolakhs' modified properly

# paths
INCLUDE = ../../include
MODULES = ../../modules

# compiler
CC = gcc

# Compile options. Το -I<dir> λέει στον compiler να αναζητήσει εκεί include files
CFLAGS = -Wall -g -I$(INCLUDE)
LDFLAGS = -lm -lpthread

# Αρχεία .o
OBJS = main.o $(MODULES)/list.o $(MODULES)/bloom_filter.o $(MODULES)/hash_functions.o $(MODULES)/skip_list.o $(MODULES)/hash_table.o $(MODULES)/various_functions.o $(MODULES)/virus_list.o $(MODULES)/filter_list.o $(MODULES)/country_list.o

# Το εκτελέσιμο πρόγραμμα
EXEC = monitorServer


$(EXEC): $(OBJS) $(LIBS)
	$(CC) $(OBJS) $(LIBS) -o $(EXEC) $(LDFLAGS)

clean:
	rm -f $(OBJS) $(EXEC)
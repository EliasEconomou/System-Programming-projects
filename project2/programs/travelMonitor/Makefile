# from lesson 'domes dedomenwn-xatzikokolakhs' modified properly

# paths
INCLUDE = ../../include
MODULES = ../../modules

# compiler
CC = gcc

# Compile options. Το -I<dir> λέει στον compiler να αναζητήσει εκεί include files
CFLAGS = -Wall -g -I$(INCLUDE)
LDFLAGS = -lm

# Αρχεία .o
OBJS = main.o $(MODULES)/list.o $(MODULES)/bloom_filter.o $(MODULES)/hash_functions.o $(MODULES)/skip_list.o $(MODULES)/hash_table.o $(MODULES)/various_functions.o $(MODULES)/virus_list.o $(MODULES)/filter_list.o $(MODULES)/country_list.o $(MODULES)/virus_table.o

# Το εκτελέσιμο πρόγραμμα
EXEC = travelMonitor

# Arguments για εκτέλεση με χρήση 'make run'
ARGS = -m 1 -b 100 -s 100000 -i files100

$(EXEC): $(OBJS) $(LIBS)
	$(CC) $(OBJS) $(LIBS) -o $(EXEC) $(LDFLAGS)

clean:
	rm -f $(OBJS) $(EXEC)

run: $(EXEC)
	./$(EXEC) $(ARGS)

valgrind: $(EXEC)
	valgrind --trace-children=yes ./$(EXEC) $(ARGS)
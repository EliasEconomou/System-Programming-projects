# from lesson 'domes dedomenwn-xatzikokolakhs' modified properly

# paths
INCLUDE = ../include
MODULES = ../modules

# compiler
CC = gcc

# Compile options. Το -I<dir> λέει στον compiler να αναζητήσει εκεί include files
CFLAGS = -Wall -g -I$(INCLUDE)
LDFLAGS = -lm

# Αρχεία .o
OBJS = main.o $(MODULES)/bloom_filter.o $(MODULES)/hash_functions.o $(MODULES)/skip_list.o $(MODULES)/hash_table.o $(MODULES)/various_functions.o $(MODULES)/virus_list.o $(MODULES)/filter_list.o $(MODULES)/country_list.o

# Το εκτελέσιμο πρόγραμμα
EXEC = vaccineMonitor

# Arguments για εκτέλεση με χρήση 'make run'
ARGS = -c inputFile -b 100000

$(EXEC): $(OBJS) $(LIBS)
	$(CC) $(OBJS) $(LIBS) -o $(EXEC) $(LDFLAGS)

clean:
	rm -f $(OBJS) $(EXEC)

run: $(EXEC)
	./$(EXEC) $(ARGS)

valgrind: $(EXEC)
	valgrind ./$(EXEC) $(ARGS)
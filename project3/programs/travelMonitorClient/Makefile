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
EXEC = travelMonitorClient

# Arguments για εκτέλεση με χρήση 'make run'
ARGS = -m 5 -b 201 -c 40 -s 100000 -i my_input_dir -t 10
# ./travelMonitorClient -m numMonitors -b socketBufferSize -c cyclicBufferSize -s sizeOfBloom -i input_dir -t numThreads

$(EXEC): $(OBJS) $(LIBS)
	$(CC) $(OBJS) $(LIBS) -o $(EXEC) $(LDFLAGS)

clean:
	rm -f $(OBJS) $(EXEC)

run: $(EXEC)
	./$(EXEC) $(ARGS)

valgrind: $(EXEC)
	valgrind --tool=memcheck --leak-check=yes --leak-check=full --show-leak-kinds=all --trace-children=yes ./$(EXEC) $(ARGS)
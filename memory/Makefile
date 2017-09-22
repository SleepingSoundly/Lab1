CC = gcc # default compiler
CFLAGS = -g -Wall # options
LDFLAGS = -lvirt  # need to load with the virt flag

all: memory_coordinator 

memory_coordinator: memory_coordinator.c
	$(CC) -o $@ $< $(CFLAGS) $(LDFLAGS)

clean:
	$(RM) -f *.o *.exe memory_coordinator

CC = gcc # default compiler
CFLAGS = -g -Wall # options
LDFLAGS =  # no load flags for now


all: vcpu_scheduler

vcpu_scheduler: vcpu_scheduler.c
	$(CC) -o $@ $< $(CFLAGS) $(LDFLAGS)

clean:
	$(RM) -f *.o *.exe vcpu_scheduler

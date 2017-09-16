CC = gcc # default compiler
CFLAGS = -g -Wall # options
LDFLAGS = -lvirt  # need to load with the virt flag

all: vcpu_scheduler

vcpu_scheduler: vcpu_scheduler.c
	$(CC) -o $@ $< $(CFLAGS) $(LDFLAGS)

clean:
	$(RM) -f *.o *.exe vcpu_scheduler

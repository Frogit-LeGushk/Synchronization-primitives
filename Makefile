CC=gcc
CFLAGS=-Wall -Wextra -pedantic -ggdb -pg -Wswitch-enum -Wno-pointer-to-int-cast -Wno-int-to-pointer-cast -pthread -std=c11

build: Peterson2 PetersonN TicketLock SharedTicketLock WaitDieLock

Peterson2: Peterson2.c
	$(CC) $(CFLAGS) -o Peterson2 Peterson2.c
PetersonN: PetersonN.c
	$(CC) $(CFLAGS) -o PetersonN PetersonN.c
TicketLock: TicketLock.c
	$(CC) $(CFLAGS) -o TicketLock TicketLock.c
SharedTicketLock: SharedTicketLock.c
	$(CC) $(CFLAGS) -o SharedTicketLock SharedTicketLock.c
WaitDieLock: WaitDieLock.c 
	$(CC) $(CFLAGS) -o WaitDieLock WaitDieLock.c
clean:
	-rm Peterson2 PetersonN TicketLock TicketLock SharedTicketLock WaitDieLock 2>/dev/null

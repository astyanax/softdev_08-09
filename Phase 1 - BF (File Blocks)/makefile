# Project: K25_BF

CC   = gcc
OBJ  = BF_BitOperations.o BF_Lib.o
CFLAGS = -ansi -pedantic-errors -Wall -W -Wshadow -Wunreachable-code -Wno-long-long  -Wno-return-type -Wno-unused-function  -Wno-unused-parameter -Wno-implicit-int
MAINS = main1.o main2.o main3.o
EXES = K25_BF1 K25_BF2 K25_BF3
RM = rm -f

.PHONY: K25_BF1 K25_BF2 K25_BF3 clean

K25_BF1: $(OBJ) main1.o
	$(CC) $(OBJ) main1.o -o K25_BF1
    
K25_BF2: $(OBJ) main2.o
	$(CC) $(OBJ) main2.o -o K25_BF2
    
K25_BF3: $(OBJ) main3.o
	$(CC) $(OBJ) main3.o -o K25_BF3

BF_BitOperations.o: BF_BitOperations.c
	$(CC) -c BF_BitOperations.c -o BF_BitOperations.o $(CFLAGS)

BF_Lib.o: BF_Lib.c
	$(CC) -c BF_Lib.c -o BF_Lib.o $(CFLAGS)

main1.o: main1.c
	$(CC) -c main1.c -o main1.o $(CFLAGS)

main2.o: main2.c
	$(CC) -c main2.c -o main2.o $(CFLAGS)
    
main3.o: main3.c
	$(CC) -c main3.c -o main3.o $(CFLAGS)
    
clean:
	${RM} $(OBJ) $(MAINS) $(EXES) ATHLETES EVENTS PARTICIPATIONS
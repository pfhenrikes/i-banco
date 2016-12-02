CFLAGS = -Wall -g -pedantic

all: i-banco i-banco-terminal

i-banco-terminal: commandlinereader.o i-banco-terminal.o
	gcc -o i-banco-terminal commandlinereader.o i-banco-terminal.o

i-banco: commandlinereader.o contas.o i-banco.o
	gcc -o i-banco -pthread commandlinereader.o contas.o i-banco.o

commandlinereader.o: commandlinereader.c
	gcc $(CFLAGS) -c commandlinereader.c
	
i-banco-terminal.o: i-banco-terminal.c commandlinereader.h
	gcc $(CFLAGS) -c i-banco-terminal.c

i-banco.o: i-banco.c commandlinereader.h contas.h
	gcc $(CFLAGS) -c i-banco.c

contas.o: contas.c contas.h
	gcc $(CFLAGS) -pthread -c contas.c

clean:
	rm -f *.o i-banco
	rm -f *.o i-banco-terminal
	rm -f i-banco-pipe
	rm -f log.txt
	rm -f pipe-*
	rm -f i-banco-sim-*

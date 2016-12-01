CFLAGS = -Wall -g -pedantic

i-banco: commandlinereader.o contas.o i-banco.o
	gcc -o i-banco -pthread commandlinereader.o contas.o i-banco.o

commandlinereader.o: commandlinereader.c
	gcc $(CFLAGS) -c commandlinereader.c

i-banco.o: i-banco.c commandlinereader.h contas.h
	gcc $(CFLAGS) -c i-banco.c

contas.o: contas.c contas.h
	gcc $(CFLAGS) -pthread -c contas.c

clean:
	rm -f *.o i-banco

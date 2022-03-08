wish: wish.o
	gcc -o wish wish.o

wish.o: wish.c
	gcc -c wish.c

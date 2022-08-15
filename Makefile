xie3 : main.o
	gcc -o xie3 main.o -lncursesw
main.o : main.c
	gcc -c main.c
clean :
	rm main.o xie3;

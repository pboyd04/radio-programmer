SRCS=../bytes.c ../motorola.c ../debug.c serial.c

radio: $(SRCS)
	$(CC) -Wall -pedantic $(SRCS) -o$@

clean:
	-@rm radio
	-@rm *.o

# build an executable named myprog from myprog.c
all: dithermatron.c
	$(CROSS_COMPILE)gcc -O1 dithermatron.c -o dithermatron
clean: 
	$(RM) dithermatron
CFLAGS+=-I../include/


all:client

client:main.o
     gcc $^ -o $@ $(CFLAGS)

clen:
    rm -rf *.o client

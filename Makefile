



all: 
	gcc -o demo main.c hello.c -I /opt/local/include/ -L /opt/local/lib -lavformat

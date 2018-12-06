#!/bin/bash

rm config.o
rm parce.o
rm db.o
rm server.o
rm task.o
rm dictionary.o
rm logs.o
rm valid.o

gcc -c logs.c
gcc -c config.c
gcc -c db.c
gcc -c server.c
gcc -c task.c
gcc -c dictionary.c
gcc -c valid.c

gcc wapa_mysql.c -lpthread -lmicrohttpd valid.o dictionary.o logs.o config.o task.o server.o parce.o db.o -L/usr/lib64/mysql/ -lmysqlclient -o controller

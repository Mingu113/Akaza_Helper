default : build
CC = gcc
PROG = akaza
SRC = main
PKG_CONFIG = %GTK4-PKG-CONFIG%
CFLAGS = `pkg-config --cflags gtk4 json-glib-1.0`
LDLIBS = `pkg-config --libs gtk4 json-glib-1.0`
${PROG} : ${PROG}.c
build :
	${CC} ${CFLAGS} -o ${PROG} ${SRC}.c ${LDLIBS} -lcurl -Wall
clean:
	rm ${PROG}
test: build
	./${PROG}
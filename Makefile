CC=gcc
CFLAGS=-g
TARGET:=test
CLILIBDIR=lib/CommandParser
LIBS=-lcli
OBJS=gluethread/glthread.o \
	  graph.o \
	  topologies.o \
	  net.o \
	  nwcli.o \
	  utils.o \

${TARGET}:testapp.o ${OBJS} ${CLILIBDIR}/libcli.a
	${CC} ${CFLAGS} testapp.o ${OBJS} -o test -L ${CLILIBDIR} ${LIBS}
testapp.o:testapp.c
	${CC} ${CFLAGS} -c testapp.c -o testapp.o
glthread/glthread.o:gluethread/glthread.c
	${CC} ${CFLAGS} -c -I gluethread gluethread/glthread.c -o gluethread/glthread.o
graph.o:graph.c
	${CC} ${CFLAGS} -c -I . graph.c -o graph.o
topologies.o:topologies.c
	${CC} ${CFLAGS} -c -I . topologies.c -o topologies.o
net.o:net.c
	${CC} ${CFLAGS} -c -I . net.c -o net.o
utils.o:utils.c
	${CC} ${CFLAGS} -c -I . utils.c -o utils.o
nwcli.o:nwcli.c
	${CC} ${CFLAGS} -c -I . nwcli.c -o nwcli.o
${CLILIBDIR}/libcli.a:
	(cd ${CLILIBDIR}; make)
clean:
	rm ${OBJS} testapp.o
	rm ${TARGET}
	(cd ${CLILIBDIR}; make clean)
all:
	make
	(cd ${CLILIBDIR}; make)
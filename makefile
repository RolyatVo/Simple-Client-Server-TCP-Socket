
EXEC = mftp mftpserve
OBJS = mftp.o mftpserve.o


all: mftp mftpserve 

mftp: mftp.c 
	cc -o mftp mftp.c 

mftpserve: mftpserve.c 
	cc -o mftpserve mftpserve.c


clean: 
	rm -f ${EXEC} ${OBJS} ${EXEC2} ${OBJS2}
#Enter Make test1 for test 1
#Enter Make test2 for test 2
#Enter Make run to compile application

CC=clang
LIBS=-lrt -lpthread
CFLAGS=-g
SOURCE1=key_value.c comp310_a2_test1.c
SOURCE2=key_value.c comp310_a2_test2.c
SOURCE3=key_value.c read_eval.c 
SOURCE4=key_value.c comp310_a2_test1ancien.c

EXEC1=os_test1 
EXEC2=os_test2
EXEC3=os_run
EXEC4=testancien

test1: $(SOURCE1)
	$(CC) -o $(EXEC1) $(CFLAGS) $(SOURCE1) $(LIBS)
	
testancien: $(SOURCE4)
	$(CC) -o $(EXEC4) $(CFLAGS) $(SOURCE4) $(LIBS)

test2: $(SOURCE2)
	$(CC) -o $(EXEC2) $(CFLAGS) $(SOURCE2) $(LIBS)
	
run: $(SOURCE2)
	$(CC) -o $(EXEC3) $(CFLAGS) $(SOURCE3) $(LIBS)

clean:
	rm $(EXEC1) $(EXEC2) $(EXEC3)

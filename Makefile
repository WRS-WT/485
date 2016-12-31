SRC := #
#SRC += new_uart.c
SRC += passenger_com_prot.c
SRC += test.c
SRC += tmUart.c
#OBJ := $(subst .c,.o,$(SRC))
OBJ = $(SRC:%.c=%.o)

#export PKG_CONFIG_PATH=/opt/gtkdfb/lib/pkgconfig
#PREFIX = /opt/gtkdfb
LDFLAGS=-L${PREFIX}/lib -Wl,-rpath,${PREFIX}/lib 
#CFLAGS=-I${PREFIX}/include/gtk-2.0/ 

CC = arm-hismall-linux-gcc
#CC = gcc
FLAG = -Wall $(LDFLAGS) $(CFLAGS) 
#`pkg-config --cflags --libs gtk+-2.0`
OPTION = -lpthread -ldl 
EXEC_NAME = demo
EXEC_PATH = .

.PHONY:clean demo

demo:$(OBJ)
	@echo make ...
	$(CC) $^ -o $(EXEC_PATH)/$(EXEC_NAME) $(FLAG) $(OPTION)
	@echo make over
	@echo Execute target is $(EXEC_PATH)/$(EXEC_NAME)
$(OBJ):%.o:%.c
	$(CC) -c -o $@ $< $(FLAG)
clean:
	@echo clean ...
	rm $(EXEC_PATH)/$(EXEC_NAME) *.o -rf
	@echo clean over
	
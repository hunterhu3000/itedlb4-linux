# C compiler options
CC	= gcc
CFLAGS	= -g -O2             
TARGET	= ite
LIBSPATH = ../lib
PLATFORM = linux32
LIBS	= -lusb-1.0
INC	= /usr/include/libusb-1.0

SRCS = itedlb4flash.c
 
OBJS	= $(SRCS:.c=.o)
 
all:	$(TARGET) 
 
$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) $(EXCHAR) -o $(TARGET) $(OBJS)  $(LIBS) 
 
$(OBJS): %.o: %.c 
	$(CC) -c $(CFLAGS) $(EXCHAR) -o $@ $< -I$(INC)  
 
clean:
	$(RM) $(OBJS) $(TARGET) 

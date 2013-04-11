CROSS		 = 
CC		 = $(CROSS)gcc
CFLAGS		 = -O0 -I.
#CFLAGS		+= -lpthread
OBJS_COMMON	 = hub_client_main.o
OBJS_REMOTE	 = remote.o
OBJS_TARGET	 = target.o
OBJS		 = $(OBJS_COMMON) $(OBJS_REMOTE) $(OBJS_TARGET)
TARGET_REMOTE	 = remote
TARGET_TARGET	 = target

.PHONY: clean purge all

all:	$(TARGET_REMOTE) $(TARGET_TARGET)

.c.o:
	@echo "========================================================"
	$(CC) $(CFLAGS) -c $<

$(TARGET_REMOTE): $(OBJS_REMOTE) $(OBJS_COMMON)
	$(CC) $(CFLAGS) -o $@ $(OBJS_REMOTE) $(OBJS_COMMON)

$(TARGET_TARGET): $(OBJS_TARGET) $(OBJS_COMMON)
	$(CC) $(CFLAGS) -o $@ $(OBJS_TARGET) $(OBJS_COMMON)

clean:
	@echo "Clean project =========================================="
	rm -f *.bak $(TARGET_REMOTE) $(TARGET_TARGET) $(OBJS)

purge:
	rm -f *~

depend: $(OBJS:.o=.c)
	-@ makedepend -- $(CFLAGS) -- -Y $^ -f Makefile 2> /dev/null

# DO NOT DELETE

hub_client_main.o: hub_client.h
remote.o: hub_client.h
target.o: hub_client.h

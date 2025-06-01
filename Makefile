CC := gcc
CFLAGS := -Wall -Wextra -O2
LDFLAGS := -lpthread
INCLUDES := -Iinc
OBJDIR := obj
SRCS := src/server.c src/thread_pool.c
OBJS := $(SRCS:src/%.c=$(OBJDIR)/%.o)
TARGET := server

all: $(OBJDIR) $(TARGET)

$(OBJDIR):
	mkdir -p $(OBJDIR)

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) $(INCLUDES) -o $@ $^ $(LDFLAGS)

$(OBJDIR)/%.o: src/%.c
	$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@

clean:
	rm -f $(OBJS) $(TARGET)
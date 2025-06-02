CC := gcc
CFLAGS := -Wall -Wextra -O2
LDFLAGS := -lpthread
INCLUDES := -Iinc
OBJDIR := obj
SRCS_SERVER := src/server.c src/thread_pool.c
SRCS_CLIENT := src/client.c
OBJS_SERVER := $(SRCS_SERVER:src/%.c=$(OBJDIR)/%.o)
OBJS_CLIENT := $(SRCS_CLIENT:src/%.c=$(OBJDIR)/%.o)
TARGET_SERVER := server
TARGET_CLIENT := client

all: $(OBJDIR) $(TARGET_SERVER) $(TARGET_CLIENT)

$(OBJDIR):
	mkdir -p $(OBJDIR)

$(TARGET_SERVER): $(OBJS_SERVER)
	$(CC) $(CFLAGS) $(INCLUDES) -o $@ $^ $(LDFLAGS)

$(TARGET_CLIENT): $(OBJS_CLIENT)
	$(CC) $(CFLAGS) $(INCLUDES) -o $@ $^ $(LDFLAGS)

$(OBJDIR)/%.o: src/%.c
	$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@

clean:
	rm -f $(OBJS_SERVER) $(OBJS_CLIENT) $(TARGET_SERVER) $(TARGET_CLIENT)
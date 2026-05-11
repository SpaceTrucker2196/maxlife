# maxlife — pure C99, no third-party dependencies.

CC      ?= cc
CFLAGS  ?= -std=c99 -Wall -Wextra -Wpedantic -O2 -D_POSIX_C_SOURCE=200809L
LDFLAGS ?= -lm

PREFIX  ?= /usr/local
BINDIR   = $(PREFIX)/bin

SRC_DIR  = src
INC_DIR  = include
TEST_DIR = tests

SRCS = \
    $(SRC_DIR)/main.c \
    $(SRC_DIR)/terminal.c \
    $(SRC_DIR)/render.c \
    $(SRC_DIR)/palette.c \
    $(SRC_DIR)/fractal.c \
    $(SRC_DIR)/life.c \
    $(SRC_DIR)/animate.c

OBJS    = $(SRCS:.c=.o)
TARGET  = maxlife

TEST_SRCS = $(TEST_DIR)/test_basic.c $(filter-out $(SRC_DIR)/main.c, $(SRCS))
TEST_BIN  = $(TEST_DIR)/test_basic

.PHONY: all clean install test

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

$(SRC_DIR)/%.o: $(SRC_DIR)/%.c $(INC_DIR)/maxlife.h
	$(CC) $(CFLAGS) -I$(INC_DIR) -c $< -o $@

test: $(TEST_BIN)
	./$(TEST_BIN)

$(TEST_BIN): $(TEST_SRCS) $(INC_DIR)/maxlife.h
	$(CC) $(CFLAGS) -I$(INC_DIR) -o $@ $(TEST_SRCS) $(LDFLAGS)

clean:
	rm -f $(OBJS) $(TARGET) $(TEST_BIN)

install: $(TARGET)
	install -d $(DESTDIR)$(BINDIR)
	install -m 755 $(TARGET) $(DESTDIR)$(BINDIR)/$(TARGET)

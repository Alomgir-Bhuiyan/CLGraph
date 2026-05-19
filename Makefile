# Makefile for clgraph
# Usage:
#   make          — build demo
#   make lib      — build static library
#   make termux   — build on Termux (Android)
#   make clean    — remove build artifacts

CC      = gcc
CFLAGS  = -O2 -Wall -Wextra -std=c11
LDFLAGS = -lncurses -lm

# Optional GSL support (uncomment if you have GSL installed)
# CFLAGS  += -DCLG_USE_GSL
# LDFLAGS += -lgsl -lgslcblas

TARGET  = demo
SRCS    = demo.c clgraph.c
OBJS    = $(SRCS:.c=.o)
LIB     = libclgraph.a

.PHONY: all lib termux clean

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

lib: clgraph.o
	ar rcs $(LIB) clgraph.o
	@echo "Built $(LIB)  — link with: gcc your_prog.c -L. -lclgraph -lncurses -lm"

# Termux: ncurses package is 'libncurses-dev' or just 'ncurses'
# pkg install clang ncurses ncurses-dev make
termux:
	$(CC) $(CFLAGS) -o $(TARGET) $(SRCS) $(LDFLAGS)

%.o: %.c clgraph.h
	$(CC) $(CFLAGS) -c -o $@ $<

clean:
	rm -f $(OBJS) $(TARGET) $(LIB)

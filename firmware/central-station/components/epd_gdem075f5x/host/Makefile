# Host preview of the graphics layer (Linux / macOS / MSYS2 / WSL).
#   make            build ./preview
#   make run        build and render preview_*.bmp
ROOT    := ..
CC      ?= cc
CFLAGS  ?= -O2 -g -Wall -Wextra -std=c11
INC     := -I$(ROOT)/include -I. \
           -I$(ROOT)/examples/showcase/main

SRC := $(ROOT)/src/epd_gfx.c $(ROOT)/src/epd_text.c $(wildcard $(ROOT)/fonts/*.c) \
       epd_host.c preview_main.c \
       $(ROOT)/examples/showcase/main/showcase_ui.c

preview: $(SRC) $(wildcard $(ROOT)/include/*.h) Makefile
	$(CC) $(CFLAGS) $(INC) -o $@ $(SRC) -lm

run: preview
	./preview

clean:
	rm -f preview preview_*.bmp

.PHONY: run clean

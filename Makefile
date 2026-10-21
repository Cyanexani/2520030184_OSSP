CC = gcc
CFLAGS = -std=gnu11 -Wall -Wextra -O2

PRACTICAL = $(wildcard Practical/*.c)
SKILL = $(wildcard Skill/*.c)
BINS = $(addprefix bin/,$(basename $(notdir $(PRACTICAL) $(SKILL))))

vpath %.c Practical Skill

.PHONY: all clean list

all: $(BINS)

bin/%: %.c | bin
	$(CC) $(CFLAGS) $< -o $@

bin:
	mkdir -p bin

list:
	@for b in $(BINS); do echo $$b; done

clean:
	rm -rf bin

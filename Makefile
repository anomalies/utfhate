CC=gcc
CFLAGS=-std=c90 -Wall -Wextra -pedantic -Werror
BINDIR=bin
NAME=utfhate
PREFIX=/usr/local

utfhate:source/utfhate.c
	@-test -d $(BINDIR) || mkdir $(BINDIR)
	$(CC) $(CFLAGS) -g -o $(BINDIR)/$(NAME) source/utfhate.c

clean:
	@rm -vf $(BINDIR)/$(NAME)

install:utfhate
	cp -v $(BINDIR)/$(NAME) $(PREFIX)/bin/
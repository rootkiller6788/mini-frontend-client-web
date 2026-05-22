CC = gcc
CFLAGS = -Wall -Wextra -O2 -I include
OUTDIR = bin

SRCS = src/html_parser.c src/css_parser.c src/dom_tree.c src/style_resolver.c src/layout_engine.c
OBJS = $(OUTDIR)/html_parser.o $(OUTDIR)/css_parser.o $(OUTDIR)/dom_tree.o $(OUTDIR)/style_resolver.o $(OUTDIR)/layout_engine.o
LIB = $(OUTDIR)/libminiweb.a

EXAMPLES = $(OUTDIR)/parse_example.exe $(OUTDIR)/dom_example.exe $(OUTDIR)/layout_example.exe

.PHONY: all clean examples

all: $(LIB) examples

$(OUTDIR):
	@if not exist "$(OUTDIR)" mkdir "$(OUTDIR)"

$(LIB): $(OUTDIR) $(OBJS)
	ar rcs $(LIB) $(OBJS)

$(OUTDIR)/html_parser.o: src/html_parser.c include/html_parser.h include/dom_tree.h
	$(CC) $(CFLAGS) -c src/html_parser.c -o $(OUTDIR)/html_parser.o

$(OUTDIR)/css_parser.o: src/css_parser.c include/css_parser.h
	$(CC) $(CFLAGS) -c src/css_parser.c -o $(OUTDIR)/css_parser.o

$(OUTDIR)/dom_tree.o: src/dom_tree.c include/dom_tree.h
	$(CC) $(CFLAGS) -c src/dom_tree.c -o $(OUTDIR)/dom_tree.o

$(OUTDIR)/style_resolver.o: src/style_resolver.c include/style_resolver.h include/dom_tree.h include/css_parser.h
	$(CC) $(CFLAGS) -c src/style_resolver.c -o $(OUTDIR)/style_resolver.o

$(OUTDIR)/layout_engine.o: src/layout_engine.c include/layout_engine.h include/dom_tree.h
	$(CC) $(CFLAGS) -c src/layout_engine.c -o $(OUTDIR)/layout_engine.o

examples: $(EXAMPLES)

$(OUTDIR)/parse_example.exe: examples/parse_example.c $(LIB)
	$(CC) $(CFLAGS) examples/parse_example.c -L$(OUTDIR) -lminiweb -o $(OUTDIR)/parse_example.exe

$(OUTDIR)/dom_example.exe: examples/dom_example.c $(LIB)
	$(CC) $(CFLAGS) examples/dom_example.c -L$(OUTDIR) -lminiweb -o $(OUTDIR)/dom_example.exe

$(OUTDIR)/layout_example.exe: examples/layout_example.c $(LIB)
	$(CC) $(CFLAGS) examples/layout_example.c -L$(OUTDIR) -lminiweb -o $(OUTDIR)/layout_example.exe

clean:
	@if exist "$(OUTDIR)\*.o" del /q "$(OUTDIR)\*.o"
	@if exist "$(OUTDIR)\*.exe" del /q "$(OUTDIR)\*.exe"
	@if exist "$(LIB)" del /q "$(LIB)"

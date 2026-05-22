CC      := gcc
CFLAGS  := -std=c99 -Wall -Wextra -Wpedantic -Wno-unused-parameter -O2
LDFLAGS := -lm
DBGFLAGS := -std=c99 -Wall -Wextra -Wpedantic -Wno-unused-parameter -g -O0 -DDEBUG

INCDIR  := include
SRCDIR  := src
EXDIR   := examples
DEMDIR  := demos
BLDDIR  := build

HEADERS := $(INCDIR)/render_pipeline.h \
           $(INCDIR)/event_loop.h \
           $(INCDIR)/js_engine_sim.h \
           $(INCDIR)/composite_layer.h \
           $(INCDIR)/concurrency_browser.h

SOURCES := $(SRCDIR)/render_pipeline.c \
           $(SRCDIR)/event_loop.c \
           $(SRCDIR)/js_engine_sim.c \
           $(SRCDIR)/composite_layer.c \
           $(SRCDIR)/concurrency_browser.c

OBJECTS := $(patsubst $(SRCDIR)/%.c,$(BLDDIR)/%.o,$(SOURCES))

EXAMPLES := $(BLDDIR)/demo01_frame_loop \
            $(BLDDIR)/demo02_js_engine \
            $(BLDDIR)/demo03_browser_process

DEMOS := $(BLDDIR)/full_browser_sim \
         $(BLDDIR)/stress_benchmark

.PHONY: all clean examples demos debug

all: $(BLDDIR) examples demos

$(BLDDIR):
	mkdir -p $(BLDDIR)

$(BLDDIR)/%.o: $(SRCDIR)/%.c $(HEADERS) | $(BLDDIR)
	$(CC) $(CFLAGS) -I$(INCDIR) -c $< -o $@

$(BLDDIR)/demo01_frame_loop: $(EXDIR)/demo01_frame_loop.c $(OBJECTS) | $(BLDDIR)
	$(CC) $(CFLAGS) -I$(INCDIR) $< $(OBJECTS) $(LDFLAGS) -o $@

$(BLDDIR)/demo02_js_engine: $(EXDIR)/demo02_js_engine.c $(OBJECTS) | $(BLDDIR)
	$(CC) $(CFLAGS) -I$(INCDIR) $< $(OBJECTS) $(LDFLAGS) -o $@

$(BLDDIR)/demo03_browser_process: $(EXDIR)/demo03_browser_process.c $(OBJECTS) | $(BLDDIR)
	$(CC) $(CFLAGS) -I$(INCDIR) $< $(OBJECTS) $(LDFLAGS) -o $@

$(BLDDIR)/full_browser_sim: $(DEMDIR)/full_browser_sim.c $(OBJECTS) | $(BLDDIR)
	$(CC) $(CFLAGS) -I$(INCDIR) $< $(OBJECTS) $(LDFLAGS) -o $@

$(BLDDIR)/stress_benchmark: $(DEMDIR)/stress_benchmark.c $(OBJECTS) | $(BLDDIR)
	$(CC) $(CFLAGS) -I$(INCDIR) $< $(OBJECTS) $(LDFLAGS) -o $@

examples: $(EXAMPLES)

demos: $(DEMOS)

debug: CFLAGS = $(DBGFLAGS)
debug: all

clean:
	rm -rf $(BLDDIR)

run-examples: examples
	@echo "=== Running examples ==="
	$(BLDDIR)/demo01_frame_loop
	@echo ""
	$(BLDDIR)/demo02_js_engine
	@echo ""
	$(BLDDIR)/demo03_browser_process

run-demos: demos
	@echo "=== Running demos ==="
	$(BLDDIR)/full_browser_sim
	@echo ""
	$(BLDDIR)/stress_benchmark

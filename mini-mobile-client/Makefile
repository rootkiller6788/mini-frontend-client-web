CC       = gcc
CFLAGS   = -std=c99 -Wall -Wextra -O2 -Iinclude
LDFLAGS  =
AR       = ar
ARFLAGS  = rcs

SRCDIR   = src
INCDIR   = include
EXDIR    = examples
DEMDIR   = demos
BUILDDIR = build

HEADERS  = $(INCDIR)/react_native_bridge.h \
           $(INCDIR)/native_modules.h \
           $(INCDIR)/yoga_layout.h \
           $(INCDIR)/list_virtualizer.h \
           $(INCDIR)/push_notif.h

SOURCES  = $(SRCDIR)/react_native_bridge.c \
           $(SRCDIR)/native_modules.c \
           $(SRCDIR)/yoga_layout.c \
           $(SRCDIR)/list_virtualizer.c \
           $(SRCDIR)/push_notif.c

OBJECTS  = $(patsubst $(SRCDIR)/%.c,$(BUILDDIR)/%.o,$(SOURCES))
LIBRARY  = $(BUILDDIR)/libmini_mobile.a

EXAMPLES = $(EXDIR)/example1_bridge \
           $(EXDIR)/example2_modules \
           $(EXDIR)/example3_layout

DEMOS    = $(DEMDIR)/demo1_basic \
           $(DEMDIR)/demo2_full

.PHONY: all examples demos clean

all: $(LIBRARY)

$(LIBRARY): $(OBJECTS)
	@mkdir -p $(BUILDDIR)
	$(AR) $(ARFLAGS) $@ $^

$(BUILDDIR)/%.o: $(SRCDIR)/%.c $(HEADERS)
	@mkdir -p $(BUILDDIR)
	$(CC) $(CFLAGS) -c -o $@ $<

examples: $(LIBRARY) $(EXAMPLES)

$(EXDIR)/example1_bridge: $(EXDIR)/example1_bridge.c $(LIBRARY)
	$(CC) $(CFLAGS) -o $@ $< -L$(BUILDDIR) -lmini_mobile

$(EXDIR)/example2_modules: $(EXDIR)/example2_modules.c $(LIBRARY)
	$(CC) $(CFLAGS) -o $@ $< -L$(BUILDDIR) -lmini_mobile

$(EXDIR)/example3_layout: $(EXDIR)/example3_layout.c $(LIBRARY)
	$(CC) $(CFLAGS) -o $@ $< -L$(BUILDDIR) -lmini_mobile

demos: $(LIBRARY) $(DEMOS)

$(DEMDIR)/demo1_basic: $(DEMDIR)/demo1_basic.c $(LIBRARY)
	$(CC) $(CFLAGS) -o $@ $< -L$(BUILDDIR) -lmini_mobile

$(DEMDIR)/demo2_full: $(DEMDIR)/demo2_full.c $(LIBRARY)
	$(CC) $(CFLAGS) -o $@ $< -L$(BUILDDIR) -lmini_mobile

clean:
	rm -rf $(BUILDDIR)
	rm -f $(EXAMPLES)
	rm -f $(DEMOS)

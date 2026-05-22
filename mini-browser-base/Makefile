CC       = gcc
CFLAGS   = -std=c99 -Wall -Wextra -O2
LDFLAGS  =

SRCS     = http_cache.c cookie_storage.c event_system.c fetch_model.c web_storage.c
HDRS     = http_cache.h cookie_storage.h event_system.h fetch_model.h web_storage.h
OBJS     = $(SRCS:.c=.o)

EXAMPLES = example_http_cache.exe example_cookie_storage.exe example_event_system.exe
DEMOS    = demo_browser_shell.exe demo_web_page.exe

.PHONY: all clean examples demos

all: examples demos

examples: $(EXAMPLES)

demos: $(DEMOS)

%.o: %.c $(HDRS)
	$(CC) $(CFLAGS) -c $< -o $@

example_http_cache.exe: example_http_cache.c http_cache.c http_cache.h
	$(CC) $(CFLAGS) $< http_cache.c -o $@ $(LDFLAGS)

example_cookie_storage.exe: example_cookie_storage.c cookie_storage.c cookie_storage.h
	$(CC) $(CFLAGS) $< cookie_storage.c -o $@ $(LDFLAGS)

example_event_system.exe: example_event_system.c event_system.c event_system.h
	$(CC) $(CFLAGS) $< event_system.c -o $@ $(LDFLAGS)

demo_browser_shell.exe: demo_browser_shell.c $(SRCS) $(HDRS)
	$(CC) $(CFLAGS) demo_browser_shell.c $(SRCS) -o $@ $(LDFLAGS)

demo_web_page.exe: demo_web_page.c $(SRCS) $(HDRS)
	$(CC) $(CFLAGS) demo_web_page.c $(SRCS) -o $@ $(LDFLAGS)

clean:
	erase /Q *.o *.exe demo_ls_data.txt demo_web_page_ls.txt 2>nul || exit 0

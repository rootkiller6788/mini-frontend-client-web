CC       = gcc
STD      = -std=c99
CFLAGS   = -Wall -Wextra -O2
LDFLAGS  = -lm
AR       = ar
ARFLAGS  = rcs

# ── Object files ──────────────────────────────────────────────────────────
OBJS = redux_store.o mobx_observable.o zustand_sim.o query_cache.o immutable_store.o
HEADERS = redux_store.h mobx_observable.h zustand_sim.h query_cache.h immutable_store.h
LIB = libmini_state_data.a

# ── Target binaries ───────────────────────────────────────────────────────
EXAMPLES = example_todo.exe example_async.exe example_collab.exe
DEMOS = demo_redux.exe demo_query.exe

# ── Primary targets ───────────────────────────────────────────────────────
.PHONY: all clean examples demos lib

all: lib examples demos

lib: $(LIB)

# ── Static library ────────────────────────────────────────────────────────
$(LIB): $(OBJS)
	$(AR) $(ARFLAGS) $@ $^

# ── Object compilation ────────────────────────────────────────────────────
redux_store.o: redux_store.c redux_store.h
	$(CC) $(STD) $(CFLAGS) -c $< -o $@

mobx_observable.o: mobx_observable.c mobx_observable.h
	$(CC) $(STD) $(CFLAGS) -c $< -o $@

zustand_sim.o: zustand_sim.c zustand_sim.h
	$(CC) $(STD) $(CFLAGS) -c $< -o $@

query_cache.o: query_cache.c query_cache.h
	$(CC) $(STD) $(CFLAGS) -c $< -o $@

immutable_store.o: immutable_store.c immutable_store.h
	$(CC) $(STD) $(CFLAGS) -c $< -o $@

# ── Examples ──────────────────────────────────────────────────────────────
examples: $(EXAMPLES)

example_todo.exe: example_todo.c $(LIB) $(HEADERS)
	$(CC) $(STD) $(CFLAGS) $< -L. -lmini_state_data $(LDFLAGS) -o $@

example_async.exe: example_async.c $(LIB) $(HEADERS)
	$(CC) $(STD) $(CFLAGS) $< -L. -lmini_state_data $(LDFLAGS) -o $@

example_collab.exe: example_collab.c $(LIB) $(HEADERS)
	$(CC) $(STD) $(CFLAGS) $< -L. -lmini_state_data $(LDFLAGS) -o $@

# ── Demos ─────────────────────────────────────────────────────────────────
demos: $(DEMOS)

demo_redux.exe: demo_redux.c $(LIB) $(HEADERS)
	$(CC) $(STD) $(CFLAGS) $< -L. -lmini_state_data $(LDFLAGS) -o $@

demo_query.exe: demo_query.c $(LIB) $(HEADERS)
	$(CC) $(STD) $(CFLAGS) $< -L. -lmini_state_data $(LDFLAGS) -o $@

# ── Convenience: build individual objects ─────────────────────────────────
redux: redux_store.o
mobx: mobx_observable.o
zustand: zustand_sim.o
query: query_cache.o
immutable: immutable_store.o

# ── Clean ─────────────────────────────────────────────────────────────────
clean:
	del /f /q *.o *.a *.exe 2>nul || rm -f *.o *.a *.exe

# ── Help ──────────────────────────────────────────────────────────────────
help:
	@echo "make          - build library, examples, and demos"
	@echo "make lib      - build static library only"
	@echo "make examples - build example programs"
	@echo "make demos    - build demo programs"
	@echo "make clean    - remove all build artifacts"
	@echo "make redux    - build only redux_store.o"
	@echo "make mobx     - build only mobx_observable.o"
	@echo "make zustand  - build only zustand_sim.o"
	@echo "make query    - build only query_cache.o"
	@echo "make immutable- build only immutable_store.o"

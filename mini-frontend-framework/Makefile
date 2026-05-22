CC       = gcc
CFLAGS   = -std=c99 -Wall -Wextra -pedantic -O2
LDFLAGS  =
TARGETS  = example_basic example_todo example_router demo_perf demo_reactive
OBJECTS  = virtual_dom.o react_hooks.o component_model.o spa_router.o reactive_system.o
HEADERS  = virtual_dom.h react_hooks.h component_model.h spa_router.h reactive_system.h

.PHONY: all clean run-basic run-todo run-router run-perf run-reactive run-all

all: $(TARGETS)

virtual_dom.o: virtual_dom.c virtual_dom.h
	$(CC) $(CFLAGS) -c virtual_dom.c -o virtual_dom.o

react_hooks.o: react_hooks.c react_hooks.h
	$(CC) $(CFLAGS) -c react_hooks.c -o react_hooks.o

component_model.o: component_model.c component_model.h virtual_dom.h react_hooks.h
	$(CC) $(CFLAGS) -c component_model.c -o component_model.o

spa_router.o: spa_router.c spa_router.h
	$(CC) $(CFLAGS) -c spa_router.c -o spa_router.o

reactive_system.o: reactive_system.c reactive_system.h
	$(CC) $(CFLAGS) -c reactive_system.c -o reactive_system.o

example_basic: example_basic.c $(OBJECTS) $(HEADERS)
	$(CC) $(CFLAGS) example_basic.c $(OBJECTS) -o example_basic $(LDFLAGS)

example_todo: example_todo.c $(OBJECTS) $(HEADERS)
	$(CC) $(CFLAGS) example_todo.c $(OBJECTS) -o example_todo $(LDFLAGS)

example_router: example_router.c $(OBJECTS) $(HEADERS)
	$(CC) $(CFLAGS) example_router.c $(OBJECTS) -o example_router $(LDFLAGS)

demo_perf: demo_perf.c $(OBJECTS) $(HEADERS)
	$(CC) $(CFLAGS) demo_perf.c $(OBJECTS) -o demo_perf $(LDFLAGS)

demo_reactive: demo_reactive.c $(OBJECTS) $(HEADERS)
	$(CC) $(CFLAGS) demo_reactive.c $(OBJECTS) -o demo_reactive $(LDFLAGS)

libframework.a: $(OBJECTS)
	ar rcs libframework.a $(OBJECTS)

run-basic: example_basic
	./example_basic

run-todo: example_todo
	./example_todo

run-router: example_router
	./example_router

run-perf: demo_perf
	./demo_perf

run-reactive: demo_reactive
	./demo_reactive

run-all: $(TARGETS)
	@echo "=== Running example_basic ==="
	./example_basic
	@echo ""
	@echo "=== Running example_todo ==="
	./example_todo
	@echo ""
	@echo "=== Running example_router ==="
	./example_router
	@echo ""
	@echo "=== Running demo_perf ==="
	./demo_perf
	@echo ""
	@echo "=== Running demo_reactive ==="
	./demo_reactive

clean:
	del /f virtual_dom.o react_hooks.o component_model.o spa_router.o reactive_system.o 2>nul
	del /f example_basic.exe example_todo.exe example_router.exe demo_perf.exe demo_reactive.exe 2>nul
	del /f libframework.a 2>nul

distclean: clean

help:
	@echo "Mini Frontend Framework - Build Targets"
	@echo ""
	@echo "  make all           Build all targets"
	@echo "  make example_basic Build basic example"
	@echo "  make example_todo  Build todo example"
	@echo "  make example_router Build router example"
	@echo "  make demo_perf     Build performance demo"
	@echo "  make demo_reactive Build reactive demo"
	@echo "  make libframework.a Build static library"
	@echo "  make run-basic     Build and run basic example"
	@echo "  make run-todo      Build and run todo example"
	@echo "  make run-router    Build and run router example"
	@echo "  make run-perf      Build and run performance demo"
	@echo "  make run-reactive  Build and run reactive demo"
	@echo "  make run-all       Build and run all examples"
	@echo "  make clean         Remove build artifacts"
	@echo "  make help          Show this help"

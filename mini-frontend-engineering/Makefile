# mini-frontend-engineering Makefile
# C99 frontend engineering toolchain

CC       = gcc
CFLAGS   = -std=c99 -Wall -Wextra -pedantic -O2
LDFLAGS  =

SRC_DIR  = .
BUILD_DIR = build
DIST_DIR = dist

HEADERS  = bundler_core.h tree_shaker.h hmr_engine.h ssr_model.h asset_pipeline.h
SOURCES  = bundler_core.c tree_shaker.c hmr_engine.c ssr_model.c asset_pipeline.c
EXAMPLES = example_bundle.c example_shake.c example_pipeline.c
DEMOS    = demo_hmr.c demo_ssr.c
DOCS     = doc_api.md doc_guide.md

OBJS     = $(SOURCES:.c=.o)
EXE_EXAMPLES = $(EXAMPLES:.c=.exe)
EXE_DEMOS    = $(DEMOS:.c=.exe)

.PHONY: all clean run-example-bundle run-example-shake run-example-pipeline run-demo-hmr run-demo-ssr

all: $(EXE_EXAMPLES) $(EXE_DEMOS)

$(BUILD_DIR):
	mkdir $(BUILD_DIR)

%.o: %.c $(HEADERS)
	$(CC) $(CFLAGS) -c $< -o $@

bundler_core.o: bundler_core.c bundler_core.h
	$(CC) $(CFLAGS) -c bundler_core.c -o bundler_core.o

tree_shaker.o: tree_shaker.c tree_shaker.h
	$(CC) $(CFLAGS) -c tree_shaker.c -o tree_shaker.o

hmr_engine.o: hmr_engine.c hmr_engine.h
	$(CC) $(CFLAGS) -c hmr_engine.c -o hmr_engine.o

ssr_model.o: ssr_model.c ssr_model.h
	$(CC) $(CFLAGS) -c ssr_model.c -o ssr_model.o

asset_pipeline.o: asset_pipeline.c asset_pipeline.h
	$(CC) $(CFLAGS) -c asset_pipeline.c -o asset_pipeline.o

example_bundle.exe: example_bundle.c bundler_core.o
	$(CC) $(CFLAGS) example_bundle.c bundler_core.o $(LDFLAGS) -o example_bundle.exe

example_shake.exe: example_shake.c tree_shaker.o
	$(CC) $(CFLAGS) example_shake.c tree_shaker.o $(LDFLAGS) -o example_shake.exe

example_pipeline.exe: example_pipeline.c asset_pipeline.o
	$(CC) $(CFLAGS) example_pipeline.c asset_pipeline.o $(LDFLAGS) -o example_pipeline.exe

demo_hmr.exe: demo_hmr.c hmr_engine.o
	$(CC) $(CFLAGS) demo_hmr.c hmr_engine.o $(LDFLAGS) -o demo_hmr.exe

demo_ssr.exe: demo_ssr.c ssr_model.o
	$(CC) $(CFLAGS) demo_ssr.c ssr_model.o $(LDFLAGS) -o demo_ssr.exe

run-example-bundle: example_bundle.exe
	./example_bundle.exe

run-example-shake: example_shake.exe
	./example_shake.exe

run-example-pipeline: example_pipeline.exe
	./example_pipeline.exe

run-demo-hmr: demo_hmr.exe
	./demo_hmr.exe

run-demo-ssr: demo_ssr.exe
	./demo_ssr.exe

clean:
	del /f *.o 2>nul
	del /f *.exe 2>nul
	del /f *.obj 2>nul
	rd /s /q example_src 2>nul
	rd /s /q assets 2>nul
	rd /s /q dist 2>nul
	rd /s /q dist_assets 2>nul
	rd /s /q ssg_output 2>nul
	rd /s /q $(BUILD_DIR) 2>nul
	del /f *.out 2>nul

info:
	@echo "mini-frontend-engineering Build System"
	@echo "====================================="
	@echo "CC       = $(CC)"
	@echo "CFLAGS   = $(CFLAGS)"
	@echo "Sources  = $(SOURCES)"
	@echo "Examples = $(EXAMPLES)"
	@echo "Demos    = $(DEMOS)"
	@echo ""
	@echo "Targets:"
	@echo "  all                  Build everything"
	@echo "  clean                Remove all build artifacts"
	@echo "  run-example-bundle   Run module bundler example"
	@echo "  run-example-shake    Run tree shaker example"
	@echo "  run-example-pipeline Run asset pipeline example"
	@echo "  run-demo-hmr         Run HMR demo"
	@echo "  run-demo-ssr         Run SSR/SSG demo"
	@echo "  info                 Show this help"

help: info

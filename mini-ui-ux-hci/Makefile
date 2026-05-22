CC      = gcc
CFLAGS  = -std=c99 -Wall -Wextra -pedantic -O2 -Iinclude
LDFLAGS = -lm
BUILD   = build
SRC     = src
EXAMPLES = examples
DEMOS   = demos

SRCS = $(SRC)/design_tokens.c $(SRC)/component_library.c $(SRC)/accessibility.c $(SRC)/interaction_model.c $(SRC)/theme_system.c
OBJS = $(SRCS:$(SRC)/%.c=$(BUILD)/%.o)
HDRS = include/design_tokens.h include/component_library.h include/accessibility.h include/interaction_model.h include/theme_system.h

EXAMPLES_SRC = $(EXAMPLES)/01_design_tokens_demo.c $(EXAMPLES)/02_component_form.c $(EXAMPLES)/03_accessibility_check.c
EXAMPLES_BIN = $(EXAMPLES_SRC:$(EXAMPLES)/%.c=$(BUILD)/%.exe)

DEMOS_SRC = $(DEMOS)/full_ui_kit_demo.c $(DEMOS)/theme_storybook.c
DEMOS_BIN = $(DEMOS_SRC:$(DEMOS)/%.c=$(BUILD)/%.exe)

.PHONY: all clean examples demos run-demos run-examples

all: $(BUILD) $(OBJS)
	@echo "=== mini-ui-ux-hci library built ==="
	@echo "Object files in $(BUILD)/"
	@echo "Run 'make examples' to build examples"
	@echo "Run 'make demos' to build demos"

$(BUILD):
	mkdir -p "$(BUILD)"

$(BUILD)/%.o: $(SRC)/%.c $(HDRS) | $(BUILD)
	$(CC) $(CFLAGS) -c $< -o $@

examples: $(BUILD) $(OBJS) $(EXAMPLES_BIN)
	@echo "=== Examples built ==="

$(BUILD)/01_design_tokens_demo.exe: $(EXAMPLES)/01_design_tokens_demo.c $(OBJS) | $(BUILD)
	$(CC) $(CFLAGS) $< $(OBJS) -o $@ $(LDFLAGS)

$(BUILD)/02_component_form.exe: $(EXAMPLES)/02_component_form.c $(OBJS) | $(BUILD)
	$(CC) $(CFLAGS) $< $(OBJS) -o $@ $(LDFLAGS)

$(BUILD)/03_accessibility_check.exe: $(EXAMPLES)/03_accessibility_check.c $(OBJS) | $(BUILD)
	$(CC) $(CFLAGS) $< $(OBJS) -o $@ $(LDFLAGS)

demos: $(BUILD) $(OBJS) $(DEMOS_BIN)
	@echo "=== Demos built ==="

$(BUILD)/full_ui_kit_demo.exe: $(DEMOS)/full_ui_kit_demo.c $(OBJS) | $(BUILD)
	$(CC) $(CFLAGS) $< $(OBJS) -o $@ $(LDFLAGS)

$(BUILD)/theme_storybook.exe: $(DEMOS)/theme_storybook.c $(OBJS) | $(BUILD)
	$(CC) $(CFLAGS) $< $(OBJS) -o $@ $(LDFLAGS)

run-examples: examples
	@echo "Running examples..."
	$(BUILD)\01_design_tokens_demo.exe
	$(BUILD)\02_component_form.exe
	$(BUILD)\03_accessibility_check.exe

run-demos: demos
	@echo "Running demos..."
	$(BUILD)\full_ui_kit_demo.exe
	$(BUILD)\theme_storybook.exe

storybook: $(BUILD)/theme_storybook.exe
	$(BUILD)\theme_storybook.exe

clean:
	rm -rf "$(BUILD)"

# Target to build
TARGET := pl0

# Compiler flags to use
override CFLAGS += -D_GNU_SOURCE=1 -Wall -Wextra -Wno-unused-function -Werror -I.
override OFLAGS += -O2
override LDFLAGS +=

# Directory where build products are stored
BUILD := build
SRC_DIRS := lexer compiler compiler/parser compiler/codegen vm
SRCS := $(wildcard *.c) $(foreach d,$(SRC_DIRS),$(wildcard $d/*.c))
OBJS := $(addprefix $(BUILD)/,$(SRCS:.c=.o))
DEPS := $(OBJS:.o=.d)
HEADERS := $(wildcard *.h) $(foreach d,$(SRC_DIRS),$(wildcard $d/*.h))
BUILD_DIRS := $(BUILD) $(addprefix $(BUILD)/,$(SRC_DIRS))
BUILD_DIR_RULES := $(addsuffix /.dir,$(BUILD_DIRS))
ZIP := pl0.zip
GRAPH_DOTS := lexer.dot ast.dot unoptimized_cfg.dot cfg.dot
GRAPH_PDFS := $(GRAPH_DOTS:.dot=.pdf)
GRAPH_FILES := $(GRAPH_DOTS) $(GRAPH_PDFS)
TEST_CASES := $(shell find test_cases -type f)
OUTPUTS := lexemetable.txt cleaninput.txt tokenlist.txt symboltable.txt mcode.txt acode.txt stacktrace.txt $(GRAPH_FILES)
DATA := input.txt $(OUTPUTS)
DOCS := PL0_Reference_Manual_Version_3.pdf PL0_Reference_Manual_Version_3.docx Scanner_and_Parser_Error.pdf
RESOURCES := .gitignore Makefile README.md $(DOCS) $(DATA) $(TEST_CASES) $(SRCS) $(HEADERS)

# Compiler and linker to use. Would prefer to use clang, but it's not installed on eustis
CC := gcc
LD := gcc

# Graphviz renderer to use
GV := dot

# Print all commands executed when VERBOSE is defined
ifdef VERBOSE
_v :=
else
_v := @
endif


# Build the target by default
all: $(TARGET)

# Build in debug mode (with asserts enabled)
debug: override CFLAGS += -ggdb -DDEBUG=1 -UNDEBUG
debug: override OFLAGS :=
debug: $(TARGET)

# Uses clang's Address Sanitizer to help detect memory errors
debug+: override CC := clang
debug+: override LD := clang
debug+: override CFLAGS += -fsanitize=address
debug+: override LDFLAGS += -fsanitize=address
debug+: debug


# Linking rule

$(TARGET): $(OBJS)
	@echo 'Linking $@'
	$(_v)$(LD) $(LDFLAGS) -o $@ $^


# Compiling rule

$(BUILD)/%.o: %.c | $(BUILD_DIR_RULES)
	@echo 'Compiling $<'
	$(_v)$(CC) $(CFLAGS) $(OFLAGS) -I$(<D) -MD -MP -MF $(BUILD)/$*.d -c -o $@ $<


# Build dependency rules
-include $(DEPS)


# Rules for creating output files by running the built products
run: debug
	@echo 'Running PL/0 Compiler Driver'
	$(_v)./$(TARGET)


# Archiving rules

archive: $(ZIP)

$(ZIP): $(RESOURCES)
	@echo 'Compressing $@'
	$(_v)zip $@ $^


# Graph rules

graph: $(GRAPH_PDFS)

%.pdf: %.dot
	@echo 'Rendering $* graph with $(GV)'
	$(_v)$(GV) -Tpdf -o$@ $<

# Crazy stupid way of telling the Makefile that running the executable makes all these outputs
unoptimized_cfg.dot: cfg.dot
cfg.dot: ast.dot
ast.dot: lexer.dot
lexer.dot: mcode.txt
mcode.txt: symboltable.txt
symboltable.txt: tokenlist.txt
tokenlist.txt: lexemetable.txt
lexemetable.txt: cleaninput.txt
cleaninput.txt: input.txt debug
	@echo 'Running PL/0 Lexer and Compiler'
	$(_v)./$(TARGET) -c

stacktrace.txt: acode.txt
acode.txt: mcode.txt debug
	@echo 'Running PM/0 VM'
	$(_v)./$(TARGET) -r


# Make sure that the .dir files aren't automatically deleted after building
.SECONDARY:

%/.dir:
	$(_v)mkdir -p $* && touch $@

clean:
	@echo 'Removing built products'
	$(_v)rm -rf $(BUILD) $(TARGET) $(ZIP) $(OUTPUTS)

.PHONY: all debug debug+ run archive graph clean

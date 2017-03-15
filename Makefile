# Target to build
TARGET := pl0

# Compiler flags to use
override CFLAGS += -D_GNU_SOURCE=1 -Wall -Wextra -Wno-unused-function -Werror -I.
override OFLAGS += -O2
override LDFLAGS +=
override YFLAGS += -Wall -Werror

# Build with support for the Bison parser generator
override CFLAGS += -DWITH_BISON=1


# Directory where build products are stored
BUILD := build

# Directory where generated source files are produced
GEN := $(BUILD)/gen

# All directories that contain source files
SRC_DIRS := lexer compiler compiler/parser compiler/codegen vm

# All C source files
SRCS := $(wildcard *.c) $(foreach d,$(SRC_DIRS),$(wildcard $d/*.c))

# Bison parser generator input Y files
BISON_FILES := $(wildcard compiler/parser/*.y)

# Bison parser generator output C and H files
BISON_C_FILES := $(patsubst %,$(GEN)/%.c,$(BISON_FILES))
BISON_H_FILES := $(patsubst %,$(GEN)/%.h,$(BISON_FILES))
BISON_OBJS := $(patsubst $(GEN)/%,$(BUILD)/%.o,$(BISON_C_FILES))

# Object files that need to be produced from C sources
OBJS := $(patsubst %,$(BUILD)/%.o,$(SRCS)) $(BISON_OBJS)

# Dependency files that are produced during compilation
DEPS := $(OBJS:.o=.d)

# Header files that should be included in the produced archive
HEADERS := $(wildcard *.h) $(foreach d,$(SRC_DIRS),$(wildcard $d/*.h))

# All build directories that will be produced
BUILD_DIRS := $(BUILD) $(addprefix $(BUILD)/,$(SRC_DIRS)) $(addprefix $(GEN)/,$(SRC_DIRS))

# .dir files in every build directory
BUILD_DIR_FILES := $(addsuffix /.dir,$(BUILD_DIRS))


## Archive variables

# Name of archive file
ZIP := pl0.zip

# Dot files produced during runtime that Graphviz should render
GRAPH_DOTS := lexer.dot ast.dot unoptimized_cfg.dot cfg.dot

# PDF files rendered by Graphviz
GRAPH_PDFS := $(GRAPH_DOTS:.dot=.pdf)

# All Graphviz input and output files
GRAPH_FILES := $(GRAPH_DOTS) $(GRAPH_PDFS)

# All test case files
TEST_CASES := $(shell find test_cases -type f)

# All output files produced at runtime
OUTPUTS := lexemetable.txt cleaninput.txt tokenlist.txt symboltable.txt mcode.txt acode.txt stacktrace.txt $(GRAPH_FILES)

# All input and output files for the program
DATA := input.txt $(OUTPUTS)

# All required documentation files
DOCS := PL0_Reference_Manual_Version_3.pdf PL0_Reference_Manual_Version_3.docx Scanner_and_Parser_Error.pdf

# All resources that should be included in the archive
RESOURCES := .gitignore Makefile README.md $(DOCS) $(DATA) $(TEST_CASES) $(SRCS) $(HEADERS)


## Build settings

# Compiler and linker to use. Would prefer to use clang, but it's not installed on eustis
CC := gcc
LD := gcc
BISON := bison

# Graphviz renderer to use
GV := dot

# Print all commands executed when VERBOSE is defined
ifdef VERBOSE
_v :=
else
_v := @
endif


## Build rules

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
$(BUILD)/%.o: % | $(BUILD_DIR_FILES) $(BISON_C_FILES) $(BISON_H_FILES)
	@echo 'Compiling $<'
	$(_v)$(CC) $(CFLAGS) $(OFLAGS) -I$(<D) -I$(GEN) -MD -MP -MF $(BUILD)/$*.d -c -o $@ $<

# Compiling rule for generated sources
$(BUILD)/%.o: $(GEN)/% | $(BUILD_DIR_FILES)
	@echo 'Compiling $<'
	$(_v)$(CC) $(CFLAGS) $(OFLAGS) -I$(<D) -I$(GEN) -MD -MP -MF $(BUILD)/$*.d -c -o $@ $<

# Bison parser generation rule
$(GEN)/%.c $(GEN)/%.h: % | $(BUILD_DIR_FILES)
	@echo 'Bison $<'
	$(_v)$(BISON) $(YFLAGS) --output=$(GEN)/$*.c --defines=$(GEN)/$*.h $<

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

# Crazy stupid way of telling make that running the executable makes all these outputs
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

# Target to build
TARGET := pl0

# Compiler flags to use
override CFLAGS += \
	-std=gnu99 \
	-D_GNU_SOURCE=1 \
	-Wall \
	-Wextra \
	-Werror \
	-Wno-unused-function \
	-I.

override OFLAGS += -O2 -flto
override LDFLAGS +=
override STRIP_FLAGS += -Wl,-S -Wl,-x
override YFLAGS += -Wall -Werror


# Directory where build products are stored
BUILD := build

# Directory where generated source files are produced
GEN := $(BUILD)/gen

# All directories that contain source files
SRC_DIRS := \
	lexer \
	compiler \
	compiler/parser \
	compiler/codegen \
	compiler/codegen/pm0 \
	vm

# If we are building with support for LLVM code generation
ifdef WITH_LLVM

# Add the LLVM sources
SRC_DIRS := $(SRC_DIRS) compiler/codegen/llvm

endif #WITH_LLVM

# find_srcs(ext) -> file paths
# Macro to find all files with the given extension in SRC_DIRS
find_srcs = $(wildcard *.$1) $(foreach d,$(SRC_DIRS),$(wildcard $d/*.$1))

# All supported source file extensions
SRC_EXTS := c cpp

# All source files
SRCS := $(foreach ext,$(SRC_EXTS),$(call find_srcs,$(ext)))

# Object files that need to be produced from sources
OBJS := $(patsubst %,$(BUILD)/%.o,$(SRCS))

# Bison parser generator input Y files
BISON_FILES := $(call find_srcs,y)

# Build with support for the Bison parser generator
ifdef WITH_BISON

# Bison parser generator output C and H files
BISON_C_FILES := $(patsubst %,$(GEN)/%.c,$(BISON_FILES))
BISON_H_FILES := $(patsubst %,$(GEN)/%.h,$(BISON_FILES))
BISON_OBJS := $(patsubst $(GEN)/%,$(BUILD)/%.o,$(BISON_C_FILES))

# Append Bison object files
OBJS := $(OBJS) $(BISON_OBJS)

# Rule dependencies for all compilation rules
PRE_CC := $(BISON_C_FILES) $(BISON_H_FILES)

else #WITH_BISON

# Empty
PRE_CC :=

endif #WITH_BISON

# Dependency files that are produced during compilation
DEPS := $(OBJS:.o=.d)

# Header files that should be included in the produced archive
HEADERS := $(call find_srcs,h)

# All build directories that will be produced
BUILD_DIRS := $(BUILD) $(addprefix $(BUILD)/,$(SRC_DIRS)) $(addprefix $(GEN)/,$(SRC_DIRS))

# .dir files in every build directory
BUILD_DIR_FILES := $(addsuffix /.dir,$(BUILD_DIRS))


## Archive variables

# Name of archive file
ZIP := pl0.zip

# Dot files produced during runtime that Graphviz should render
GRAPH_DOTS := lexer.dot ast.dot cfg.dot

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

# All resources that should be included in the archive
RESOURCES := .gitignore Makefile README.md $(DATA) $(TEST_CASES) $(BISON_FILES) $(SRCS) $(HEADERS)


## Build settings

# Compiler choices
GCC := gcc
G++ := g++

CLANG := clang
CLANG++ := clang++

BISON := bison

# Compilers and linker to use
ifdef USE_GCC
CC := $(GCC)
CXX := $(G++)
LD := $(GCC)
LD++ := $(G++)
else #USE_GCC
CC := $(CLANG)
CXX := $(CLANG++)
LD := $(CLANG)
LD++ := $(CLANG++)
endif #USE_GCC

# Parser generator to use
YACC := $(BISON)

# Graphviz renderer to use
GV := dot


# Apply build configuration changes needed for building with Bison
ifdef WITH_BISON

# Set WITH_BISON macro for conditional compilation sections
override CFLAGS += -DWITH_BISON=1

endif #WITH_BISON


# Apply build configuration changes needed for building with LLVM
ifdef WITH_LLVM

# Get the flags needed for building with the LLVM API
LLVM_CFLAGS := $(shell llvm-config --cflags)
LLVM_LDFLAGS := $(shell llvm-config --ldflags)

LLVM_CFLAGS_REMOVE := \
	-pedantic \
	-Wvariadic-macros \
	-Wstring-conversion \
	-Wgnu-statement-expression \
	-Wgnu-conditional-omitted-operand \
	-Wcovered-switch-default

LLVM_CFLAGS := $(filter-out $(LLVM_CFLAGS_REMOVE),$(LLVM_CFLAGS))

# Set WITH_LLVM macro for conditional compilation sections
override CFLAGS := -DWITH_LLVM=1 $(LLVM_CFLAGS) $(CFLAGS)
override LDFLAGS := $(LDFLAGS) $(LLVM_LDFLAGS)

# Linking against LLVM requires a C++ linker (even though this is all C)
override LD := $(LD++)

endif #WITH_LLVM


# Print all commands executed when VERBOSE is defined
ifdef VERBOSE
_v :=
else #VERBOSE
_v := @
endif #VERBOSE


## Build rules

# Build the target by default
all: $(TARGET)

# Build in debug mode (with asserts enabled)
debug: override CFLAGS += -ggdb -DDEBUG=1 -UNDEBUG
debug: override OFLAGS := -O0
debug: override STRIP_FLAGS :=
debug: $(TARGET)

# Uses clang's Address Sanitizer to help detect memory errors
debug+: override CC := $(CLANG)
debug+: override LD := $(CLANG)
debug+: override CFLAGS += -fsanitize=address
debug+: override LDFLAGS += -fsanitize=address
debug+: debug


# Linking rule
$(TARGET): $(OBJS)
	@echo 'Linking $@'
	$(_v)$(LD) $(LDFLAGS) $(OFLAGS) $(STRIP_FLAGS) -o $@ $^

# Compiling rule for C sources
$(BUILD)/%.c.o: %.c | $(BUILD_DIR_FILES) $(PRE_CC)
	@echo 'Compiling $<'
	$(_v)$(CC) $(CFLAGS) $(OFLAGS) -I$(<D) -I$(GEN) -MD -MP -MF $(BUILD)/$*.c.d -c -o $@ $<

# Compiling rule for generated C sources
$(BUILD)/%.c.o: $(GEN)/%.c | $(BUILD_DIR_FILES)
	@echo 'Compiling $<'
	$(_v)$(CC) $(CFLAGS) $(OFLAGS) -I$(<D) -I$(GEN) -MD -MP -MF $(BUILD)/$*.c.d -c -o $@ $<


ifdef WITH_BISON
# Bison parser generation rule
$(GEN)/%.c $(GEN)/%.h: % | $(BUILD_DIR_FILES)
	@echo 'Yacc $<'
	$(_v)$(YACC) $(YFLAGS) --output=$(GEN)/$*.c --defines=$(GEN)/$*.h $<

endif #WITH_BISON


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

# Disable stupid built-in rules
MAKEFLAGS += --no-builtin-rules
.SUFFIXES:

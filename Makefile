CC       ?= cc
CFLAGS   ?= -std=c11 -Wall -Wextra -Wpedantic -O2 -g
CPPFLAGS := -Iinclude
AR       ?= ar
INSTALL  ?= install

PREFIX      ?= /usr/local
DESTDIR     ?=
LIBDIR      ?= $(PREFIX)/lib
INCLUDEDIR  ?= $(PREFIX)/include
PKGCONFIGDIR ?= $(LIBDIR)/pkgconfig

# Version — derived from the header so releases don't need to touch this file.
VERSION_MAJOR := $(shell sed -n 's/^\#define CTESTPROBE_VERSION_MAJOR \([0-9]*\).*/\1/p' include/ctestprobe.h)
VERSION_MINOR := $(shell sed -n 's/^\#define CTESTPROBE_VERSION_MINOR \([0-9]*\).*/\1/p' include/ctestprobe.h)
VERSION_PATCH := $(shell sed -n 's/^\#define CTESTPROBE_VERSION_PATCH \([0-9]*\).*/\1/p' include/ctestprobe.h)
VERSION       := $(VERSION_MAJOR).$(VERSION_MINOR).$(VERSION_PATCH)

BUILD    := build
SRC      := src/ctestprobe.c
OBJ      := $(BUILD)/ctestprobe.o
LIB      := $(BUILD)/libctestprobe.a
PC       := $(BUILD)/ctestprobe.pc

SELF_TEST_SRC := tests/test_ctestprobe.c
SELF_TEST_BIN := $(BUILD)/test_ctestprobe

EXAMPLE_SRC := examples/example1.c
EXAMPLE_BIN := $(BUILD)/example1

.PHONY: all lib test example clean install uninstall pkgconfig asan ubsan tsan single-header

all: $(LIB) $(SELF_TEST_BIN) $(EXAMPLE_BIN) $(PC)

lib: $(LIB)

pkgconfig: $(PC)

test: $(SELF_TEST_BIN)
	./$(SELF_TEST_BIN)

example: $(EXAMPLE_BIN)
	./$(EXAMPLE_BIN)

$(LIB): $(OBJ)
	$(AR) rcs $@ $^

$(OBJ): $(SRC) include/ctestprobe.h | $(BUILD)
	$(CC) $(CFLAGS) $(CPPFLAGS) -c -o $@ $<

$(SELF_TEST_BIN): $(SELF_TEST_SRC) $(LIB) | $(BUILD)
	$(CC) $(CFLAGS) $(CPPFLAGS) -o $@ $< $(LIB) $(LDFLAGS)

$(EXAMPLE_BIN): $(EXAMPLE_SRC) $(LIB) | $(BUILD)
	$(CC) $(CFLAGS) $(CPPFLAGS) -o $@ $< $(LIB) $(LDFLAGS)

$(PC): ctestprobe.pc.in | $(BUILD)
	sed -e 's|@PREFIX@|$(PREFIX)|g' \
	    -e 's|@VERSION@|$(VERSION)|g' \
	    $< > $@

$(BUILD):
	mkdir -p $(BUILD)

install: $(LIB) $(PC)
	$(INSTALL) -d $(DESTDIR)$(LIBDIR)
	$(INSTALL) -d $(DESTDIR)$(INCLUDEDIR)
	$(INSTALL) -d $(DESTDIR)$(PKGCONFIGDIR)
	$(INSTALL) -m 0644 $(LIB) $(DESTDIR)$(LIBDIR)/libctestprobe.a
	$(INSTALL) -m 0644 include/ctestprobe.h $(DESTDIR)$(INCLUDEDIR)/ctestprobe.h
	$(INSTALL) -m 0644 $(PC) $(DESTDIR)$(PKGCONFIGDIR)/ctestprobe.pc

uninstall:
	rm -f $(DESTDIR)$(LIBDIR)/libctestprobe.a
	rm -f $(DESTDIR)$(INCLUDEDIR)/ctestprobe.h
	rm -f $(DESTDIR)$(PKGCONFIGDIR)/ctestprobe.pc

# Sanitizer targets: rebuild the whole tree with the relevant -fsanitize
# flags and run the self-test under the sanitizer. Because the flags
# affect codegen for both the library and its consumers, the tree must
# be built entirely with them — hence the recursive `make` with an
# overridden CFLAGS/LDFLAGS.
asan:
	$(MAKE) clean
	$(MAKE) test \
	    CFLAGS="-std=c11 -Wall -Wextra -Wpedantic -O1 -g -fsanitize=address,undefined -fno-omit-frame-pointer" \
	    LDFLAGS="-fsanitize=address,undefined"

ubsan:
	$(MAKE) clean
	$(MAKE) test \
	    CFLAGS="-std=c11 -Wall -Wextra -Wpedantic -O1 -g -fsanitize=undefined -fno-omit-frame-pointer" \
	    LDFLAGS="-fsanitize=undefined"

tsan:
	$(MAKE) clean
	$(MAKE) test \
	    CFLAGS="-std=c11 -Wall -Wextra -Wpedantic -O1 -g -fsanitize=thread -fno-omit-frame-pointer" \
	    LDFLAGS="-fsanitize=thread"

# Single-header (stb-style) amalgamation for drop-in consumers.
single-header: single_include/ctestprobe.h

single_include/ctestprobe.h: include/ctestprobe.h src/ctestprobe.c scripts/amalgamate.sh
	scripts/amalgamate.sh $@

clean:
	rm -rf $(BUILD) single_include ctestprobe.o libctestprobe.a

# Static library build. The c skill syntax-checks and runs cppcheck.
# It does not compile or link.

override CFLAGS += -std=c11 -Wall -Wextra -Wpedantic -Werror -Iinclude
override CPPCHECK_FLAGS += -Iinclude --std=c11

BITVECTOR_BUILD := build
BITVECTOR_LIB := $(BITVECTOR_BUILD)/libbitvector.a
BITVECTOR_OBJ := $(BITVECTOR_BUILD)/bitvector.o
BITVECTOR_TEST := $(BITVECTOR_BUILD)/test_bitvector

.PHONY: lib test check clean

lib: $(BITVECTOR_LIB)

$(BITVECTOR_OBJ): src/bitvector.c include/bitvector.h
	mkdir -p $(BITVECTOR_BUILD)
	$(CC) $(CFLAGS) -c -o $@ src/bitvector.c

$(BITVECTOR_LIB): $(BITVECTOR_OBJ)
	$(AR) rcs $@ $(BITVECTOR_OBJ)

$(BITVECTOR_TEST): tests/test_bitvector.c include/bitvector.h $(BITVECTOR_LIB)
	$(CC) $(CFLAGS) -o $@ tests/test_bitvector.c $(BITVECTOR_LIB) $(LDFLAGS)

test: $(BITVECTOR_TEST)
	./$(BITVECTOR_TEST)

check: $(if $(filter c,$(SKILLS)),c-check) test

clean:
	rm -rf $(BITVECTOR_BUILD)

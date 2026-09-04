CC = cc
CFLAGS = -O3 -Wall -Wextra -std=gnu11
LDFLAGS =

UNAME_S := $(shell uname -s)
ifeq ($(UNAME_S),Darwin)
LIBOMP_PREFIX := $(shell brew --prefix libomp 2>/dev/null)
OMPFLAGS = -Xpreprocessor -fopenmp -I$(LIBOMP_PREFIX)/include
OMPLIBS = -L$(LIBOMP_PREFIX)/lib -lomp
else
OMPFLAGS = -fopenmp
OMPLIBS = -fopenmp
endif

SRC_DIR = src
BIN_DIR = bin
COMMON_SRC = $(SRC_DIR)/toy_hash.c $(SRC_DIR)/pdf_io.c $(SRC_DIR)/hashtable.c $(SRC_DIR)/birthday_attack.c

.PHONY: all clean

all: $(BIN_DIR)/birthday_serial $(BIN_DIR)/birthday_parallel

$(BIN_DIR):
	mkdir -p $(BIN_DIR)

$(BIN_DIR)/birthday_serial: $(COMMON_SRC) $(SRC_DIR)/serial_main.c | $(BIN_DIR)
	$(CC) $(CFLAGS) $^ -o $@ $(LDFLAGS)

$(BIN_DIR)/birthday_parallel: $(COMMON_SRC) $(SRC_DIR)/parallel_main.c | $(BIN_DIR)
	$(CC) $(CFLAGS) $(OMPFLAGS) $^ -o $@ $(OMPLIBS)

clean:
	rm -rf $(BIN_DIR)

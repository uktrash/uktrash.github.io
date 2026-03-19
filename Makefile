CC ?= cc
SRC := src/main.c src/page.c
BENCH_SRC := src/bench.c
BIN := build/portfolio
BENCH_BIN := build/bench
EXPORT_DIR ?= docs

CPPFLAGS := -D_POSIX_C_SOURCE=200809L
CFLAGS ?= -std=c11 -O3 -pipe -flto -Wall -Wextra -Wpedantic -Wshadow -Wconversion -DNDEBUG
LDFLAGS ?= -flto

.PHONY: all clean run export pages smoke bench

all: $(BIN) $(BENCH_BIN)

build:
	mkdir -p build

$(BIN): $(SRC) | build
	$(CC) $(CPPFLAGS) $(CFLAGS) $(SRC) -o $(BIN) $(LDFLAGS)

$(BENCH_BIN): $(BENCH_SRC) | build
	$(CC) $(CPPFLAGS) $(CFLAGS) $(BENCH_SRC) -o $(BENCH_BIN) $(LDFLAGS)

run: $(BIN)
	./$(BIN)

export: $(BIN)
	mkdir -p $(EXPORT_DIR)
	./$(BIN) --export $(EXPORT_DIR)/index.html
	touch $(EXPORT_DIR)/.nojekyll

pages: export

smoke: $(BIN)
	./scripts/smoke.sh

bench: $(BIN) $(BENCH_BIN)
	./scripts/bench.sh

clean:
	rm -rf build dist docs

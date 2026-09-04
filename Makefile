CXX      = clang++
CXXFLAGS = -std=c++17 -Wall -Wextra -O2
BUILDDIR = build

SRCS = src/main.cpp src/error.cpp src/lexer.cpp src/parser.cpp src/sema.cpp src/codegen.cpp
OBJS = $(patsubst src/%.cpp,$(BUILDDIR)/%.o,$(SRCS))
DEPS = $(OBJS:.o=.d)

TARGET = $(BUILDDIR)/prashc

.PHONY: all clean test

all: $(TARGET)

$(BUILDDIR):
	mkdir -p $(BUILDDIR)

$(BUILDDIR)/%.o: src/%.cpp | $(BUILDDIR)
	$(CXX) $(CXXFLAGS) -MMD -MP -c $< -o $@

$(TARGET): $(OBJS)
	$(CXX) $(CXXFLAGS) $^ -o $@

-include $(DEPS)

clean:
	rm -rf $(BUILDDIR) examples/*.s hello fib fact fizzbuzz scope a.out

test: all
	@echo "=== hello.pd ==="
	@./$(TARGET) examples/hello.pd -o hello && ./hello
	@echo "=== fibonacci.pd ==="
	@./$(TARGET) examples/fibonacci.pd -o fib && ./fib
	@echo "=== factorial.pd ==="
	@./$(TARGET) examples/factorial.pd -o fact && ./fact
	@echo "=== fizzbuzz.pd ==="
	@./$(TARGET) examples/fizzbuzz.pd -o fizzbuzz && ./fizzbuzz
	@echo "=== scope.pd ==="
	@./$(TARGET) examples/scope.pd -o scope && ./scope
	@echo "=== ALL TESTS PASSED ==="

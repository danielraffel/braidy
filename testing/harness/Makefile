# Simple Makefile for the Braids Test Harness
# This builds the test harness components independently for testing

CXX = clang++
CXXFLAGS = -std=c++17 -O2 -Wall -Wextra

# Include paths
INCLUDES = -I../../Source -I../../Source/BraidyCore -I../../Source/BraidyVoice

# Source files
HARNESS_SOURCES = ModeRegistry.cpp BraidsReference.cpp AudioComparator.cpp TestRunner.cpp
BRAIDY_SOURCES = ../../Source/BraidyCore/MacroOscillator.cpp \
                 ../../Source/BraidyCore/AnalogOscillator.cpp \
                 ../../Source/BraidyCore/DigitalOscillator.cpp \
                 ../../Source/BraidyCore/BraidyMath.cpp \
                 ../../Source/BraidyVoice/BraidyVoice.cpp

EXAMPLE_SOURCE = example_usage.cpp

# Object files
HARNESS_OBJECTS = $(HARNESS_SOURCES:.cpp=.o)
BRAIDY_OBJECTS = $(notdir $(BRAIDY_SOURCES:.cpp=.o))
EXAMPLE_OBJECT = $(EXAMPLE_SOURCE:.cpp=.o)

# Output executable
TARGET = test_harness_example

.PHONY: all clean test

all: $(TARGET)

$(TARGET): $(HARNESS_OBJECTS) $(BRAIDY_OBJECTS) $(EXAMPLE_OBJECT)
	$(CXX) $(CXXFLAGS) $^ -o $@

# Harness object files
%.o: %.cpp
	$(CXX) $(CXXFLAGS) $(INCLUDES) -c $< -o $@

# Braidy source object files (with path handling)
MacroOscillator.o: ../../Source/BraidyCore/MacroOscillator.cpp
	$(CXX) $(CXXFLAGS) $(INCLUDES) -c $< -o $@

AnalogOscillator.o: ../../Source/BraidyCore/AnalogOscillator.cpp
	$(CXX) $(CXXFLAGS) $(INCLUDES) -c $< -o $@

DigitalOscillator.o: ../../Source/BraidyCore/DigitalOscillator.cpp
	$(CXX) $(CXXFLAGS) $(INCLUDES) -c $< -o $@

BraidyMath.o: ../../Source/BraidyCore/BraidyMath.cpp
	$(CXX) $(CXXFLAGS) $(INCLUDES) -c $< -o $@

BraidyVoice.o: ../../Source/BraidyVoice/BraidyVoice.cpp
	$(CXX) $(CXXFLAGS) $(INCLUDES) -c $< -o $@

# Test individual components
test-registry: ModeRegistry.o
	$(CXX) $(CXXFLAGS) $(INCLUDES) -DTEST_MAIN -c ModeRegistry.cpp -o test_registry.o
	$(CXX) test_registry.o -o test_registry
	./test_registry

test-comparator: AudioComparator.o
	$(CXX) $(CXXFLAGS) $(INCLUDES) -DTEST_MAIN -c AudioComparator.cpp -o test_comparator.o
	$(CXX) test_comparator.o -o test_comparator
	./test_comparator

clean:
	rm -f *.o $(TARGET) test_registry test_comparator

# Help target
help:
	@echo "Available targets:"
	@echo "  all          - Build the test harness example"
	@echo "  clean        - Remove all built files"
	@echo "  test-registry - Test the ModeRegistry component"
	@echo "  test-comparator - Test the AudioComparator component"
	@echo "  help         - Show this help message"

# Dependencies
ModeRegistry.o: ModeRegistry.h ModeRegistry.cpp
BraidsReference.o: BraidsReference.h BraidsReference.cpp ModeRegistry.h
AudioComparator.o: AudioComparator.h AudioComparator.cpp
TestRunner.o: TestRunner.h TestRunner.cpp BraidsReference.h AudioComparator.h ModeRegistry.h
example_usage.o: example_usage.cpp TestRunner.h
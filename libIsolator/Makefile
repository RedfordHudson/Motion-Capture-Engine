CXX = g++
CXXFLAGS = -std=c++11 -Wall -Wextra -O2

# Target binary
TARGETS = test_isolator

# Source files
SRCS = isolator.cpp test_isolator.cpp

# Object files
OBJS = $(SRCS:.cpp=.o)

# Default target
all: $(TARGETS)

# Linking the test executable
test_isolator: isolator.o test_isolator.o
	$(CXX) $(CXXFLAGS) -o $@ $^

# Compiling source files
%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

# Clean up
clean:
	rm -f $(OBJS) $(TARGETS)

# Run the test
run: test_isolator
	./test_isolator

# Dependencies
isolator.o: isolator.cpp isolator.h
test_isolator.o: test_isolator.cpp isolator.h

.PHONY: all clean run 
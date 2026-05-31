# Compiler and Flags
CXX      := g++
CXXFLAGS := -Wall -Wextra -std=c++17 -O2

# Directories
SRCDIR   := src
INCDIR   := include
BUILDDIR := build
TARGETDIR := bin

# Target Binary Name
TARGET   := $(TARGETDIR)/manager

# File tracking
SOURCES  := $(wildcard $(SRCDIR)/*.cpp)
OBJECTS  := $(patsubst $(SRCDIR)/%.cpp,$(BUILDDIR)/%.o,$(SOURCES))

# Include path flag
INC      := -I$(INCDIR)

# Default Rule
all: $(TARGET)

# Link the executable
$(TARGET): $(OBJECTS)
	@mkdir -p $(TARGETDIR)
	$(CXX) $(CXXFLAGS) $(OBJECTS) -o $(TARGET)

# Compile source files to object files
$(BUILDDIR)/%.o: $(SRCDIR)/%.cpp
	@mkdir -p $(BUILDDIR)
	$(CXX) $(CXXFLAGS) $(INC) -c -o $@ $<

# Clean up build artifacts
clean:
	rm -rf $(BUILDDIR) $(TARGETDIR)

.PHONY: all clean

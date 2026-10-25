CXX = g++
CXXFLAGS = -std=c++17 -Wall -Wextra -Wpedantic -O2 -MMD -MP
TARGET = loadbalancer
SOURCES = $(wildcard *.cpp)
OBJECTS = $(SOURCES:.cpp=.o)
DEPS = $(OBJECTS:.o=.d)

.PHONY: all docs clean

all: $(TARGET)

$(TARGET): $(OBJECTS)
	$(CXX) $(OBJECTS) -o $(TARGET)

%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

docs:
	doxygen Doxyfile

clean:
	$(RM) $(OBJECTS) $(DEPS) $(TARGET) $(TARGET).exe

-include $(DEPS)

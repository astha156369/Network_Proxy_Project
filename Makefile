
CXX = g++
CXXFLAGS = -std=c++17 -O2 -Wall -Iinclude
LIBS = -lws2_32


SRCS = $(wildcard src/*.cpp)
OBJS = $(SRCS:.cpp=.o)
TARGET = proxy.exe


.PHONY: all clean


all: $(TARGET)


$(TARGET): $(OBJS)
	$(CXX) $(OBJS) -o $(TARGET) $(LIBS)


%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@


clean:
	@if exist src\*.o del /q src\*.o
	@if exist $(TARGET) del /q $(TARGET)
	@echo Cleanup complete.
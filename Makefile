
ALL = passive

# Compiler and flags
CXX = g++
CXXFLAGS = -g -Wall
LFLAGS = 

LIBS =
OBJS = parser.o

all: $(ALL)

# wifipcap.a: $(OBJS)
# 	ar rc $@ $(OBJS)

passive: $(OBJS)  main.cpp parser.h
	$(CXX) $(CXXFLAGS)  -o passive $(OBJS) main.cpp 

parser.o: parser.cpp parser.h debug.h
	$(CXX) $(CXXFLAGS)  -c parser.cpp

clean:
	rm -f $(ALL) *.o

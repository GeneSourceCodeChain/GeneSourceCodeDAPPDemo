CXXFLAGS=-std=c++14 -O2 -msse2 -msse3
LIBS=-lboost_program_options -lboost_regex -lboost_system -lboost_filesystem
OBJS=$(patsubst %.cpp,%.o,$(wildcard *.cpp))

all: wrapper

wrapper: wrapper.o main.o
	$(CXX) $^ $(LIBS) -o ${@}

clean:
	$(RM) $(OBJS) wrapper


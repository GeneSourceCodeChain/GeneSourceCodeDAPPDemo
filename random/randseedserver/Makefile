BOOST_PREFIX=/home/ssgene/opt/boost
CXXFLAGS=-I${BOOST_PREFIX}/include `pkg-config --cflags libcrypto++` -O2
LIBS=-L${BOOST_PREFIX}/lib `pkg-config --libs libcrypto++` -lboost_system -lboost_filesystem -lboost_thread -lpthread
OBJS=$(patsubst %.cpp,%.o,$(wildcard *.cpp))

all: randseedserver

randseedserver: randseedserver.o
	$(CXX) $^ $(LIBS) -o ${@}

clean:
	$(RM) $(OBJS) randseedserver




INCLUDES=-I/opt/local/include
LIBS=
# LIBS+=-pthread 
# LIBS+=-L/opt/local/lib 
# LIBS+=-lmongoclient -lboost_filesystem -lboost_program_options -lboost_system

DEBUGFLAGS=
DEBUGFLAGS+=-g 
# DEBUGFLAGS=-g -Wa,-ahl=$(addsuffix .s, $(basename $<))

# -Wno-array-bounds: In one case gcc 4.8.1 (Ubuntu/Linaro 4.8.1-10ubuntu9) generates this warning
# after placing htons in completely another place. Probably a compiler bug.
CFLAGS=-Wall -O2 -std=c++0x $(DEBUGFLAGS) $(INCLUDES)
# -Wno-array-bounds

LDFLAGS=$(LIBS)
SOURCES=main.cpp bintxtSink.cpp sinkManager.cpp tcpSource.cpp observer.cpp asciitxtSink.cpp
OBJECTS=$(SOURCES:.cpp=.o)
DEPS=$(SOURCES:.cpp=.d)
EXECUTABLE=devlogd

all: $(SOURCES) $(EXECUTABLE)

$(EXECUTABLE): $(OBJECTS)
	g++ $(OBJECTS) $(LDFLAGS) -o $@

mongoSink.o: mongoSink.cpp
	g++ $(CFLAGS) -Wno-deprecated -Wno-strict-aliasing -Wno-unused-local-typedefs -c mongoSink.cpp

%.o: %.cpp
	g++ -MD -MP -std=c++0x $(CFLAGS) -c $<

clean:
	rm -f $(OBJECTS) $(EXECUTABLE) $(DEPS)

-include *.d

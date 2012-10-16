INCLUDES += `sdl-config --cflags`
LIBS += `sdl-config --libs` ../gb_core/*.o -lz

CFLAGS = -O2 -fPIC
CXXFLAGS = $(CFLAGS)

SUBDIRS =
SOURCES=$(wildcard *.cpp)
CSOURCES=$(wildcard *.c)
OBJS=$(SOURCES:.cpp=.o)
COBJS=$(CSOURCES:.c=.o)

MAKE_INCLUDES=`sdl-config --cflags` -I.. -I../util

MAKE_WITH="CXXFLAGS=$(CXXFLAGS)" "CFLAGS=$(CFLAGS)" "INCLUDES=$(MAKE_INCLUDES)"

all: $(OBJS)

subdir:

%.o:%.c
	$(CC) -c $(CXXFLAGS) $(INCLUDES) $<

%.o:%.cpp
	$(CXX) -c $(CXXFLAGS) $(INCLUDES) $<

clean:
	rm -f $(OBJS) tgbdual



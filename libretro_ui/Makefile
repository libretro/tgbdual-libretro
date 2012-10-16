
LIBS += ../gb_core/*.o -lz -lm
CFLAGS = -O3 -fPIC
CXXFLAGS = $(CFLAGS)
LDFLAGS = -shared -Wl,--version-script=link.T -Wl,--no-undefined

SOURCES=$(wildcard *.cpp)
CSOURCES=$(wildcard *.c)
OBJS=$(SOURCES:.cpp=.o)
COBJS=$(CSOURCES:.c=.o)

all: libretro.so

libretro.so: $(OBJS) $(COBJS)
	$(CXX) -o libretro.so $(OBJS) $(COBJS) $(LIBS) $(CXXFLAGS) $(LDFLAGS)

%.o:%.c
	$(CC) -c $(CFLAGS) $<

%.o:%.cpp
	$(CXX) -c $(CXXFLAGS) $<

clean:
	rm -f $(OBJS) $(COBJS) libretro.so


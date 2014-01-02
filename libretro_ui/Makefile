
LIBS += ../gb_core/*.o -lm
CFLAGS += -O3 -fPIC
CXXFLAGS = $(CFLAGS)
LDFLAGS = -shared -Wl,--version-script=link.T

SOURCES=$(wildcard *.cpp)
CSOURCES=$(wildcard *.c)
OBJS = $(SOURCES:.cpp=.o)
OBJS += $(CSOURCES:.c=.o)

all: libretro.so

libretro.so: $(OBJS)
	$(CXX) -o tgbdual_libretro.so $(OBJS) $(LIBS) $(CXXFLAGS) $(LDFLAGS)

%.o:%.c
	$(CC) -c $(CFLAGS) $<

%.o:%.cpp
	$(CXX) -c $(CXXFLAGS) $<

clean:
	rm -f $(OBJS) tgbdual_libretro.so


CFLAGS = -O3 -fPIC
CXXFLAGS = $(CFLAGS)

SOURCES = $(wildcard *.cpp)
CSOURCES = $(wildcard *.c)
OBJS = $(SOURCES:.cpp=.o)
OBJS += $(CSOURCES:.c=.o)

all: $(OBJS)

subdir:

%.o:%.c
	$(CC) -c $(CFLAGS) $(INCLUDES) $<

%.o:%.cpp
	$(CXX) -c $(CXXFLAGS) $(INCLUDES) $<

clean:
	rm -f $(OBJS) tgbdual



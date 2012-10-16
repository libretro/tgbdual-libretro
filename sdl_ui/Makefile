#TGB_INI_DIR = /opt/QtPalmtop/share/tgbdual

INCLUDES += `sdl-config --cflags` 
#INCLUDES += `sdl-config --cflags` -Dmain=SDL_main -DQWS -D_REENTRANT
LIBS += `sdl-config --libs` ../gb_core/*.o -lz
#CXXFLAGS = -g 
#CXXFLAGS = -pg -O -W -Wall
CXXFLAGS = -O3
#CXXFLAGS = -DNDEBUG -O2 -W -Wall 

ifdef TGB_INI_DIR
CXXFLAGS += -DTGB_INI_DIR=\"$(TGB_INI_DIR)\"
endif

CFLAGS = -O2
SUBDIRS =
SOURCES=$(wildcard *.cpp)
CSOURCES=$(wildcard *.c)
OBJS=$(SOURCES:.cpp=.o)
COBJS=$(CSOURCES:.c=.o)

MAKE_INCLUDES=`sdl-config --cflags` -I.. -I../util

MAKE_WITH="CXXFLAGS=$(CXXFLAGS)" "CFLAGS=$(CFLAGS)" "INCLUDES=$(MAKE_INCLUDES)"

all: tgbdual

tgbdual: subdir $(OBJS) $(COBJS)
	$(CXX) -o tgbdual $(OBJS) $(COBJS) $(LIBS) $(CXXFLAGS) $(INCLUDES)

sdlmain.o: dialogs.h

subdir:

%.o:%.c
	$(CC) -c $(CXXFLAGS) $(INCLUDES) $<

%.o:%.cpp
	$(CXX) -c $(CXXFLAGS) $(INCLUDES) $<

clean:
	rm -f $(OBJS) $(COBJS) tgbdual



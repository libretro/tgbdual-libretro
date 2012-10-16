#TGB_INI_DIR = /opt/QtPalmtop/share/tgbdual

INCLUDES += `sdl-config --cflags` 
#INCLUDES += `sdl-config --cflags` -Dmain=SDL_main -DQWS -D_REENTRANT
LIBS += `sdl-config --libs` ../gb_core/*.o -lz
CXXFLAGS = -O3 -fPIC

ifdef TGB_INI_DIR
CXXFLAGS += -DTGB_INI_DIR=\"$(TGB_INI_DIR)\"
endif

SOURCES=$(wildcard *.cpp)
CSOURCES=$(wildcard *.c)
OBJS = $(SOURCES:.cpp=.o)
OBJS += $(CSOURCES:.c=.o)

all: tgbdual

tgbdual: $(OBJS)
	$(CXX) -o tgbdual $(OBJS) $(LIBS) $(CXXFLAGS) $(INCLUDES)

sdlmain.o: dialogs.h

%.o:%.c
	$(CC) -c $(CXXFLAGS) $(INCLUDES) $<

%.o:%.cpp
	$(CXX) -c $(CXXFLAGS) $(INCLUDES) $<

clean:
	rm -f $(OBJS) tgbdual



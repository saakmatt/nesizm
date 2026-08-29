CXX      := g++
CC       := gcc
CXXFLAGS := -g -O0 -std=gnu++14 -fpermissive -w \
            -DTARGET_WINSIM=1 -DDEBUG=1 \
            -I. -Isrc -Iscope_timer \
            -I$(HOME)/Project/PrizmSDK/utils \
						-I$(HOME)/Project/PrizmSDK/utils/calctype \
						-I$(HOME)/Project/PrizmSDK/utils/calctype/fonts/arial_small
CFLAGS   := -g -O0 -w -DTARGET_WINSIM=1 -DDEBUG=1 \
            -I. \
            -I$(HOME)/Project/PrizmSDK/utils \
            -I$(HOME)/Project/PrizmSDK/utils/calctype \
            -I$(HOME)/Project/PrizmSDK/utils/zx7 \
						-I$(HOME)/Project/PrizmSDK/utils/calctype/fonts/arial_small
LDFLAGS  := -lglut -lGL

CXX_SRCS := $(wildcard src/*.cpp) $(wildcard src/mappers/*.cpp) $(wildcard src/gfx/*.cpp) \
            scope_timer/src/scope_timer.cpp sim_misc.cpp sim_file.cpp linux_main.cpp sim_stubs.cpp

CALCTYPE := $(HOME)/Project/PrizmSDK/utils/calctype
ZX7 := $(HOME)/Project/PrizmSDK/utils/zx7
C_SRCS   := $(ZX7)/src/dzx7.c $(ZX7)/src/compress.c $(ZX7)/src/optimize.c $(ZX7)/src/zx7.c \
            calctype/calctype.c calctype/calctype_prizm.c 

# Combine and correctly translate both extensions into .o files
OBJS := $(CXX_SRCS:.cpp=.o) $(C_SRCS:.c=.o) 

nesizm-sim: $(OBJS)
	$(CXX) $(OBJS) -o $@ $(LDFLAGS)

%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

# It is safer to compile .c files with the C compiler (gcc) rather than g++
%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(OBJS) nesizm-sim

.PHONY: clean

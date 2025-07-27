# Detect platform
UNAME_S := $(shell uname -s)
UNAME_M := $(shell uname -m)

# Compiler settings
CC = gcc
CXX = g++
CFLAGS = -Wall -g -O2
CXXFLAGS = -Wall -g -O2

# Platform-specific settings
ifeq ($(UNAME_S),Darwin)
    ifeq ($(UNAME_M),arm64)
        # M1 Mac
        IDIR = /opt/homebrew/include
        LDIR = /opt/homebrew/lib
    else
        # Intel Mac
        IDIR = /usr/local/include
        LDIR = /usr/local/lib
    endif
else
    # Linux/Windows
    IDIR = /usr/include
    LDIR = /usr/lib
endif

# Include paths
INCLUDES = -I$(IDIR) -I$(IDIR)/opus
LDFLAGS = -L$(LDIR)
LIBS = -lportaudio -lopus -lsndfile -lpthread

# Source files
COMMON_SRCS = tcpUtils.c paUtils.c circular_buf.cpp
COMMON_OBJS = $(COMMON_SRCS:.c=.o)
COMMON_OBJS := $(COMMON_OBJS:.cpp=.o)

# Targets
all: tcp_source_client tcp_client

tcp_source_client: tcp_source_client.o sfUtils.o $(COMMON_OBJS)
	$(CXX) -o $@ $^ $(LDFLAGS) $(LIBS)

tcp_client: tcp_client.o $(COMMON_OBJS)
	$(CXX) -o $@ $^ $(LDFLAGS) $(LIBS)

%.o: %.c
	$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@

%.o: %.cpp
	$(CXX) $(CXXFLAGS) $(INCLUDES) -c $< -o $@

clean:
	rm -f *.o tcp_source_client tcp_client

.PHONY: all clean
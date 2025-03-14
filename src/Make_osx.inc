# Compilateur
CC = g++-14
CXX = g++-14

# Flags de compilation et de linkage pour SDL2
CXXFLAGS += -I/opt/homebrew/Cellar/sdl2/2.32.2/include $(shell sdl2-config --cflags)
LIB = $(shell sdl2-config --libs) -lpthread -framework Cocoa -framework QuartzCore -framework IOKit

CXX = mpic++
CXXFLAGS = -std=c++17 -Wall -O3 -march=native
INCLUDES = -I/opt/homebrew/Cellar/sdl2/2.32.2/include -I/opt/homebrew/include/SDL2 -I/opt/homebrew/include -D_THREAD_SAFE
LIBS = -L/opt/homebrew/lib -lSDL2

SRCS = display.cpp model.cpp simulation.cpp
OBJS = $(SRCS:.cpp=.o)
DEPS = $(SRCS:.cpp=.d)

TARGET = simulation.exe

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CXX) $(CXXFLAGS) -o $@ $^ $(LIBS)

%.o: %.cpp
	$(CXX) $(CXXFLAGS) $(INCLUDES) -MMD -c $< -o $@

clean:
	rm -f $(OBJS) $(DEPS) $(TARGET)

-include $(DEPS)

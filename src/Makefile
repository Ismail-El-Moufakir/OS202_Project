CXX = mpic++
CXXFLAGS = -std=c++17 -Wall -O3 -march=native
INCLUDES = -I/opt/homebrew/Cellar/sdl2/2.32.2/include -I/opt/homebrew/include/SDL2 -I/opt/homebrew/include
LIBS = -L/opt/homebrew/lib -lSDL2

all: simulation.exe step_4.exe

%.o: %.cpp
	$(CXX) $(CXXFLAGS) $(INCLUDES) -c $< -o $@

simulation.exe: simulation.o model.o display.o
	$(CXX) $(CXXFLAGS) $^ -o $@ $(LIBS)

step_4.exe: step_4.o model.o display.o
	$(CXX) $(CXXFLAGS) $^ -o $@ $(LIBS)

clean:
	@rm -f *.o *.exe *~ *.d

.PHONY: clean all

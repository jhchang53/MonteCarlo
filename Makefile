CXXFLAGS = -g
# CXXFLAGS = -pg

mc: mainmc.o MC.o Geom.o Part.o IniParser.o
	g++ -o mc $(CXXFLAGS) $^ -lm -lcairo
clean:
	rm -f *.o

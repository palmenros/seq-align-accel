PROJECT_NAME = seqMatcher
MAKEFLAGS += " -j2 "
CFLAGS=-O3 -Wall

all: obj $(PROJECT_NAME)

$(PROJECT_NAME): obj/$(PROJECT_NAME).o obj/util.o obj/CAccelDriver.o obj/CSeqMatcherDriver.o
	g++ $(CFLAGS) obj/$(PROJECT_NAME).o obj/util.o obj/CAccelDriver.o obj/CSeqMatcherDriver.o -o $(PROJECT_NAME) -lm -lcma -lpthread

obj/$(PROJECT_NAME).o: src/$(PROJECT_NAME).cpp src/util.hpp src/CAccelDriver.hpp src/CSeqMatcherDriver.hpp
	g++ -c $(CFLAGS) src/$(PROJECT_NAME).cpp -o obj/$(PROJECT_NAME).o
obj/util.o: src/util.cpp src/util.hpp
	g++ -c $(CFLAGS) src/util.cpp -o obj/util.o
obj/CAccelDriver.o: src/CAccelDriver.cpp src/CAccelDriver.hpp src/util.hpp
	g++ -c $(CFLAGS) src/CAccelDriver.cpp -o obj/CAccelDriver.o
obj/CSeqMatcherDriver.o: src/CSeqMatcherDriver.cpp src/CSeqMatcherDriver.hpp src/CAccelDriver.hpp src/util.hpp
	g++ -c $(CFLAGS) src/CSeqMatcherDriver.cpp -o obj/CSeqMatcherDriver.o

obj:
	mkdir obj/

clean:
	rm -f $(PROJECT_NAME)
	rm -rf obj/
	rm -f sds_trace_data.dat
	rm -f scores.bit

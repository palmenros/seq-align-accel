PROJECT_NAME = genSequenceFiles
MAKEFLAGS += " -j2 "
CFLAGS = -O3 -Wall
.PHONY: all

all: $(PROJECT_NAME)

$(PROJECT_NAME): $(PROJECT_NAME).cpp
	g++ $(CFLAGS) -o $(PROJECT_NAME) $(PROJECT_NAME).cpp


clean:
	rm -f $(PROJECT_NAME)
	#rm -f $(PROJECT_NAME).o

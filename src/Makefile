PROJECT=tema2
SOURCES=client.cpp file_hash.cpp peer_threads.cpp tracker.cpp \
 busyness_score.cpp
LIBFLAGS=
INCPATHS=
LINKFLAGS=ROUND_ROBIN DEBUG
CFLAGS=-Wall -Werror -Wno-error=unused-variable
CC=mpic++

# Automatic generation of some important lists
OBJECTS=$(SOURCES:.cpp=.o)
INCFLAGS=$(foreach TMP,$(INCPATHS),-I$(TMP))
LIBFLAGS=$(foreach TMP,$(LIBPATHS),-l$(TMP))
LINKFLAGSS=$(foreach TMP,$(LINKFLAGS),-D$(TMP))


build: all
all : $(SOURCES) tema2

tema2: $(OBJECTS)
	$(CC) $(CFLAGS) $(INCFLAGS) $(LIBFLAGS) $(LINKFLAGSS) -pthread tema2.cpp $(OBJECTS) -o tema2

.cpp.o:
	$(CC) $(CFLAGS) $(INCFLAGS) $(LIBFLAGS) $(LINKFLAGSS) -pthread -c $< -o $@

clean:
	rm -rf tema2 $(OBJECTS)

OBJECT_FILES = db.o util.o main.o
CC = g++-5
CFLAGS = -Wall -g

%.o: %.cpp %.hpp
	$(CC) $(CFLAGS) -c $< -o $@

neotio: $(OBJECT_FILES)
	$(CC) $(CFLAGS) $(OBJECT_FILES) -lsqlite3 -ltag -o neotio


# future: -lportaudio for portaudio
# 	  -lavformat -lavcodec for ffmpeg


.phony: clean

clean:
	rm -f *.o
	rm -f *~
	rm -f test.db
	rm -f neotio

// Copyright 2015 Ryan Marcus
// This file is part of neotio

#include <boost/lockfree/queue.hpp>
#include <taglib/tag.h>
#include <taglib/fileref.h>

#include <pthread.h>
#include <dirent.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <regex.h>


#include "db.hpp"
#include "util.hpp"

typedef struct {
	const char* title;
	const char* artist;
	const char* album;
	const char* path;
} DBQueueItem;

typedef struct {
	boost::lockfree::queue<char*>* q;
	boost::lockfree::queue<DBQueueItem*>* dbQ;
	bool* finished;
} ConsumerArgs;

typedef struct {
	boost::lockfree::queue<char*>* q;
	const char* dir;
} ProducerArgs;



typedef struct {
	boost::lockfree::queue<DBQueueItem*>*q;
	const char* dbPath;
	bool* finished;
	int libraryID;
} DBThreadArgs;




void* readFromDBQueue(void* vargs) {
	DBThreadArgs* args = (DBThreadArgs*) vargs;
	DBConf db;

	db.path = args->dbPath;
	connectDB(&db);

	startTx(&db);
	while (1) {
		DBQueueItem* dbqi;
		bool res = args->q->pop(dbqi);
		if (res) {
			Song song;
			song.title = dbqi->title;
			song.artist = dbqi->artist;
			song.album = dbqi->album;
			
			if (insertSong(&db, &song)) {
				// we inserted the song,
				// time to add the source...
				Source src;
				src.libraryID = args->libraryID;
				src.songID = song.id;
				src.path = dbqi->path;
				insertSource(&db, &src);
			}
			//free(dbqi->title);
			//free(dbqi->artist);
			//free(dbqi->album);
			//free(dbqi->path);
			free(dbqi);
		} else {
			if (*(args->finished)) {
				break;
			} else {
				// let the producer catch up...
				msleep(250);
			}
		}
	}

	commitTx(&db);
	freeDB(&db);
	return NULL;
	
}

void processFile(char* path, Song* s) {
	TagLib::FileRef f(path);

	size_t len;
	len = f.tag()->title().length();
	s->title = strndup(f.tag()->title().toCString(), len);

	len = f.tag()->album().length();
	s->album = strndup(f.tag()->album().toCString(), len);

	
	len = f.tag()->artist().length();
	s->artist = strndup(f.tag()->artist().toCString(), len);

	//printf("[%s] by (%s) on {%s} from %s\n", s->title, s->artist, s->album, path);
	
}



void* readFromQueue(void* vargs) {
	ConsumerArgs* args = (ConsumerArgs*) vargs;

	while (1) {
		char* nxt;
		bool res = args->q->pop(nxt);
		if (res) {
			Song song;
			processFile(nxt, &song);
			DBQueueItem* dbqi = (DBQueueItem*) malloc(sizeof(DBQueueItem));
			dbqi->title = song.title;
			dbqi->artist = song.artist;
			dbqi->album = song.album;
			dbqi->path = nxt;
			args->dbQ->push(dbqi);

		} else {
			if (*(args->finished)) {
				break;
			} else {
				// let the producer catch up...
				msleep(250);
			}
		}
	}


	return NULL;
}

bool isValidExtension(char* filename) {
	// use a regular expression to find filenames ending in...
	// flac,mp4

	// TODO don't compile the regex every time...
	regex_t regex;
	regcomp(&regex, "^.*(flac|m4a)$", REG_EXTENDED | REG_NOSUB);
	int res = regexec(&regex, filename, 0, NULL, 0);
	regfree(&regex);

	return res == 0;
	
}

void* readDirectory(void* vargs) {

	ProducerArgs* args = (ProducerArgs*) vargs;
	int dir_len = strlen(args->dir);
	//printf("Going to read from: %s (%d)\n", dir, dir_len);
	

	DIR* d = opendir(args->dir);
	
	if (d == NULL) {
		printf("Error opening directory\n");
		return NULL;
	}
	
	// get the FD of the directory so that
	// we can yield when we would otherwise block
	// int fd = dirfd(d);

	// readdir potentially uses global state!
	// however, readdir_r has issues with buffer size
	// allocation...
	struct dirent* dp;
	while ((dp = readdir(d)) != NULL) {

		if (dp->d_namlen <= 2)
			continue;

		if (dp->d_name[0] == '.' 
		    && ( dp->d_name[1] == '.' 
			 || dp->d_name[1] == 0))
			continue;

		if (dp->d_type == DT_REG &&
		    !isValidExtension(dp->d_name))
			continue;
		
		// construct the path for this entry
		char* path = (char*) calloc(dir_len + dp->d_namlen + 2,
					    sizeof(char));

		strncat(path, args->dir, dir_len);
		strncat(path, dp->d_name, dp->d_namlen);
		if (dp->d_type == DT_DIR) {
			strncat(path, "/", 1);

			ProducerArgs toPass;
			toPass.dir = path;
			toPass.q = args->q;
			
			readDirectory(&toPass);
			free(path);
		} else if (dp->d_type == DT_REG) {
			//printf("Pushing value...\n");
			args->q->push(path);
		}


	}


	closedir(d);
	return NULL;
}


void scanLibrary(char* path, const char* dbPath, int numConsumers) {
	boost::lockfree::queue<char*>* q = new boost::lockfree::queue<char*>(100);
	boost::lockfree::queue<DBQueueItem*>* dbQ = new boost::lockfree::queue<DBQueueItem*>(100);
	
	bool finished = false;
	bool finished2 = false;

	DBConf db;
	initDB(dbPath, &db);
	connectDB(&db);

	Library l;
	l.path = path;

	if (!insertLibrary(&db, &l)) {
		printf("Could not insert library\n");
		return;
	}

	

	ProducerArgs pargs;
	ConsumerArgs cargs;
	DBThreadArgs dbargs;

	pargs.q = q;
	pargs.dir = path;

	cargs.q = q;
	cargs.finished = &finished;
	cargs.dbQ = dbQ;

	dbargs.q = dbQ;
	dbargs.dbPath = dbPath;
	dbargs.finished = &finished2;
	dbargs.libraryID = l.id;
	
       
	
	pthread_t producer;
	pthread_t consumers[numConsumers];
	pthread_t dbThread;

	pthread_create(&producer, NULL, readDirectory, &pargs);

	for (int i = 0; i < numConsumers; i++) {
		pthread_create(&consumers[i], NULL, readFromQueue, &cargs);
	}

	pthread_create(&dbThread, NULL, readFromDBQueue, &dbargs);


	pthread_join(producer, NULL);
	printf("Producer thread joined!\n");
	finished = true;

	for (int i = 0; i < numConsumers; i++) {
		pthread_join(consumers[i], NULL);
	}
	printf("Consumer threads joined!\n");
	finished2 = true;


	pthread_join(dbThread, NULL);
	printf("DB thread joined!\n");
}



int main(int argc, char** argv) {
	//gotrace(1);

	if (argc != 2) {
		printf("Pleae enter a path\n");
		return -1;
	}
	
	
	scanLibrary(argv[1], "test.db", 4);


	return 0;
}



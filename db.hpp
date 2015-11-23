// Copyright 2015 Ryan Marcus
// This file is part of neotio


#ifndef NEOTIO_DB
#define NEOTIO_DB

#include <sqlite3.h>

typedef struct {
	const char* path;
	sqlite3* db;
} DBConf;

typedef struct {
	int id;
	const char* path;
} Library;

typedef struct {
	int id;
	const char* title;
	const char* artist;
	const char* album;
} Song;

typedef struct {
	int id;
	int songID;
	int libraryID;
	const char* path;
} Source;

void initDB(const char* dbPath, DBConf* conf);
void connectDB(DBConf* conf);
void freeDB(DBConf* conf);
bool insertLibrary(DBConf* db, Library* library);
bool insertSong(DBConf* db, Song* song);
bool insertSource(DBConf* db, Source* source);

void startTx(DBConf* db);
void commitTx(DBConf* db);


#endif

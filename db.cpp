// Copyright 2015 Ryan Marcus
// This file is part of neotio


#include <stdio.h>
#include "db.hpp"
#include "util.hpp"


	

    

void connectDB(DBConf* conf) {
	sqlite3_open(conf->path, &(conf->db));
}

void freeDB(DBConf* conf) {
	sqlite3_close_v2(conf->db);

}

void startTx(DBConf* conf) {
	sqlite3_exec(conf->db, "BEGIN TRANSACTION", NULL, NULL, NULL);
}

void commitTx(DBConf* conf) {
	sqlite3_exec(conf->db, "COMMIT TRANSACTION", NULL, NULL, NULL);
}


bool executeBusyLoop(sqlite3_stmt* stmt) {
	int res;
	while (true) {
		res = sqlite3_step(stmt);

		if (res == SQLITE_CONSTRAINT)
			return false;

		if (res == SQLITE_BUSY) {
			msleep(250);
			continue;
		}

		return true;
	}
}

bool insertLibrary(DBConf* db, Library* library) {
	sqlite3_stmt* stmt;
	sqlite3_prepare_v2(db->db,
			   "INSERT INTO library (uri) VALUES(?)",
			   500, &stmt, NULL);

	sqlite3_bind_text(stmt, 1, library->path, -1, SQLITE_STATIC);

	if (!executeBusyLoop(stmt)) {
		library->id = -1;
		return false;
	}
	library->id = sqlite3_last_insert_rowid(db->db);
	return true;
}

bool insertSong(DBConf* db, Song* song) {
	sqlite3_stmt* stmt;
	sqlite3_prepare_v2(db->db,
			   "INSERT INTO song (title, artist, album) VALUES(?,?,?)",
			   500, &stmt, NULL);

	sqlite3_bind_text(stmt, 1, song->title, -1, SQLITE_STATIC);
	sqlite3_bind_text(stmt, 2, song->artist, -1, SQLITE_STATIC);
	sqlite3_bind_text(stmt, 3, song->album, -1, SQLITE_STATIC);


	if (!executeBusyLoop(stmt)) {
		song->id = -1;
		return false;
	}
	
	song->id = sqlite3_last_insert_rowid(db->db);

	return true;
}

bool insertSource(DBConf* db, Source* source) {
	sqlite3_stmt* stmt;

	if (source->libraryID == -1 || source->songID == -1)
		return false;
	
	sqlite3_prepare_v2(db->db,
			   "INSERT INTO source (songID, libraryID, path) VALUES(?,?,?)",
			   500, &stmt, NULL);

	sqlite3_bind_int(stmt, 1, source->songID);
	sqlite3_bind_int(stmt, 2, source->libraryID);
	sqlite3_bind_text(stmt, 3, source->path, -1, SQLITE_STATIC);

	if (!executeBusyLoop(stmt)) {
		source->id = -1;
		return false;
	}
	source->id = sqlite3_last_insert_rowid(db->db);
	return true;
}


void initDB(const char* dbPath, DBConf* conf) {
	sqlite3_config(SQLITE_CONFIG_MULTITHREAD);
	conf->path = dbPath;


	// try to create the schema
	sqlite3* db;
	sqlite3_open(dbPath, &db);

	sqlite3_stmt* stmt;

	sqlite3_prepare_v2(db,
			"CREATE TABLE IF NOT EXISTS song (id INTEGER PRIMARY KEY, title TEXT, artist TEXT, album TEXT, UNIQUE(title, artist, album))",
			500, &stmt, NULL);

/*	if (res != SQLITE_OK) {
		printf("Error in preparing statement: %d\n", res);
		printf("%s\n", sqlite3_errmsg(db));
		}*/
	

	sqlite3_step(stmt);
	sqlite3_finalize(stmt);

	sqlite3_prepare_v2(db,
			   "CREATE TABLE IF NOT EXISTS library (id INTEGER PRIMARY KEY, uri TEXT, UNIQUE(uri))",
			500, &stmt, NULL);
	

	sqlite3_step(stmt);
	sqlite3_finalize(stmt);

		sqlite3_prepare_v2(db,
			   "CREATE TABLE IF NOT EXISTS source (id INTEGER PRIMARY KEY, songID REFERENCES song(id), libraryID REFERENCES library(id), path TEXT, UNIQUE(libraryID, path))",
			500, &stmt, NULL);
	

	sqlite3_step(stmt);
	sqlite3_finalize(stmt);
	
	

	sqlite3_close_v2(db);
	
}


/*int main(int argc, char** argv) {
	DBConf db;
	initDB("test.db", &db);
	connectDB(&db);
	
	Library l;
	l.path = "/Users/ryan/dev/Copy/Local FLACs/";

	insertLibrary(&db, &l);

	Song s;
	s.title = "Obedear";
	s.artist = "Purity Ring";
	s.album = "Purity Ring (album)";

	insertSong(&db, &s);

	Source src;
	src.libraryID = l.id;
	src.songID = s.id;
	src.path = "/Users/ryan/dev/Copy/Local FLACs/Purity Ring/Purity Ring/Obedear.flac";

	insertSource(&db, &src);
	     

	freeDB(&db);
	
}


*/

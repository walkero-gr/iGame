

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <proto/dos.h>

#include "sqldb.h"

// External variable from iGameMain.c
extern char *executable_name;


static char absDbFilePath[512];
static BOOL dbPathInitialized = FALSE;

/**
 * Get the path of the parent folder (from AmigaOS API)
 * Implements the same logic as fsfuncs.c:getParentPath()
 */
static void getParentPathLock(char *filename, char *result, int resultSize)
{
	BPTR fileLock = Lock(filename, SHARED_LOCK);
	if (fileLock)
	{
		BPTR folderLock = ParentDir(fileLock);
		if (folderLock) {
			NameFromLock(folderLock, result, resultSize);
			UnLock(folderLock);
		}
		UnLock(fileLock);
	}
}

/**
 * Safe string concatenation (like strlcat but portable)
 */
static size_t safe_strlcat(char *dst, const char *src, size_t dstSize)
{
	size_t dstLen = strlen(dst);
	size_t srcLen = strlen(src);
	
	if (dstLen + srcLen >= dstSize) {
		return dstLen + srcLen;  // Would overflow
	}
	
	strcat(dst, src);
	return dstLen + srcLen;
}

/**
 * Resolve database path using executable location via AmigaOS APIs
 * This uses getParentPath() which properly locks and unlocks directories
 */
BOOL initializeDBPath(void)
{
	if (dbPathInitialized) {
		return TRUE;  // Already initialized
	}
	
	printf("[initializeDBPath] Starting path initialization...\n");
	
	if (!executable_name || executable_name[0] == '\0') {
		printf("[WARNING] executable_name not set, using fallback\n");
		strcpy(absDbFilePath, "data.db");
		dbPathInitialized = TRUE;
		return TRUE;
	}
	
	printf("[initializeDBPath] Executable name: %s\n", executable_name);
	
	// Get the parent directory of the executable using AmigaOS API
	printf("[initializeDBPath] Getting parent directory...\n");
	char parentPath[512];
	parentPath[0] = '\0';
	getParentPathLock(executable_name, parentPath, sizeof(parentPath));
	
	if (parentPath[0] == '\0') {
		printf("[WARNING] Failed to get parent directory, using fallback\n");
		strcpy(absDbFilePath, "data.db");
		dbPathInitialized = TRUE;
		return TRUE;
	}
	
	printf("[initializeDBPath] Parent directory: %s\n", parentPath);
	
	// Initialize the path
	strcpy(absDbFilePath, parentPath);
	
	// Append path separator if needed
	int pathLen = strlen(absDbFilePath);
	if (pathLen > 0 && absDbFilePath[pathLen - 1] != '/' && absDbFilePath[pathLen - 1] != ':') {
		if (safe_strlcat(absDbFilePath, "/", sizeof(absDbFilePath)) >= sizeof(absDbFilePath)) {
			printf("[ERROR] Database path too long (after separator)\n");
			strcpy(absDbFilePath, "data.db");  // Fallback
			dbPathInitialized = TRUE;
			return TRUE;
		}
	}
	
	// Append database filename
	if (safe_strlcat(absDbFilePath, "data.db", sizeof(absDbFilePath)) >= sizeof(absDbFilePath)) {
		printf("[ERROR] Database path too long (after filename)\n");
		strcpy(absDbFilePath, "data.db");  // Fallback
		dbPathInitialized = TRUE;
		return TRUE;
	}
	
	printf("[initializeDBPath] Full database path resolved to: %s\n", absDbFilePath);
	dbPathInitialized = TRUE;
	return TRUE;
}

/**
 * Get the absolute database path
 */
const char* getDBPath(void)
{
	if (!dbPathInitialized) {
		initializeDBPath();
	}
	return absDbFilePath;
}

void printSqliteVer(void)
{
  printf("%s\n", sqlite3_libversion());
}
/*
static int callback(void *NotUsed, int argc, char **argv, char **azColName) {
  if (NotUsed)
    NotUsed = 0;

  for (int i = 0; i < argc; i++) {
    printf("%s = %s\n", azColName[i], argv[i] ? argv[i] : "NULL");
  }
  printf("\n");
  return 0;
}
*/
BOOL createDB(void)
{
	sqlite3 *db;
	char *err_msg = 0;
	
	printf("[createDB] Starting database creation...\n");
	
	// Initialize and get database path
	if (!initializeDBPath()) {
		printf("[createDB] ERROR: Failed to initialize database path\n");
		return FALSE;
	}
	
	const char *dbPath = getDBPath();
	if (!dbPath || dbPath[0] == '\0') {
		printf("[createDB] ERROR: Database path not initialized\n");
		return FALSE;
	}
	printf("[createDB] Creating database: %s\n", dbPath);
	
	// Use resolved absolute path
	printf("[createDB] Calling sqlite3_open_v2 with flags: SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE\n");
	int rc = sqlite3_open_v2(dbPath, &db, SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE, NULL);
	if (rc != SQLITE_OK) {
		printf("[createDB] ERROR: Cannot open database: %s (error code: %d)\n", sqlite3_errmsg(db), rc);
		if (db) sqlite3_close(db);
		return FALSE;
	}
	printf("[createDB] Database opened successfully\n");
	// Set busy timeout to 5 seconds to allow for lock retries
	sqlite3_busy_timeout(db, 5000);
	
	// Set pragmas for better Amiga filesystem compatibility
	printf("[createDB] Setting database pragmas...\n");
	int pragma_rc = sqlite3_exec(db, "PRAGMA journal_mode = TRUNCATE;", 0, 0, 0);
	if (pragma_rc != SQLITE_OK) {
		printf("[createDB] WARNING: Failed to set journal_mode pragma (error code: %d)\n", pragma_rc);
	}
	sqlite3_exec(db, "PRAGMA synchronous = NORMAL;", 0, 0, 0);
	sqlite3_exec(db, "PRAGMA cache_size = 2000;", 0, 0, 0);
	printf("[createDB] Pragmas set\n");
	
	printf("[createDB] Database opened, creating schema...\n");
	
	// Create version table
	printf("Creating version table...\n");
	CONST_STRPTR sql = "CREATE TABLE IF NOT EXISTS version (ver INTEGER PRIMARY KEY);";
	rc = sqlite3_exec(db, sql, 0, 0, &err_msg);
	if (rc != SQLITE_OK) {
		printf("SQL error creating version table: %s (error code: %d)\n", err_msg, rc);
		if (err_msg) sqlite3_free(err_msg);
		sqlite3_close(db);
		return FALSE;
	}
	printf("[createDB] Version table created successfully\n");
	
	// Insert version
	printf("Inserting version data...\n");
	sql = "INSERT OR IGNORE INTO version VALUES (1);";
	rc = sqlite3_exec(db, sql, 0, 0, &err_msg);
	if (rc != SQLITE_OK) {
		printf("SQL error inserting version: %s (error code: %d)\n", err_msg, rc);
		if (err_msg) sqlite3_free(err_msg);
		sqlite3_close(db);
		return FALSE;
	}
	printf("[createDB] Version data inserted successfully\n");
	
	// Create items table
	printf("Creating items table...\n");
	sql = "CREATE TABLE IF NOT EXISTS items (" 
		"id INTEGER PRIMARY KEY, "
		"title TEXT, "
		"genre TEXT, "
		"path TEXT, "
		"favourite INTEGER, "
		"times_played INTEGER, "
		"last_played INTEGER, "
		"hidden INTEGER, "
		"deleted INTEGER, "
		"user_title TEXT, "
		"year INTEGER, "
		"players INTEGER, "
		"chipset TEXT);";
	
	rc = sqlite3_exec(db, sql, 0, 0, &err_msg);
	if (rc != SQLITE_OK) {
		printf("SQL error creating items table: %s (error code: %d)\n", err_msg, rc);
		if (err_msg) sqlite3_free(err_msg);
		sqlite3_close(db);
		return FALSE;
	}
	printf("[createDB] Items table created successfully\n");
	
	// Commit and close
	printf("[createDB] Closing database...\n");
	int close_rc = sqlite3_close(db);
	if (close_rc != SQLITE_OK) {
		printf("[createDB] WARNING: sqlite3_close returned error code: %d\n", close_rc);
	}
	printf("[createDB] Database and schema created successfully at: %s\n", getDBPath());
	return TRUE;
}

sqlite3 *sqlDBOpen(void)
{
  sqlite3 *db;
  // sqlite3_stmt *res;

  // Initialize path if not already done
  const char *dbPath = getDBPath();
  if (!dbPath || dbPath[0] == '\0') {
    printf("ERROR: Database path not initialized\n");
    return 0;
  }
  printf("Opening database for read/write: %s\n", dbPath);
  int rc = sqlite3_open_v2(dbPath, &db, SQLITE_OPEN_READWRITE, NULL);
  if (rc != SQLITE_OK) {
    printf("Cannot open database: %s (error code: %d)\n", sqlite3_errmsg(db), rc);
    if (db) sqlite3_close(db);
    return 0;
  }
  // Set busy timeout to 5 seconds to allow for lock retries
  sqlite3_busy_timeout(db, 5000);
  
  // Set pragmas for better Amiga filesystem compatibility
  sqlite3_exec(db, "PRAGMA journal_mode = TRUNCATE;", 0, 0, 0);
  sqlite3_exec(db, "PRAGMA synchronous = NORMAL;", 0, 0, 0);

  return db;
}

void sqlDBClose(sqlite3 *db)
{
  sqlite3_close(db);
}

BOOL sqlBeginTransaction(sqlite3 *db)
{
	char *err_msg = 0;
	int rc = sqlite3_exec(db, "BEGIN TRANSACTION;", 0, 0, &err_msg);
	if (rc != SQLITE_OK) {
		printf("Failed to begin transaction: %s (error code: %d)\n", err_msg, rc);
		if (err_msg) sqlite3_free(err_msg);
		return FALSE;
	}
	return TRUE;
}

BOOL sqlCommitTransaction(sqlite3 *db)
{
	char *err_msg = 0;
	int rc = sqlite3_exec(db, "COMMIT;", 0, 0, &err_msg);
	if (rc != SQLITE_OK) {
		printf("Failed to commit transaction: %s (error code: %d)\n", err_msg, rc);
		if (err_msg) sqlite3_free(err_msg);
		return FALSE;
	}
	return TRUE;
}

BOOL sqlRollbackTransaction(sqlite3 *db)
{
	char *err_msg = 0;
	int rc = sqlite3_exec(db, "ROLLBACK;", 0, 0, &err_msg);
	if (rc != SQLITE_OK) {
		printf("Failed to rollback transaction: %s (error code: %d)\n", err_msg, rc);
		if (err_msg) sqlite3_free(err_msg);
		return FALSE;
	}
	return TRUE;
}

BOOL sqlAddItem(
	STRPTR title,
	STRPTR genre,
	STRPTR path,
	int favourite,
	int times_played,
	int last_played,
	int hidden,
	int deleted,
	STRPTR user_title,
	int year,
	int players,
	STRPTR chipset,
	sqlite3 *db
) {
	// sqlite3 *db;
	sqlite3_stmt *res;

	// int rc = sqlite3_open(dbPath, &db);
	// if (rc != SQLITE_OK) {
	//   fprintf(stderr, "Cannot open database: %s\n", sqlite3_errmsg(db));
	//   sqlite3_close(db);
	//   return FALSE;
	// }

	CONST_STRPTR sql = "INSERT INTO items " \
		"(" \
		"title, genre, path, favourite, " \
		"times_played, last_played, hidden, " \
		"deleted, user_title, year, players, chipset" \
		") " \
		"VALUES (" \
		":title, :genre, :path, :favourite, " \
		":times_played, :last_played, :hidden, " \
		":deleted, :user_title, :year, :players, :chipset" \
		")";

	int rc = sqlite3_prepare_v2(db, sql, -1, &res, 0);
	if (rc != SQLITE_OK) {
		printf("Failed to execute statement: %s\n", sqlite3_errmsg(db));
		return FALSE;
	}

	int titleIdx        = sqlite3_bind_parameter_index(res, ":title");
	int genreIdx        = sqlite3_bind_parameter_index(res, ":genre");
	int pathIdx         = sqlite3_bind_parameter_index(res, ":path");
	int favouriteIdx    = sqlite3_bind_parameter_index(res, ":favourite");
	int timesPlayedIdx  = sqlite3_bind_parameter_index(res, ":times_played");
	int lastPlayedIdx   = sqlite3_bind_parameter_index(res, ":last_played");
	int hiddenIdx       = sqlite3_bind_parameter_index(res, ":hidden");
	int deletedIdx      = sqlite3_bind_parameter_index(res, ":deleted");
	int userTitleIdx    = sqlite3_bind_parameter_index(res, ":user_title");
	int yearIdx         = sqlite3_bind_parameter_index(res, ":year");
	int playersIdx      = sqlite3_bind_parameter_index(res, ":players");
	int chipsetIdx      = sqlite3_bind_parameter_index(res, ":chipset");

	sqlite3_bind_text(res, titleIdx, title, -1, 0);
	sqlite3_bind_text(res, genreIdx, genre, -1, 0);
	sqlite3_bind_text(res, pathIdx, path, -1, 0);
	sqlite3_bind_int(res, favouriteIdx, favourite);
	sqlite3_bind_int(res, timesPlayedIdx, times_played);
	sqlite3_bind_int(res, lastPlayedIdx, last_played);
	sqlite3_bind_int(res, hiddenIdx, hidden);
	sqlite3_bind_int(res, deletedIdx, deleted);
	sqlite3_bind_text(res, userTitleIdx, user_title, -1, 0);
	sqlite3_bind_int(res, yearIdx, year);
	sqlite3_bind_int(res, playersIdx, players);
	sqlite3_bind_text(res, chipsetIdx, chipset, -1, 0);

	rc = sqlite3_step(res);
	sqlite3_finalize(res);
	// sqlite3_close(db);

	if (rc != SQLITE_DONE) {
		printf("Failed to insert item: %s\n", sqlite3_errmsg(db));
		return FALSE;
	}

	return TRUE;
}

BOOL sqlGetItems(int (*resultCallback)(void *, int, char **, char **))
{
  sqlite3 *db;
  char *err_msg = 0;
  int sqlSize = 64;

  // Initialize path if not already done
  const char *dbPath = getDBPath();
  if (!dbPath || dbPath[0] == '\0') {
    printf("ERROR: Database path not initialized\n");
    return FALSE;
  }
  printf("Opening database for query: %s\n", dbPath);
  int rc = sqlite3_open_v2(dbPath, &db, SQLITE_OPEN_READWRITE, NULL);
  if (rc != SQLITE_OK) {
    printf("Cannot open database: %s (error code: %d)\n", sqlite3_errmsg(db), rc);
    if (db) sqlite3_close(db);
    return FALSE;
  }
  // Set busy timeout to 5 seconds to allow for lock retries
  sqlite3_busy_timeout(db, 5000);

  STRPTR sql = malloc(sqlSize * sizeof(char));

  snprintf(sql, sqlSize,
    "SELECT * FROM items"
  );

  rc = sqlite3_exec(db, sql, resultCallback, 0, &err_msg);

  free(sql);

  if (rc != SQLITE_OK) {
    //fprintf(stderr, "SQL error: %s\n", err_msg);
    if (err_msg) sqlite3_free(err_msg);
    sqlite3_close(db);
    return FALSE;
  }
  sqlite3_close(db);

  return TRUE;
}

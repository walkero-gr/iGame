
#ifndef _SQLDB_H
#define _SQLDB_H

#include <sqlite3.h>

void printSqliteVer(void);
BOOL initializeDBPath(void);
const char* getDBPath(void);
BOOL createDB(void);
sqlite3 *sqlDBOpen(void);
void sqlDBClose(sqlite3 *);
BOOL sqlBeginTransaction(sqlite3 *db);
BOOL sqlCommitTransaction(sqlite3 *db);
BOOL sqlRollbackTransaction(sqlite3 *db);
BOOL sqlAddItem(STRPTR, STRPTR, STRPTR, int, int, int, int, int, STRPTR, int, int, STRPTR, sqlite3*);
BOOL sqlGetItems(int (*)(void *, int, char **, char **));
// BOOL sqlAddFavouriteRadio(STRPTR, STRPTR, STRPTR, STRPTR, STRPTR, STRPTR, STRPTR);
// BOOL sqlAddFavouritePodcast(STRPTR, STRPTR, STRPTR, STRPTR, STRPTR, STRPTR);
// BOOL sqlAddListenLaterPodcast(STRPTR, STRPTR, STRPTR, STRPTR, STRPTR, STRPTR, STRPTR, uint8, ULONG, ULONG);
// BOOL sqlRemoveFavourite(STRPTR, CONST_STRPTR);
// BOOL sqlRemoveListenLater(STRPTR);
// BOOL sqlCheckFavouriteExist(STRPTR, CONST_STRPTR);
// BOOL sqlCheckListenLaterExist(STRPTR);
// BOOL sqlGetFavourites(CONST_STRPTR, int (*)(void *, int, char **, char **));
// BOOL sqlGetListenLater(int (*)(void *, int, char **, char **));

#endif





#include <stdio.h>
#include <stdlib.h>

#include <proto/dos.h>

#include "sqldb.h"


static char dbFileName[] = "data.db";

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
  
  int rc = sqlite3_open(dbFileName, &db);
  if (rc != SQLITE_OK) {
    fprintf(stderr, "Cannot open database: %s\n", sqlite3_errmsg(db));
    sqlite3_close(db);
    return FALSE;
  }

  CONST_STRPTR sql =
      "CREATE TABLE version ( " \
      "     ver Integer); " \
      "INSERT into version VALUES (\"1\");" \
      "CREATE TABLE items ( " \
          "id           Numeric, " \
          "title        Text, " \
          "genre        Text, " \
          "path         Text, " \
          "favourite    Numeric, " \
          "times_played Numeric, " \
          "last_played  Numeric, " \
          "hidden       Numeric); ";

  rc = sqlite3_exec(db, sql, 0, 0, &err_msg);
  if (rc != SQLITE_OK) {
    //fprintf(stderr, "SQL error: %s\n", err_msg);
    sqlite3_free(err_msg);
    sqlite3_close(db);
    return FALSE;
  }
  sqlite3_close(db);

  return TRUE;
}

sqlite3 *sqlDBOpen(void)
{
  sqlite3 *db;
  // sqlite3_stmt *res;

  int rc = sqlite3_open(dbFileName, &db);
  if (rc != SQLITE_OK) {
    fprintf(stderr, "Cannot open database: %s\n", sqlite3_errmsg(db));
    sqlite3_close(db);
    return 0;
  }
  
  return db;
}

void sqlDBClose(sqlite3 *db)
{
  sqlite3_close(db);
}

BOOL sqlAddItem(
  int id,
  STRPTR title,
  STRPTR genre,
  STRPTR path,
  int favourite,
  int times_played,
  int last_played,
  int hidden,
  sqlite3 *db
) {
  // sqlite3 *db;
  sqlite3_stmt *res;

  // int rc = sqlite3_open(dbFileName, &db);
  // if (rc != SQLITE_OK) {
  //   fprintf(stderr, "Cannot open database: %s\n", sqlite3_errmsg(db));
  //   sqlite3_close(db);
  //   return FALSE;
  // }

  CONST_STRPTR sql = "INSERT INTO items " \
    "(" \
      "id, title, genre, path, favourite, " \
      "times_played, last_played, hidden" \
    ") " \
    "VALUES (" \
      ":id, :title, :genre, :path, :favourite, " \
      ":times_played, :last_played, :hidden" \
    ")";

  int rc = sqlite3_prepare_v2(db, sql, -1, &res, 0);
  if (rc != SQLITE_OK) {
    fprintf(stderr, "Failed to execute statement: %s\n", sqlite3_errmsg(db));
    sqlite3_close(db);
    return FALSE;
  }

  int idIdx           = sqlite3_bind_parameter_index(res, ":id");
  int titleIdx        = sqlite3_bind_parameter_index(res, ":title");
  int genreIdx        = sqlite3_bind_parameter_index(res, ":genre");
  int pathIdx         = sqlite3_bind_parameter_index(res, ":path");
  int favouriteIdx    = sqlite3_bind_parameter_index(res, ":favourite");
  int timesPlayedIdx  = sqlite3_bind_parameter_index(res, ":times_played");
  int lastPlayedIdx   = sqlite3_bind_parameter_index(res, ":last_played");
  int hiddenIdx       = sqlite3_bind_parameter_index(res, ":hidden");

  sqlite3_bind_int(res, idIdx, id);
  sqlite3_bind_text(res, titleIdx, title, -1, 0);
  sqlite3_bind_text(res, genreIdx, genre, -1, 0);
  sqlite3_bind_text(res, pathIdx, path, -1, 0);
  sqlite3_bind_int(res, favouriteIdx, favourite);
  sqlite3_bind_int(res, timesPlayedIdx, times_played);
  sqlite3_bind_int(res, lastPlayedIdx, last_played);
  sqlite3_bind_int(res, hiddenIdx, hidden);

  sqlite3_step(res);
  sqlite3_finalize(res);
  // sqlite3_close(db);

  return TRUE;
}

BOOL sqlGetItems(int (*resultCallback)(void *, int, char **, char **))
{
  sqlite3 *db;
  char *err_msg = 0;
  int sqlSize = 64;

  int rc = sqlite3_open(dbFileName, &db);
  if (rc != SQLITE_OK) {
    //fprintf(stderr, "Cannot open database: %s\n", sqlite3_errmsg(db));
    sqlite3_close(db);
    return FALSE;
  }

  STRPTR sql = malloc(sqlSize * sizeof(char));

  snprintf(sql, sqlSize,
    "SELECT * FROM items"
  );

  rc = sqlite3_exec(db, sql, resultCallback, 0, &err_msg);

  free(sql);

  if (rc != SQLITE_OK) {
    //fprintf(stderr, "SQL error: %s\n", err_msg);
    sqlite3_free(err_msg);
    sqlite3_close(db);
    return FALSE;
  }
  sqlite3_close(db);

  return TRUE;
}

// BOOL sqlAddFavouriteRadio(
//   STRPTR uuid,
//   STRPTR title,
//   STRPTR country,
//   STRPTR bitrate,
//   STRPTR codec,
//   STRPTR url,
//   STRPTR image
// ) {
//   sqlite3 *db;
//   sqlite3_stmt *res;

//   int rc = sqlite3_open(dbFileName, &db);
//   if (rc != SQLITE_OK) {
//     //fprintf(stderr, "Cannot open database: %s\n", sqlite3_errmsg(db));
//     sqlite3_close(db);
//     return FALSE;
//   }

//   CONST_STRPTR sql = "INSERT INTO favourites " \
//     "(uuid, title, added, type, country, bitrate, codec, url, image) " \
//     "VALUES (:uuid, :title, :added, 'radio', :country, :bitrate, :codec, :url, :image)";

//   rc = sqlite3_prepare_v2(db, sql, -1, &res, 0);
//   if (rc != SQLITE_OK) {
//     //fprintf(stderr, "Failed to execute statement: %s\n", sqlite3_errmsg(db));
//     sqlite3_close(db);
//     return FALSE;
//   }

//   int uuidIdx     = sqlite3_bind_parameter_index(res, ":uuid");
//   int titleIdx    = sqlite3_bind_parameter_index(res, ":title");
//   int addedIdx    = sqlite3_bind_parameter_index(res, ":added");
//   int countryIdx  = sqlite3_bind_parameter_index(res, ":country");
//   int bitrateIdx  = sqlite3_bind_parameter_index(res, ":bitrate");
//   int codecIdx    = sqlite3_bind_parameter_index(res, ":codec");
//   int urlIdx      = sqlite3_bind_parameter_index(res, ":url");
//   int imageIdx    = sqlite3_bind_parameter_index(res, ":image");

//   sqlite3_bind_text(res, uuidIdx, uuid, -1, 0);
//   sqlite3_bind_text(res, titleIdx, title, -1, 0);
//   sqlite3_bind_int(res, addedIdx, now());
//   sqlite3_bind_text(res, countryIdx, country, -1, 0);
//   sqlite3_bind_text(res, bitrateIdx, bitrate, -1, 0);
//   sqlite3_bind_text(res, codecIdx, codec, -1, 0);
//   sqlite3_bind_text(res, urlIdx, url, -1, 0);
//   sqlite3_bind_text(res, imageIdx, image, -1, 0);

//   sqlite3_step(res);
//   sqlite3_finalize(res);
//   sqlite3_close(db);

//   return TRUE;
// }

// BOOL sqlAddFavouritePodcast(
//   STRPTR uuid,
//   STRPTR title,
//   STRPTR language,
//   STRPTR image,
//   STRPTR description,
//   STRPTR lastepisode
// ) {
//   sqlite3 *db;
//   sqlite3_stmt *res;

//   int rc = sqlite3_open(dbFileName, &db);
//   if (rc != SQLITE_OK) {
//     //fprintf(stderr, "Cannot open database: %s\n", sqlite3_errmsg(db));
//     sqlite3_close(db);
//     return FALSE;
//   }

//   CONST_STRPTR sql = "INSERT INTO favourites " \
//     "(uuid, title, added, lastepisode, type, language, image, description) " \
//     "VALUES (:uuid, :title, :added, :lastepisode, 'podcast', :language, :image, :descr)";

//   rc = sqlite3_prepare_v2(db, sql, -1, &res, 0);
//   if (rc != SQLITE_OK) {
//     //fprintf(stderr, "Failed to execute statement: %s\n", sqlite3_errmsg(db));
//     sqlite3_close(db);
//     return FALSE;
//   }

//   int uuidIdx     = sqlite3_bind_parameter_index(res, ":uuid");
//   int titleIdx    = sqlite3_bind_parameter_index(res, ":title");
//   int addedIdx    = sqlite3_bind_parameter_index(res, ":added");
//   int lastEpIdx   = sqlite3_bind_parameter_index(res, ":lastepisode");
//   int languageIdx = sqlite3_bind_parameter_index(res, ":language");
//   int imageIdx    = sqlite3_bind_parameter_index(res, ":image");
//   int descrIdx    = sqlite3_bind_parameter_index(res, ":descr");
  
//   sqlite3_bind_text(res, uuidIdx, uuid, -1, 0);
//   sqlite3_bind_text(res, titleIdx, title, -1, 0);
//   sqlite3_bind_int(res, addedIdx, now());
//   sqlite3_bind_text(res, lastEpIdx, lastepisode, -1, 0);
//   sqlite3_bind_text(res, languageIdx, language, -1, 0);
//   sqlite3_bind_text(res, imageIdx, image, -1, 0);
//   sqlite3_bind_text(res, descrIdx, description, -1, 0);

//   sqlite3_step(res);
//   sqlite3_finalize(res);
//   sqlite3_close(db);

//   return TRUE;
// }

// BOOL sqlAddListenLaterPodcast(
//   STRPTR id,
//   STRPTR title,
//   STRPTR datePublishedPretty,
//   STRPTR enclosureUrl,
//   STRPTR description,
//   STRPTR feedImage,
//   STRPTR feedId,
//   uint8 season,
//   ULONG episode,
//   ULONG duration
// ) {
//   sqlite3 *db;
//   sqlite3_stmt *res;

//   int rc = sqlite3_open(dbFileName, &db);
//   if (rc != SQLITE_OK) {
//     //fprintf(stderr, "Cannot open database: %s\n", sqlite3_errmsg(db));
//     sqlite3_close(db);
//     return FALSE;
//   }

//   CONST_STRPTR sql = "INSERT INTO listenlater " \
//     "(" \
//       "id, added, title, datePublishedPretty, enclosureUrl, description, " \
//       "feedImage, feedId, season, episode, duration" \
//     ") " \
//     "VALUES (" \
//       ":id, :added, :title, :datePublishedPretty, :enclosureUrl, :descr, " \
//       ":feedImage, :feedId, :season, :episode, :duration" \
//     ")";

//   rc = sqlite3_prepare_v2(db, sql, -1, &res, 0);
//   if (rc != SQLITE_OK) {
//     //fprintf(stderr, "Failed to execute statement: %s\n", sqlite3_errmsg(db));
//     sqlite3_close(db);
//     return FALSE;
//   }

//   int idIdx                   = sqlite3_bind_parameter_index(res, ":id");
//   int addedIdx                = sqlite3_bind_parameter_index(res, ":added");
//   int titleIdx                = sqlite3_bind_parameter_index(res, ":title");
//   int datePublishedPrettyIdx  = sqlite3_bind_parameter_index(res, ":datePublishedPretty");
//   int enclosureUrlIdx         = sqlite3_bind_parameter_index(res, ":enclosureUrl");
//   int descrIdx                = sqlite3_bind_parameter_index(res, ":descr");
//   int feedImageIdx            = sqlite3_bind_parameter_index(res, ":feedImage");
//   int feedIdIdx               = sqlite3_bind_parameter_index(res, ":feedId");
//   int seasonIdx               = sqlite3_bind_parameter_index(res, ":season");
//   int episodeIdx              = sqlite3_bind_parameter_index(res, ":episode");
//   int durationIdx             = sqlite3_bind_parameter_index(res, ":duration");

//   sqlite3_bind_text(res, idIdx, id, -1, 0);
//   sqlite3_bind_int(res, addedIdx, now());
//   sqlite3_bind_text(res, titleIdx, title, -1, 0);
//   sqlite3_bind_text(res, datePublishedPrettyIdx, datePublishedPretty, -1, 0);
//   sqlite3_bind_text(res, enclosureUrlIdx, enclosureUrl, -1, 0);
//   sqlite3_bind_text(res, descrIdx, description, -1, 0);
//   sqlite3_bind_text(res, feedImageIdx, feedImage, -1, 0);
//   sqlite3_bind_text(res, feedIdIdx, feedId, -1, 0);
//   sqlite3_bind_int(res, seasonIdx, season);
//   sqlite3_bind_int(res, episodeIdx, episode);
//   sqlite3_bind_int(res, durationIdx, duration);

//   sqlite3_step(res);
//   sqlite3_finalize(res);
//   sqlite3_close(db);

//   return TRUE;
// }

// BOOL sqlRemoveFavourite(STRPTR uuid, CONST_STRPTR type)
// {
//   sqlite3 *db;
//   sqlite3_stmt *res;

//   int rc = sqlite3_open(dbFileName, &db);
//   if (rc != SQLITE_OK) {
//     //fprintf(stderr, "Cannot open database: %s\n", sqlite3_errmsg(db));
//     sqlite3_close(db);
//     return FALSE;
//   }

//   CONST_STRPTR sql = "DELETE FROM favourites " \
//         "WHERE uuid = :uuid AND type = :type ";

//   rc = sqlite3_prepare_v2(db, sql, -1, &res, 0);
//   if (rc != SQLITE_OK) {
//     //fprintf(stderr, "Failed to execute statement: %s\n", sqlite3_errmsg(db));
//     sqlite3_close(db);
//     return FALSE;
//   }

//   int uuidIdx = sqlite3_bind_parameter_index(res, ":uuid");
//   int typeIdx = sqlite3_bind_parameter_index(res, ":type");

//   sqlite3_bind_text(res, uuidIdx, uuid, -1, 0);
//   sqlite3_bind_text(res, typeIdx, type, -1, 0);

//   sqlite3_step(res);
//   sqlite3_finalize(res);
//   sqlite3_close(db);

//   return TRUE;
// }

// BOOL sqlRemoveListenLater(STRPTR id)
// {
//   sqlite3 *db;
//   sqlite3_stmt *res;

//   int rc = sqlite3_open(dbFileName, &db);
//   if (rc != SQLITE_OK) {
//     //fprintf(stderr, "Cannot open database: %s\n", sqlite3_errmsg(db));
//     sqlite3_close(db);
//     return FALSE;
//   }

//   CONST_STRPTR sql = "DELETE FROM listenlater " \
//         "WHERE id = :id ";

//   rc = sqlite3_prepare_v2(db, sql, -1, &res, 0);
//   if (rc != SQLITE_OK) {
//     //fprintf(stderr, "Failed to execute statement: %s\n", sqlite3_errmsg(db));
//     sqlite3_close(db);
//     return FALSE;
//   }

//   int idIdx = sqlite3_bind_parameter_index(res, ":id");

//   sqlite3_bind_text(res, idIdx, id, -1, 0);

//   sqlite3_step(res);
//   sqlite3_finalize(res);
//   sqlite3_close(db);

//   return TRUE;
// }

// BOOL sqlCheckFavouriteExist(STRPTR uuid, CONST_STRPTR type)
// {
//   BOOL result = FALSE;
//   sqlite3 *db;
//   sqlite3_stmt *res;

//   int rc = sqlite3_open(dbFileName, &db);
//   if (rc != SQLITE_OK) {
//     //fprintf(stderr, "Cannot open database: %s\n", sqlite3_errmsg(db));
//     sqlite3_close(db);
//     return FALSE;
//   }
  
//   CONST_STRPTR sql = "SELECT COUNT(*) AS cnt " \
//         "FROM favourites " \
//         "WHERE uuid = :uuid AND type = :type " \
//         "LIMIT 1";

//   rc = sqlite3_prepare_v2(db, sql, -1, &res, 0);
//   if (rc != SQLITE_OK) {
//     //fprintf(stderr, "Failed to execute statement: %s\n", sqlite3_errmsg(db));
//     sqlite3_close(db);
//     return FALSE;
//   }
    
//   int uuidIdx = sqlite3_bind_parameter_index(res, ":uuid");
//   int typeIdx = sqlite3_bind_parameter_index(res, ":type");
//   sqlite3_bind_text(res, uuidIdx, uuid, -1, 0);
//   sqlite3_bind_text(res, typeIdx, type, -1, 0);
  
//   int step = sqlite3_step(res);
//   if (step == SQLITE_ROW) {
//     if(sqlite3_column_int(res, 0) == 1)
//     {
//       result = TRUE;
//     }
//   }
//   sqlite3_finalize(res);
//   sqlite3_close(db);

//   return result;
// }

// BOOL sqlCheckListenLaterExist(STRPTR id)
// {
//   BOOL result = FALSE;
//   sqlite3 *db;
//   sqlite3_stmt *res;

//   int rc = sqlite3_open(dbFileName, &db);
//   if (rc != SQLITE_OK) {
//     //fprintf(stderr, "Cannot open database: %s\n", sqlite3_errmsg(db));
//     sqlite3_close(db);
//     return FALSE;
//   }

//   CONST_STRPTR sql = "SELECT COUNT(*) AS cnt " \
//         "FROM listenlater " \
//         "WHERE id = :id " \
//         "LIMIT 1";

//   rc = sqlite3_prepare_v2(db, sql, -1, &res, 0);
//   if (rc != SQLITE_OK) {
//     //fprintf(stderr, "Failed to execute statement: %s\n", sqlite3_errmsg(db));
//     sqlite3_close(db);
//     return FALSE;
//   }

//   int idIdx = sqlite3_bind_parameter_index(res, ":id");
//   sqlite3_bind_text(res, idIdx, id, -1, 0);

//   int step = sqlite3_step(res);
//   if (step == SQLITE_ROW) {
//     if(sqlite3_column_int(res, 0) == 1)
//     {
//       result = TRUE;
//     }
//   }
//   sqlite3_finalize(res);
//   sqlite3_close(db);

//   return result;
// }

// BOOL sqlGetFavourites(CONST_STRPTR type, int (*resultCallback)(void *, int, char **, char **))
// {
//   sqlite3 *db;
//   char *err_msg = 0;
//   int sqlSize = 64;

//   int rc = sqlite3_open(dbFileName, &db);
//   if (rc != SQLITE_OK) {
//     //fprintf(stderr, "Cannot open database: %s\n", sqlite3_errmsg(db));
//     sqlite3_close(db);
//     return FALSE;
//   }

//   STRPTR sql = IExec->AllocVecTags(sizeof(char) * sqlSize,
//           AVT_Type,            MEMF_SHARED,
//           AVT_ClearWithValue,  "\0",
//           TAG_DONE);

//   snprintf(sql, sqlSize,
//     "SELECT * FROM favourites " \
//     "WHERE type = '%s';", type
//   );

//   rc = sqlite3_exec(db, sql, resultCallback, 0, &err_msg);

//   IExec->FreeVec(sql);

//   if (rc != SQLITE_OK) {
//     //fprintf(stderr, "SQL error: %s\n", err_msg);
//     sqlite3_free(err_msg);
//     sqlite3_close(db);
//     return FALSE;
//   }
//   sqlite3_close(db);

//   return TRUE;
// }

// BOOL sqlGetListenLater(int (*resultCallback)(void *, int, char **, char **))
// {
//   sqlite3 *db;
//   char *err_msg = 0;
//   int sqlSize = 64;

//   int rc = sqlite3_open(dbFileName, &db);
//   if (rc != SQLITE_OK) {
//     //fprintf(stderr, "Cannot open database: %s\n", sqlite3_errmsg(db));
//     sqlite3_close(db);
//     return FALSE;
//   }

//   STRPTR sql = IExec->AllocVecTags(sizeof(char) * sqlSize,
//           AVT_Type,            MEMF_SHARED,
//           AVT_ClearWithValue,  "\0",
//           TAG_DONE);

//   snprintf(sql, sqlSize, "SELECT * FROM listenlater;");

//   rc = sqlite3_exec(db, sql, resultCallback, 0, &err_msg);

//   IExec->FreeVec(sql);

//   if (rc != SQLITE_OK) {
//     //fprintf(stderr, "SQL error: %s\n", err_msg);
//     sqlite3_free(err_msg);
//     sqlite3_close(db);
//     return FALSE;
//   }
//   sqlite3_close(db);

//   return TRUE;
// }


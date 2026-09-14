/*
  collections.h
  Collections management header for iGame

  Copyright (c) 2019, Emmanuel Vasilakis and contributors

  This file is part of iGame.

  iGame is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.

  iGame is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with iGame. If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef COLLECTIONS_H
#define COLLECTIONS_H

#include "iGameExtern.h"

/* Public API */
collection *getActiveCollection(void);
void setActiveCollection(collection *);
collection *getCollectionsHead(void);
void addCollectionToRegistry(collection *);
void removeCollectionFromRegistry(collection *);
void initCollectionsRegistry(void);
void cleanupCollectionsRegistry(void);

/* Collection switching */
void collection_switch(collection *);

/* Collection preferences I/O */
void load_collections_prefs(void);
void save_collections_prefs(void);

/* Collection name utilities */
BOOL canonicalize_collection_name(const char *, char *, int);
BOOL validate_collection_name(const char *);

#endif

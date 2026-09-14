/*
  migrate.c
  Migration functions for iGame v2 to v3 collections

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

#include <proto/exec.h>

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "iGameExtern.h"
#include "collections.h"
#include "fsfuncs.h"
#include "migrate.h"

/* Check if v2 files exist */
static BOOL v2_files_exist(void)
{
	return check_path_exists((char *)DEFAULT_GAMESLIST_FILE) ||
	       check_path_exists((char *)DEFAULT_REPOS_FILE);
}

/* Check if collections.prefs already exists (v3) */
static BOOL v3_collections_exist(void)
{
	return check_path_exists((char *)DEFAULT_COLLECTIONS_FILE);
}

/* Migrate v2 files to v3 collections registry */
void migrate_v2_to_v3(void)
{
	/* Already migrated or v2 files don't exist */
	if (v3_collections_exist() || !v2_files_exist())
		return;

	/* Create Default collection */
	collection *default_col = (collection *)malloc(sizeof(collection));
	if (!default_col)
		return;

	strncpy(default_col->name, DEFAULT_COLLECTION_NAME, MAX_COLLECTION_NAME_SIZE);
	default_col->name[MAX_COLLECTION_NAME_SIZE] = '\0';

	strcpy(default_col->filename_prefix, "gameslist");  /* Use v2 filename as-is for Default */

	default_col->scan_paths = NULL;
	default_col->is_default = 1;  /* Mark as default */
	default_col->next = NULL;

	/* Add to registry */
	addCollectionToRegistry(default_col);
	setActiveCollection(default_col);

	/* Migration complete; collections.prefs will be written after app_start() loads data */
	/* (see Phase 5: Prefs I/O — save_collections_prefs() writes at startup) */
}

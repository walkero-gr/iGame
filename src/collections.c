/*
  collections.c
  Collections management functions for iGame

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

#include <libraries/mui.h>
#include <proto/exec.h>

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <ctype.h>

#include "iGameExtern.h"
#include "iGameGUI.h"
#include "collections.h"
#include "funcs.h"
#include "slavesList.h"
#include "genresList.h"
#include "fsfuncs.h"

extern struct ObjApp* app;

static collection *collectionsHead = NULL;
static collection *collectionsTail = NULL;
static collection *activeCollection = NULL;

collection *getActiveCollection(void)
{
	return activeCollection;
}

void setActiveCollection(collection *col)
{
	activeCollection = col;
}

collection *getCollectionsHead(void)
{
	return collectionsHead;
}

void addCollectionToRegistry(collection *node)
{
	if (node != NULL)
	{
		node->next = NULL;
		if (collectionsHead == NULL)
		{
			collectionsHead = node;
			collectionsTail = node;
		}
		else
		{
			collectionsTail->next = node;
			collectionsTail = node;
		}
	}
}

void removeCollectionFromRegistry(collection *node)
{
	if (node == NULL) return;
	if (node == collectionsHead)
	{
		collectionsHead = node->next;
		if (collectionsHead == NULL) collectionsTail = NULL;
	}
	else
	{
		collection *curr = collectionsHead;
		while (curr != NULL && curr->next != node) curr = curr->next;
		if (curr != NULL)
		{
			curr->next = node->next;
			if (node == collectionsTail) collectionsTail = curr;
		}
	}
}

void initCollectionsRegistry(void)
{
	collectionsHead = NULL;
	collectionsTail = NULL;
	activeCollection = NULL;
}

void cleanupCollectionsRegistry(void)
{
	collection *curr = collectionsHead;
	while (curr != NULL)
	{
		collection *next = curr->next;
		free(curr);
		curr = next;
	}
	collectionsHead = NULL;
	collectionsTail = NULL;
	activeCollection = NULL;
}

/* Canonicalize collection name: spaces->underscores, lowercase, safe chars only */
BOOL canonicalize_collection_name(const char *name, char *result, int result_size)
{
	if (!name || !result || result_size < 2)
		return FALSE;

	int src_idx = 0, dst_idx = 0;
	while (name[src_idx] && dst_idx < result_size - 1)
	{
		char c = name[src_idx];
		if (isalnum((unsigned char)c))
		{
			result[dst_idx++] = tolower((unsigned char)c);
		}
		else if (c == ' ')
		{
			result[dst_idx++] = '_';
		}
		/* Skip other special characters */
		src_idx++;
	}
	result[dst_idx] = '\0';

	return dst_idx > 0;  /* At least one char survived */
}

/* Validate collection name */
BOOL validate_collection_name(const char *name)
{
	if (!name || strlen(name) == 0 || strlen(name) > MAX_COLLECTION_NAME_SIZE)
		return FALSE;

	/* At least one alphanumeric character */
	for (int i = 0; name[i]; i++)
	{
		if (isalnum((unsigned char)name[i]))
			return TRUE;
	}

	return FALSE;
}

/* Build full CSV path from collection name */
void build_collection_csv_path(const char *collection_name, char *result_path, int result_size)
{
	char filename_prefix[MAX_FILENAME_PREFIX_SIZE];

	canonicalize_collection_name(collection_name, filename_prefix, sizeof(filename_prefix));
	snprintf(result_path, result_size, "PROGDIR:%s", filename_prefix);
}

/* Load collections from prefs file into registry */
void load_collections_prefs(void)
{
	FILE *fp = fopen(DEFAULT_COLLECTIONS_FILE, "r");
	if (!fp)
		return;  /* No prefs file yet (first run); migration created Default */

	char line[PREFS_SECTION_MAX_SIZE];
	collection *current_col = NULL;

	while (fgets(line, sizeof(line), fp))
	{
		/* Remove trailing newline */
		size_t len = strlen(line);
		if (len > 0 && line[len - 1] == '\n')
			line[len - 1] = '\0';

		/* Skip empty lines and comments */
		if (line[0] == '\0' || line[0] == ';')
			continue;

		/* Section header: [CollectionName] */
		if (line[0] == '[' && line[strlen(line) - 1] == ']')
		{
			/* Extract collection name from [Name] */
			char col_name[MAX_COLLECTION_NAME_SIZE + 1];
			strncpy(col_name, line + 1, strlen(line) - 2);
			col_name[strlen(line) - 2] = '\0';

			/* Create new collection */
			current_col = (collection *)malloc(sizeof(collection));
			if (!current_col)
				continue;

			memset(current_col, 0, sizeof(collection));
			strncpy(current_col->name, col_name, MAX_COLLECTION_NAME_SIZE);
			current_col->name[MAX_COLLECTION_NAME_SIZE] = '\0';
			canonicalize_collection_name(col_name, current_col->filename_prefix, sizeof(current_col->filename_prefix));
			current_col->is_default = 0;
			current_col->next = NULL;

			addCollectionToRegistry(current_col);
		}
		/* Repository path (any other line under a section) */
		else if (current_col && line[0] != '\0')
		{
			/* Treat as repository path; create repos_list node */
			repos_list *repo = (repos_list *)malloc(sizeof(repos_list));
			if (!repo)
				continue;

			strncpy(repo->repo, line, sizeof(repo->repo) - 1);
			repo->repo[sizeof(repo->repo) - 1] = '\0';

			/* Prepend to collection's scan_paths */
			repo->next = current_col->scan_paths;
			current_col->scan_paths = repo;
		}
	}

	fclose(fp);
}

/* Save all collections to prefs file (atomic rewrite) */
void save_collections_prefs(void)
{
	FILE *fp = fopen(DEFAULT_COLLECTIONS_FILE, "w");
	if (!fp)
		return;  /* Error; could not create/write file */

	collection *col = getCollectionsHead();
	while (col)
	{
		/* Write section header */
		fprintf(fp, "[%s]\n", col->name);

		/* Write repository paths */
		repos_list *repo = col->scan_paths;
		while (repo)
		{
			fprintf(fp, "%s\n", repo->repo);
			repo = repo->next;
		}

		fprintf(fp, "\n");  /* Blank line between sections */
		col = col->next;
	}

	fclose(fp);
}

/* Save selected collection name to igame.prefs */
void save_default_collection_to_prefs(collection *col)
{
	if (!col)
		return;

	extern igame_settings *iGameSettings;
	if (iGameSettings)
	{
		strncpy(iGameSettings->default_collection, col->name, MAX_COLLECTION_NAME_SIZE);
		iGameSettings->default_collection[MAX_COLLECTION_NAME_SIZE] = '\0';
	}
}

/* Switch to a different collection */
void collection_switch(collection *newCollection)
{
	if (!newCollection || newCollection == activeCollection)
		return;

	/* Freeze UI */
	if (app)
		set(app->WI_MainWindow, MUIA_Window_Sleep, TRUE);

	/* Unload old collection: clear all global lists */
	emptySlavesList();
	emptyGenresList();

	/* Switch active collection */
	setActiveCollection(newCollection);

	/* Load new collection's CSV */
	char csv_path[MAX_PATH_SIZE];
	build_collection_csv_path(newCollection->name, csv_path, sizeof(csv_path));
	strcat(csv_path, ".csv");  /* Append .csv extension */

	setStatusText("Loading collection...");
	slavesListLoadFromCSV(csv_path);

	/* Refresh UI displays */
	populateGenresLists();
	filter_change();

	/* Update status */
	char status_msg[128];
	snprintf(status_msg, sizeof(status_msg), "Collection: %s", newCollection->name);
	setStatusText(status_msg);

	/* Unfreeze UI */
	if (app)
		set(app->WI_MainWindow, MUIA_Window_Sleep, FALSE);
}

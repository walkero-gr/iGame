/*
  WinCollections.c
  Collections window for iGame

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
#include <clib/alib_protos.h>
#include <proto/muimaster.h>
#include <proto/asl.h>
#include <SDI_hook.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#define iGame_NUMBERS
#include "iGame_strings.h"

#include "iGameExtern.h"
#include "iGameGUI.h"
#include "collections.h"
#include "fsfuncs.h"
#include "strfuncs.h"
#include "funcs.h"
#include "WinCollections.h"

extern struct ObjApp* app;

static char last_selected_path[MAX_PATH_SIZE] = "PROGDIR:";

/* PopASL StartHook - called before the file browser dialog opens */
HOOKPROTO(collections_popasl_start_hook, LONG, APTR, struct TagItem *tags)
{
	while (tags->ti_Tag) tags++;  // Find end

	tags->ti_Tag = ASLFR_InitialDrawer;
	tags->ti_Data = (ULONG)last_selected_path;
	tags++;

	tags->ti_Tag = TAG_END;

	return TRUE;
}
MakeStaticHook(CollectionsPopaslStartHook, collections_popasl_start_hook);

/* PopASL StopHook - called when the file browser dialog closes */
HOOKPROTO(collections_popasl_stop_hook, void, Object *pop, struct FileRequester *req)
{
	if (req && req->fr_Drawer)
	{
		char full_path[MAX_PATH_SIZE];

		/* Build full path from drawer and file */
		snprintf(full_path, sizeof(full_path), "%s", req->fr_Drawer);
		// getParentPath(req->fr_Drawer, full_path, sizeof(full_path));

		/* Populate the string field so collection_path_add_handler can use it */
		set(app->STR_CollectionRepoPath, MUIA_String_Contents, full_path);

		/* Store the selected path for next time the requester opens */
		strncpy(last_selected_path, req->fr_Drawer, MAX_PATH_SIZE - 1);

		// TODO: If I decide to use by default the parent path, the following can make the trick.
		// getParentPath(req->fr_Drawer, last_selected_path, sizeof(last_selected_path));

		last_selected_path[MAX_PATH_SIZE - 1] = '\0';
	}
}
MakeStaticHook(CollectionsPopaslStopHook, collections_popasl_stop_hook);

/* Add new collection */
static void collections_add_handler(void)
{
	/* Prompt user for collection name (simplified: default name) */
	char new_name[MAX_COLLECTION_NAME_SIZE + 1];
	strcpy(new_name, (CONST_STRPTR)GetMBString(MSG_DefaultCollectionName));

	/* Create collection struct */
	collection *new_col = (collection *)malloc(sizeof(collection));
	if (!new_col)
		return;

	memset(new_col, 0, sizeof(collection));
	strncpy(new_col->name, new_name, MAX_COLLECTION_NAME_SIZE);
	new_col->name[MAX_COLLECTION_NAME_SIZE] = '\0';
	canonicalize_collection_name(new_name, new_col->filename_prefix, sizeof(new_col->filename_prefix));
	new_col->scan_paths = NULL;
	new_col->is_default = 0;
	new_col->next = NULL;

	/* Add to registry */
	addCollectionToRegistry(new_col);

	/* Update UI */
	DoMethod(app->LV_Collections, MUIM_List_InsertSingle, new_col->name, MUIV_List_Insert_Bottom);
	DoMethod(app->LV_Collections, MUIM_List_Sort);
}

/* Remove selected collection */
static void collections_remove_handler(void)
{
	char *selected_name = NULL;
	DoMethod(app->LV_Collections, MUIM_List_GetEntry, MUIV_List_GetEntry_Active, &selected_name);

	if (!selected_name)
		return;

	/* Find collection in registry */
	collection *col = getCollectionsHead();
	while (col)
	{
		if (strcmp(col->name, selected_name) == 0)
		{
			/* Don't delete if it's the active collection */
			if (col == getActiveCollection())
				return;

			/* Remove from registry and free */
			removeCollectionFromRegistry(col);
			free(col);

			/* Update UI */
			DoMethod(app->LV_Collections, MUIM_List_Remove, MUIV_List_Remove_Active);
			break;
		}
		col = col->next;
	}
}

/* Selection changed: update right-side details */
static void collections_select_handler(void)
{
	char *selected_name = NULL;
	DoMethod(app->LV_Collections, MUIM_List_GetEntry, MUIV_List_GetEntry_Active, &selected_name);

	if (!selected_name)
	{
		/* Disable detail fields if no selection */
		set(app->STR_CollectionName, MUIA_Disabled, TRUE);
		set(app->CH_CollectionDefault, MUIA_Disabled, TRUE);
		set(app->LV_CollectionRepoPaths, MUIA_Disabled, TRUE);
		set(app->PA_CollectionRepoPath, MUIA_Disabled, TRUE);
		set(app->BT_CollectionPathAdd, MUIA_Disabled, TRUE);
		set(app->BT_CollectionPathRemove, MUIA_Disabled, TRUE);
		return;
	}

	/* Find collection in registry */
	collection *col = getCollectionsHead();
	while (col)
	{
		if (strcmp(col->name, selected_name) == 0)
		{

			/* Enable detail fields */
			set(app->STR_CollectionName, MUIA_Disabled, FALSE);
			set(app->CH_CollectionDefault, MUIA_Disabled, FALSE);
			set(app->LV_CollectionRepoPaths, MUIA_Disabled, FALSE);
			set(app->PA_CollectionRepoPath, MUIA_Disabled, FALSE);
			set(app->BT_CollectionPathAdd, MUIA_Disabled, FALSE);
			/* BT_CollectionPathRemove stays disabled until a path is selected */

			/* Update name field */
			set(app->STR_CollectionName, MUIA_String_Contents, col->name);

			/* Update default checkbox */
			set(app->CH_CollectionDefault, MUIA_Selected, col->is_default);

			/* Update repository paths list */
			DoMethod(app->LV_CollectionRepoPaths, MUIM_List_Clear);

			/* Use quiet mode for efficient batch insertion */
			set(app->LV_CollectionRepoPaths, MUIA_List_Quiet, TRUE);

			repos_list *repo = col->scan_paths;
			while (repo)
			{
				DoMethod(app->LV_CollectionRepoPaths, MUIM_List_InsertSingle, repo->repo, MUIV_List_Insert_Bottom);
				repo = repo->next;
			}

			/* Disable quiet mode and sort */
			set(app->LV_CollectionRepoPaths, MUIA_List_Quiet, FALSE);
			DoMethod(app->LV_CollectionRepoPaths, MUIM_List_Sort);

			/* Clear path selection so remove button stays disabled */
			set(app->LV_CollectionRepoPaths, MUIA_List_Active, MUIV_List_Active_Off);

			break;
		}
		col = col->next;
	}
}

/* Rename collection (on string field change) */
static void collections_rename_handler(void)
{
	char *selected_name = NULL;
	char *new_name = NULL;

	DoMethod(app->LV_Collections, MUIM_List_GetEntry, MUIV_List_GetEntry_Active, &selected_name);
	get(app->STR_CollectionName, MUIA_String_Contents, &new_name);

	if (!selected_name || !new_name || !validate_collection_name(new_name))
		return;

	/* Find collection and update */
	collection *col = getCollectionsHead();
	while (col)
	{
		if (strcmp(col->name, selected_name) == 0)
		{
			strncpy(col->name, new_name, MAX_COLLECTION_NAME_SIZE);
			col->name[MAX_COLLECTION_NAME_SIZE] = '\0';
			canonicalize_collection_name(new_name, col->filename_prefix, sizeof(col->filename_prefix));

			/* Update listview */
			DoMethod(app->LV_Collections, MUIM_List_Redraw, MUIV_List_Redraw_Active);

			break;
		}
		col = col->next;
	}
}

/* Update default status for selected collection */
static void collections_default_handler(void)
{
	char *selected_name = NULL;
	int is_default = 0;

	DoMethod(app->LV_Collections, MUIM_List_GetEntry, MUIV_List_GetEntry_Active, &selected_name);
	get(app->CH_CollectionDefault, MUIA_Selected, &is_default);

	if (!selected_name)
		return;

	/* Find collection and update */
	collection *col = getCollectionsHead();
	while (col)
	{
		if (strcmp(col->name, selected_name) == 0)
		{
			col->is_default = is_default;
			break;
		}
		col = col->next;
	}
}

/* Path selection changed: enable/disable remove button */
static void collection_path_select_handler(void)
{
	char *selected_path = NULL;
	DoMethod(app->LV_CollectionRepoPaths, MUIM_List_GetEntry, MUIV_List_GetEntry_Active, &selected_path);

	if (selected_path)
		set(app->BT_CollectionPathRemove, MUIA_Disabled, FALSE);
	else
		set(app->BT_CollectionPathRemove, MUIA_Disabled, TRUE);
}

/* Check if path already exists in collection's paths */
static int path_exists_in_collection(collection *col, const char *path)
{
	if (!col || !path)
		return 0;

	repos_list *repo = col->scan_paths;
	while (repo)
	{
		if (strcmp(repo->repo, path) == 0)
			return 1;
		repo = repo->next;
	}
	return 0;
}

/* Find insertion position for alphabetical ordering in MUI list */
/* Add path to selected collection */
static void collection_path_add_handler(void)
{
	char *selected_name = NULL;
	char *repo_path = NULL;
	int bufSize = sizeof(char) * MAX_PATH_SIZE;
	char *buf = malloc(bufSize);

	if (!buf)
		return;

	DoMethod(app->LV_Collections, MUIM_List_GetEntry, MUIV_List_GetEntry_Active, &selected_name);
	get(app->STR_CollectionRepoPath, MUIA_String_Contents, &repo_path);

	if (!selected_name || !repo_path || strlen(repo_path) == 0)
	{
		free(buf);
		return;
	}

	/* Find collection */
	collection *col = getCollectionsHead();
	while (col)
	{
		if (strcmp(col->name, selected_name) == 0)
		{
			/* Create repos_list node */
			repos_list *repo = (repos_list *)malloc(sizeof(repos_list));
			if (!repo)
			{
				free(buf);
				return;
			}

			/* Remove trailing slash if present */
			char *path_copy = malloc(strlen(repo_path) + 1);
			if (!path_copy)
			{
				free(repo);
				free(buf);
				return;
			}
			strcpy(path_copy, repo_path);
			if (strlen(path_copy) > 0 && path_copy[strlen(path_copy) - 1] == '/')
				path_copy[strlen(path_copy) - 1] = '\0';

			/* Get full path */
			getFullPath(path_copy, buf);
			strcpy(repo->repo, buf);
			if (isPathOnAssign(path_copy))
				strcpy(repo->repo, path_copy);

			free(path_copy);

			/* Check if path already exists in collection */
			if (path_exists_in_collection(col, repo->repo))
			{
				msg_box(GetMBString(MSG_ERR_CollectionPathExists));
				free(repo);
				free(buf);
				return;
			}

			/* Add to collection's scan_paths at end */
			repo->next = NULL;
			if (col->scan_paths == NULL)
			{
				col->scan_paths = repo;
			}
			else
			{
				/* Find tail and append */
				repos_list *tail = col->scan_paths;
				while (tail->next != NULL)
					tail = tail->next;
				tail->next = repo;
			}

			/* Update UI and sort */
			DoMethod(app->LV_CollectionRepoPaths, MUIM_List_InsertSingle, repo->repo, MUIV_List_Insert_Bottom);
			DoMethod(app->LV_CollectionRepoPaths, MUIM_List_Sort);

			/* Clear path input */
			set(app->STR_CollectionRepoPath, MUIA_String_Contents, "");

			break;
		}
		col = col->next;
	}

	free(buf);
}

/* Remove selected path from collection */
static void collection_path_remove_handler(void)
{
	char *selected_name = NULL;
	char *path = NULL;

	DoMethod(app->LV_Collections, MUIM_List_GetEntry, MUIV_List_GetEntry_Active, &selected_name);
	DoMethod(app->LV_CollectionRepoPaths, MUIM_List_GetEntry, MUIV_List_GetEntry_Active, &path);

	if (!selected_name || !path)
		return;

	/* Find collection */
	collection *col = getCollectionsHead();
	while (col)
	{
		if (strcmp(col->name, selected_name) == 0)
		{
			/* Find and remove path from collection's scan_paths */
			repos_list *current = col->scan_paths;
			repos_list *prev = NULL;

			while (current)
			{
				if (strcmp(current->repo, path) == 0)
				{
					if (prev)
						prev->next = current->next;
					else
						col->scan_paths = current->next;

					free(current);

					/* Update UI */
					DoMethod(app->LV_CollectionRepoPaths, MUIM_List_Remove, MUIV_List_Remove_Active);
					return;
				}

				prev = current;
				current = current->next;
			}
			break;
		}
		col = col->next;
	}
}

/* Save all collections to prefs file and close window */
static void save_collections(void)
{
	save_collections_prefs();
}

/* Create Collections window UI */
APTR getCollectionsWindow(struct ObjApp *object)
{

	/* Create String object for path input first */
	object->STR_CollectionRepoPath = String("", MAX_PATH_SIZE);

	/* Create PopaslObject */
	object->PA_CollectionRepoPath = PopaslObject,
		MUIA_Popasl_Type, ASL_FileRequest,
		MUIA_Popasl_StartHook, &CollectionsPopaslStartHook,
		MUIA_Popasl_StopHook, &CollectionsPopaslStopHook,
		MUIA_Popstring_String, object->STR_CollectionRepoPath,
		MUIA_Popstring_Button, PopButton(MUII_PopDrawer),
		ASLFR_TitleText, GetMBString(MSG_SelectDir),
		ASLFR_DrawersOnly, TRUE,
		MUIA_Disabled, TRUE,
	End;

	return WindowObject,
		MUIA_Window_Title, (ULONG)GetMBString(MSG_WI_Collections),
		MUIA_Window_ID, MAKE_ID('C', 'O', 'L', '1'),
		MUIA_HelpNode, "WINCOLLECTIONS",
		WindowContents, VGroup,

			/* Main layout: left side (collections list) + right side (details) */
			Child, HGroup,
				MUIA_Weight, 50,

				/* LEFT SIDE: Collections Listview */
				Child, VGroup,
					MUIA_Weight, 20,
					Child, TextObject,
						MUIA_Text_Contents, (ULONG)GetMBString(MSG_LA_Collections),
					End,
					Child, object->LV_Collections = ListviewObject,
						MUIA_Listview_List, ListObject,
							InputListFrame,
							MUIA_List_AutoVisible, TRUE,
						End,
						MUIA_Weight, 100,
					End,

					/* Add/Remove buttons */
					Child, HGroup, MUIA_Group_SameSize, FALSE,
						Child, object->BT_CollectionsAdd = SimpleButton((STRPTR)GetMBString(MSG_BT_CollectionsAdd)),
						Child, object->BT_CollectionsRemove = SimpleButton((STRPTR)GetMBString(MSG_BT_CollectionsRemove)),
						Child, HVSpace,
					End,
				End,

				Child, BalanceObject,
				End,

				/* RIGHT SIDE: Collection details */
				Child, VGroup,
					MUIA_Weight, 80,

					/* Collection name */
					Child, TextObject,
						MUIA_Text_Contents, (ULONG)GetMBString(MSG_LA_CollectionName),
					End,
					Child, object->STR_CollectionName = StringObject,
						MUIA_Frame, MUIV_Frame_String,
						MUIA_String_MaxLen, MAX_COLLECTION_NAME_SIZE,
						MUIA_Disabled, TRUE,
					End,

					/* Default checkbox */
					Child, HGroup,
						Child, object->CH_CollectionDefault = CheckMark(FALSE),
						Child, TextObject,
							MUIA_Text_Contents, (ULONG)GetMBString(MSG_CH_CollectionDefault),
						End,
					End,

					/* Repository paths list */
					// Child, TextObject,
					// 	MUIA_Text_Contents, (ULONG)GetMBString(MSG_LA_CollectionPaths),
					// End,
					Child, GroupObject,
						MUIA_Weight, 100,
						MUIA_Frame, MUIV_Frame_Group,
						MUIA_FrameTitle, GetMBString(MSG_LA_CollectionPaths),
						Child, object->LV_CollectionRepoPaths = ListviewObject,
							MUIA_Listview_List, ListObject,
								InputListFrame,
								MUIA_List_AutoVisible, TRUE,
							End,
							MUIA_Weight, 80,
							MUIA_Disabled, TRUE,
						End,

						/* Path input and selector */
						Child, HGroup, MUIA_Group_SameSize, FALSE,
							Child, object->PA_CollectionRepoPath,
						End,

						/* Add/Remove path buttons */
						Child, HGroup, MUIA_Group_SameSize, FALSE,
							Child, object->BT_CollectionPathAdd = SimpleButton((STRPTR)GetMBString(MSG_BT_CollectionPathAdd)),
							Child, object->BT_CollectionPathRemove = SimpleButton((STRPTR)GetMBString(MSG_BT_CollectionPathRemove)),
							Child, HVSpace,
						End,
					End,
				End,
			End,

			/* Separator */
			Child, MUI_MakeObject(MUIO_HBar, 4),

			/* OK/Cancel buttons */
			Child, HGroup, MUIA_Group_SameSize, TRUE,
				Child, object->BT_CollectionsOK = MUI_MakeObject(MUIO_Button, (STRPTR)GetMBString(MSG_BT_CollectionsSAVE)),
				Child, object->BT_CollectionsCancel = MUI_MakeObject(MUIO_Button, (STRPTR)GetMBString(MSG_BT_CollectionsCancel)),
			End,
		End,
	End;
}

/* Hook setup */
void setCollectionsWindowMethods(struct ObjApp *object)
{
	MakeStaticHook(CollectionsAddHook, collections_add_handler);
	MakeStaticHook(CollectionsRemoveHook, collections_remove_handler);
	MakeStaticHook(CollectionsSelectHook, collections_select_handler);
	MakeStaticHook(CollectionsRenameHook, collections_rename_handler);
	MakeStaticHook(CollectionsDefaultHook, collections_default_handler);
	MakeStaticHook(CollectionPathAddHook, collection_path_add_handler);
	MakeStaticHook(CollectionPathRemoveHook, collection_path_remove_handler);
	MakeStaticHook(CollectionPathSelectHook, collection_path_select_handler);
	MakeStaticHook(CollectionsSaveHook, save_collections);

	/* Initialize path remove button as disabled (until path selected) */
	set(object->BT_CollectionPathRemove, MUIA_Disabled, TRUE);

	/* Initialize add path button as disabled (until collection selected) */
	set(object->BT_CollectionPathAdd, MUIA_Disabled, TRUE);

	/* Initialize default checkbox as disabled (until collection selected) */
	set(object->CH_CollectionDefault, MUIA_Disabled, TRUE);

	/* Window close request (X button) */
	DoMethod(object->WI_Collections,
		MUIM_Notify, MUIA_Window_CloseRequest, TRUE,
		object->WI_Collections, 3,
		MUIM_Set, MUIA_Window_Open, FALSE
	);

	/* Add button */
	DoMethod(object->BT_CollectionsAdd,
		MUIM_Notify, MUIA_Pressed, FALSE,
		object->App, 2,
		MUIM_CallHook, &CollectionsAddHook
	);

	/* Remove button */
	DoMethod(object->BT_CollectionsRemove,
		MUIM_Notify, MUIA_Pressed, FALSE,
		object->App, 2,
		MUIM_CallHook, &CollectionsRemoveHook
	);

	/* Collection selection change */
	DoMethod(object->LV_Collections,
		MUIM_Notify, MUIA_List_Active, MUIV_EveryTime,
		MUIV_Notify_Self, 2,
		MUIM_CallHook, &CollectionsSelectHook
	);

	/* Collection name edit */
	DoMethod(object->STR_CollectionName,
		MUIM_Notify, MUIA_String_Contents, MUIV_EveryTime,
		object->App, 2,
		MUIM_CallHook, &CollectionsRenameHook
	);

	/* Default checkbox change */
	DoMethod(object->CH_CollectionDefault,
		MUIM_Notify, MUIA_Selected, MUIV_EveryTime,
		object->App, 2,
		MUIM_CallHook, &CollectionsDefaultHook
	);

	/* Add path button */
	DoMethod(object->BT_CollectionPathAdd,
		MUIM_Notify, MUIA_Pressed, FALSE,
		object->App, 2,
		MUIM_CallHook, &CollectionPathAddHook
	);

	/* Remove path button */
	DoMethod(object->BT_CollectionPathRemove,
		MUIM_Notify, MUIA_Pressed, FALSE,
		object->App, 2,
		MUIM_CallHook, &CollectionPathRemoveHook
	);

	/* Path selection change: enable/disable remove button */
	DoMethod(object->LV_CollectionRepoPaths,
		MUIM_Notify, MUIA_List_Active, MUIV_EveryTime,
		MUIV_Notify_Self, 2,
		MUIM_CallHook, &CollectionPathSelectHook
	);

	/* OK button: save and close */
	DoMethod(object->BT_CollectionsOK,
		MUIM_Notify, MUIA_Pressed, FALSE,
		object->App, 2,
		MUIM_CallHook, &CollectionsSaveHook
	);

	DoMethod(object->BT_CollectionsOK,
		MUIM_Notify, MUIA_Pressed, FALSE,
		object->WI_Collections, 3,
		MUIM_Set, MUIA_Window_Open, FALSE
	);

	/* Cancel button: close without saving */
	DoMethod(object->BT_CollectionsCancel,
		MUIM_Notify, MUIA_Pressed, FALSE,
		object->WI_Collections, 3,
		MUIM_Set, MUIA_Window_Open, FALSE
	);
}

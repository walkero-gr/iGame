/*
  iGameMain.c
  Main source for iGame

  Copyright (c) 2018, Emmanuel Vasilakis

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

#define MUI_OBSOLETE
#define MUIMASTER_LIBRARY "muimaster.library"

/* MUI */
#include <libraries/mui.h>

/* Prototypes */
#include <clib/alib_protos.h>
#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/lowlevel.h>
#include <proto/muimaster.h>

/* ANSI C */
#include <stdio.h>
#include <stdlib.h>

#include "iGameExtern.h"

/* Increase stack size */
#if defined(__amigaos4__)
static const char USED min_stack[] = "$STACK:102400";
#else
LONG __stack = 32768;
#endif


#ifndef MAKE_ID
#define MAKE_ID(a,b,c,d) ((ULONG) (a)<<24 | (ULONG) (b)<<16 | (ULONG) (c)<<8 | (ULONG) (d))
#endif

#include "iGameGUI.h"

struct ObjApp* app = NULL; /* Object */
extern void app_stop();
extern igame_settings* load_settings(const char* filename);
extern char* get_executable_name(int argc, char** argv);

struct Library* MUIMasterBase;
#if defined(__amigaos4__)
struct MUIMasterIFace *IMUIMaster;
#endif
struct Library* TextEditorMCC;
struct Library* GuiGfxMCC;
struct Library* GuiGfxBase;
struct Library* RenderLibBase;
struct Library* LowLevelBase;
char *executable_name;

igame_settings* iGameSettings = NULL;

igame_settings* iGameSettings = NULL;

void joystick_buttons(ULONG val)
{
	//if (val & JPF_BUTTON_PLAY) printf("[PLAY/MMB]\n");
	//if (val & JPF_BUTTON_REVERSE) printf("[REVERSE]\n");
	//if (val & JPF_BUTTON_FORWARD) printf("[FORWARD]\n");
	//if (val & JPF_BUTTON_GREEN) printf("[SHUFFLE]\n");
	if (val & JPF_BUTTON_RED)
	{
		launch_game();
	}
	//if (val & JPF_BUTTON_BLUE) printf("[STOP/RMB]\n");
}

void joystick_directions(ULONG val)
{
	if (val & JPF_JOY_UP)
		set(app->LV_GamesList, MUIA_List_Active, MUIV_List_Active_Up);

	if (val & JPF_JOY_DOWN)
		set(app->LV_GamesList, MUIA_List_Active, MUIV_List_Active_Down);

	if (val & JPF_JOY_LEFT)
	  joy_left();

	if (val & JPF_JOY_RIGHT)
	  joy_right();
}

void joystick_input(ULONG val)
{
	if ((val & JP_TYPE_MASK) == JP_TYPE_NOTAVAIL)
		return;
	if ((val & JP_TYPE_MASK) == JP_TYPE_UNKNOWN)
		return;
	if ((val & JP_TYPE_MASK) == JP_TYPE_MOUSE)
		return;

	if ((val & JP_TYPE_MASK) == JP_TYPE_JOYSTK)
	{
		joystick_directions(val);
		joystick_buttons(val);
	}

	if ((val & JP_TYPE_MASK) == JP_TYPE_GAMECTLR)
	{
		joystick_directions(val);
		joystick_buttons(val);
	}
}

void clean_exit(CONST_STRPTR msg)
{
	if (app)
		app_stop();

	executable_name = NULL;

	#if defined(__amigaos4__)
	if (IMUIMaster)
		DropInterface((struct Interface *)IMUIMaster);
	#endif
	if (MUIMasterBase)
		CloseLibrary(MUIMasterBase);
	if (LowLevelBase)
		CloseLibrary(LowLevelBase);

	if(app)
		DisposeApp(app);

	if (msg)
	{
		PutStr(msg);
		exit(20);
	}
  exit(0);
}

BOOL init_app(int argc, char **argv)
{
	MUIMasterBase = OpenLibrary((CONST_STRPTR)MUIMASTER_LIBRARY, 19);
	if (MUIMasterBase == NULL)
	{
		clean_exit((unsigned char*)"Can't open muimaster.library v19\n");
		return FALSE;
	}

	#if defined(__amigaos4__)
	if (!(IMUIMaster = (struct MUIMasterIFace *)GetInterface(MUIMasterBase, "main", 1, NULL))) {
		clean_exit((unsigned char*)"Failed to open "MUIMASTER_NAME".");
		return FALSE;
	}
	#endif

	TextEditorMCC = OpenLibrary((CONST_STRPTR)"mui/TextEditor.mcc", 15);
	if (TextEditorMCC == NULL)
	{
		clean_exit((unsigned char*)"Can't open TextEditor.mcc v15 or greater\n");
		return FALSE;
	}
	CloseLibrary(TextEditorMCC);

	LowLevelBase = OpenLibrary((CONST_STRPTR)"lowlevel.library", 0);
	if (LowLevelBase == NULL)
	{
		clean_exit((unsigned char*)"Can't open lowlevel.library\n");
		return FALSE;
	}

	executable_name = get_executable_name(argc, argv);
	iGameSettings = load_settings(DEFAULT_SETTINGS_FILE);

	if (!iGameSettings->no_guigfx && !iGameSettings->hide_screenshots)
	{
		RenderLibBase = OpenLibrary((CONST_STRPTR)"render.library", 30);
		if (RenderLibBase == NULL)
		{
			clean_exit((unsigned char*)"Can't open render.library v30 or greater\n");
			return FALSE;
		}
		CloseLibrary(RenderLibBase);

		GuiGfxBase = OpenLibrary((CONST_STRPTR)"guigfx.library", 17);
		if (GuiGfxBase == NULL)
		{
			clean_exit((unsigned char*)"Can't open guigfx.library v17 or greater\n");
			return FALSE;
		}
		CloseLibrary(GuiGfxBase);

		GuiGfxMCC = OpenLibrary((CONST_STRPTR)"mui/Guigfx.mcc", 19);
		if (GuiGfxMCC == NULL)
		{
			clean_exit((unsigned char*)"Can't open Guigfx.mcc v19 or greater\n");
			return FALSE;
		}
		CloseLibrary(GuiGfxMCC);
	}

	app = CreateApp();
	if (!app)
		clean_exit((unsigned char*)"Can't initialize application\n");
	else
		app_start();

	return TRUE;
}

int main(int argc, char **argv)
{
	init_app(argc, argv);
	ULONG sigs = 0;
	ULONG old = 0;
	const int unit = 1;

	if (app)
	{
		while (DoMethod(app->App, MUIM_Application_NewInput, &sigs)
			!= (ULONG)MUIV_Application_ReturnID_Quit)
		{
			if (sigs)
			{
				sigs = Wait(sigs | SIGBREAKF_CTRL_C);
				if (sigs & SIGBREAKF_CTRL_C)
					break;
			}

			// TODO: This doesn't work on AmigaOS 4. Needs to be updated with compatible code
			#if !defined(__amigaos4__)
			if (LowLevelBase)
			{
				const ULONG new = ReadJoyPort(unit);
				if (new != old)
				{
					old = new;
					joystick_input(new);
				}

				Delay(1);
			}
			#endif
		}
		clean_exit(NULL);
	}
	else
		clean_exit((unsigned char*)"Can't create application\n");

	return 0;
}

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <psapi.h>
#include <stdlib.h>
#include <string.h>
#include "hexchat-plugin.h"


static hexchat_plugin *ph;   /* plugin handle */
static char name[] = "Spotify Now Playing";
static char desc[] = "Sends currently playing song in Spotify to the current channel.";
static char version[] = "1.1";
static const char helpmsg[] = "Sends currently playing song in Spotify to the current channel. USAGE: /spotify";

static char tempbuf[1024];
int foundsomething = 0;

int GetProcName(int procid, char* temp, int bufsiz)
{
	int returnval = -1;
    // Get a handle to the process.
    HANDLE hProcess = OpenProcess(PROCESS_QUERY_INFORMATION |
                                   PROCESS_VM_READ,
                                   FALSE, procid);

    // Get the process name.
    if (NULL != hProcess)
    {
        HMODULE hMod;
        DWORD cbNeeded;

        if (EnumProcessModules(hProcess, &hMod, sizeof(hMod), &cbNeeded))
        {
            GetModuleBaseName(hProcess, hMod, temp, bufsiz);
			return 0;
        }
    }
    CloseHandle( hProcess );
	
	return returnval;
}

static BOOL EnumWindCallback(HWND hwnd, LPARAM lParam)
{
	int phandle;
	unsigned long procid = GetWindowThreadProcessId(hwnd,&phandle);
	
	RealGetWindowClass (hwnd,&tempbuf,1024);
	
	if(strncmp(tempbuf,"Chrome_WidgetWin",16))
		return TRUE;

	GetProcName(phandle, tempbuf, 1024);
	
	if(strcmp(tempbuf,"Spotify.exe"))
		return TRUE;
	
	wchar_t window_text[1024];
	if (GetWindowTextW (hwnd, window_text, 1024))
	{
		if (wcscmp (window_text, L"Spotify") == 0)
		{
			foundsomething = 0;
			return FALSE;
		}

		// UTF-16 to UTF-8
		if (!WideCharToMultiByte (CP_UTF8, 0, window_text, -1, &tempbuf, sizeof(tempbuf), NULL, NULL))
		{
			foundsomething = -1;
			return FALSE;
		}
		
		foundsomething = 1;
		
		return FALSE;
	}
	return TRUE;
}


static int spotify_cb(char *word[], char *word_eol[], void *userdata)
{
	foundsomething = -2;

	EnumWindows(EnumWindCallback,0);
	
	if(foundsomething == 1)
	{
		hexchat_commandf (ph, "me is now listening to: %s", tempbuf);
		return HEXCHAT_EAT_ALL;
	}
	else if(foundsomething == 0)
	{
		hexchat_print (ph, "Spotify is not playing anything right now.");
		return HEXCHAT_EAT_ALL;
	}
	else if(foundsomething == -1)
	{
		hexchat_print (ph, "Some error occured.");
		return HEXCHAT_EAT_ALL;
	}
	else if(foundsomething == -2)
	{
		hexchat_print (ph, "Spotify not running or failed to detect.");
		return HEXCHAT_EAT_ALL;
	}
	return HEXCHAT_EAT_ALL;
}

int hexchat_plugin_init(hexchat_plugin *plugin_handle,
                      char **plugin_name,
                      char **plugin_desc,
                      char **plugin_version,
                      char *arg)
{
	/* we need to save this for use with any hexchat_* functions */
	ph = plugin_handle;

	/* tell HexChat our info */
	*plugin_name = name;
	*plugin_desc = desc;
	*plugin_version = version;

	hexchat_hook_command (ph, "SPOTIFY", HEXCHAT_PRI_NORM, spotify_cb, helpmsg, 0);

	hexchat_printf (ph, "%s plugin loaded\n", name);

	return 1;	/* return 1 for success */
}

int hexchat_plugin_deinit(hexchat_plugin *plugin_handle)
{
	hexchat_printf (plugin_handle, "%s plugin unloaded\n", name);
	return 1;
}

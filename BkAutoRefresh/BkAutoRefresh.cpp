// BkAutoRefresh.cpp : DLL アプリケーション用にエクスポートされる関数を定義します。
//

#include "stdafx.h"
#include "BkAutoRefresh.h"
#include "BeckyAPI.h"
#include "BkCommon.h"
#include "bregexp\BREGEXP.H"

#pragma comment(lib, "bregexp\\BREGEXP.LIB")


CBeckyAPI bka; // You can have only one instance in a project.

HINSTANCE g_hInstance = NULL;
HHOOK g_mhhk = NULL;

BOOL g_enableRefresh = TRUE;
BOOL g_enableQuote = TRUE;

char szIni[_MAX_PATH+2]; // Ini file to save your plugin settings.


////////////////////////////////////////////////////////////////////////////
// Original functions
int strcount(const char* str, const char c) {
	int count = 0;
	for (;*str != '\0'; str++) {
		if (*str == c) count++;
	}
	return count;
}

bool ends_with(const char* src, const char* val) {
	int srcLen, valLen;

	srcLen = strlen(src);
	valLen = strlen(val);

#ifdef _DEBUG
	char buf[256];
	sprintf_s(buf, sizeof(buf), "src: \"%s\"\nsrcLen: %d\nval: \"%s\"\nvalLen: %d\n", src, srcLen, val, valLen);
	MessageBox(NULL, buf, "BkAutoRefresh", MB_OK);
#endif

	return ((srcLen >= valLen) && (strncmp(src + srcLen - valLen, val, valLen) == 0));
}

////////////////////////////////////////////////////////////////////////////
// Dialog procs
BOOL CALLBACK SetupProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	switch (msg){
	case WM_COMMAND:
		if (LOWORD(wParam) == IDOK) {
			g_enableRefresh = ((SendDlgItemMessage(hWnd, IDC_CHK_REFRESH, BM_GETCHECK, 0, 0) == BST_CHECKED)? TRUE:FALSE);
			g_enableQuote = ((SendDlgItemMessage(hWnd, IDC_CHK_QUOTE, BM_GETCHECK, 0, 0) == BST_CHECKED)? TRUE:FALSE);

			char szVal[60];
			sprintf(szVal, "%d", g_enableRefresh);
			WritePrivateProfileString("Settings", "EnableRefresh", szVal, szIni);
			sprintf(szVal, "%d", g_enableQuote);
			WritePrivateProfileString("Settings", "EnableQuote", szVal, szIni);
			EndDialog(hWnd, IDOK);
			return TRUE;
		} else
		if (LOWORD(wParam) == IDCANCEL) {
			EndDialog(hWnd, IDCANCEL);
			return TRUE;
		}
		break;

	case WM_INITDIALOG:
		SendDlgItemMessage(hWnd, IDC_CHK_REFRESH, BM_SETCHECK, g_enableRefresh, 0);
		SendDlgItemMessage(hWnd, IDC_CHK_QUOTE, BM_SETCHECK, g_enableQuote, 0);
		return TRUE;

	default:
		break;
	}
	return FALSE;
}

////////////////////////////////////////////////////////////////////////////
// Hook procs
LRESULT CALLBACK LDoubleClickHookProc(int nCode, WPARAM wParam, LPARAM lParam)
{
	MOUSEHOOKSTRUCT *pmh;
	HGLOBAL hg;
	PTSTR strText = NULL, strClip = NULL, strSel = NULL;

	pmh = (MOUSEHOOKSTRUCT *)lParam;

	if (nCode == HC_ACTION && wParam == WM_LBUTTONDBLCLK) {
		char buf[256];
		GetClassName(pmh->hwnd, buf, sizeof(buf));
		if (strcmp(buf, "DanaEditWindowClass") == 0) {
			// backup clipboard
			if (IsClipboardFormatAvailable(CF_TEXT)) {
				if (OpenClipboard(NULL) && (hg = GetClipboardData(CF_TEXT))) {
					strText = (PTSTR)bka.Alloc(GlobalSize(hg));
					strClip = (PTSTR)GlobalLock(hg);
					lstrcpy(strText, strClip);

					GlobalUnlock(hg);
					EmptyClipboard();
					CloseClipboard();
				}
			}

			PostMessage(pmh->hwnd, WM_COMMAND, 57634, NULL); //0x0000E122
			if (IsClipboardFormatAvailable(CF_TEXT)) {
				if (OpenClipboard(NULL) && (hg = GetClipboardData(CF_TEXT))) {
					strSel = (PTSTR)bka.Alloc(GlobalSize(hg));
					strClip = (PTSTR)GlobalLock(hg);
					lstrcpy(strSel, strClip);
					GlobalUnlock(hg);
					CloseClipboard();

					MessageBox(pmh->hwnd, strSel, "BkAutoRefresh", MB_OK);
					bka.Free(strSel);
				}
			}

			// restore clipboard
			if (strText != NULL) {
				if (OpenClipboard(NULL)) {
					EmptyClipboard();

					hg = GlobalAlloc(GHND | GMEM_SHARE, strlen(strText));
					strClip = (PTSTR)GlobalLock(hg);
					lstrcpy(strClip, strText);
					GlobalUnlock(hg);

					SetClipboardData(CF_TEXT, hg);
					CloseClipboard();
				}
				bka.Free(strText);
			}
		}
	}

	return CallNextHookEx(g_mhhk, nCode, wParam, lParam);
}
void EnableHook()
{
	if (g_mhhk != NULL) return;

	g_mhhk = SetWindowsHookEx(WH_MOUSE, (HOOKPROC)LDoubleClickHookProc, g_hInstance, NULL);
	if (g_mhhk == NULL) {
		MessageBox(NULL, "Hooking mouse message failed!", "BkAutoRefresh", MB_OK | MB_ICONERROR);
	}
}
void DisableHook()
{
	if (g_mhhk != NULL) {
		UnhookWindowsHookEx(g_mhhk);
	}
}

/////////////////////////////////////////////////////////////////////////////
// DLL entry point
BOOL APIENTRY DllMain( HANDLE hModule, 
					   DWORD  ul_reason_for_call, 
					   LPVOID lpReserved
					 )
{
	g_hInstance = (HINSTANCE)hModule;
	switch (ul_reason_for_call)
	{
		case DLL_PROCESS_ATTACH:
			{
				if (!bka.InitAPI()) {
					return FALSE;
				}
				GetModuleFileName((HINSTANCE)hModule, szIni, _MAX_PATH);
				LPSTR lpExt = strrchr(szIni, '.');
				if (lpExt) {
					strcpy(lpExt, ".ini");
				} else {
					// just in case
					strcat(szIni, ".ini");
				}
				g_enableRefresh = GetPrivateProfileInt("Settings", "EnableRefresh", TRUE, szIni);
				g_enableQuote = GetPrivateProfileInt("Settings", "EnableQuote", TRUE, szIni);
			}
			break;
		case DLL_THREAD_ATTACH:
			break;
		case DLL_THREAD_DETACH:
			break;
		case DLL_PROCESS_DETACH:
			break;
	}
	return TRUE;
}

/////////////////////////////////////////////////////////////////////////////////////////////
// Callbacks from Becky!

#ifdef __cplusplus
extern "C"{
#endif

////////////////////////////////////////////////////////////////////////
// Called when the program is started and the main window is created.
int WINAPI BKC_OnStart()
{
	/*
	Since BKC_OnStart is called after Becky!'s main window is
	created, at least BKC_OnMenuInit with BKC_MENU_MAIN is called
	before BKC_OnStart. So, do not assume BKC_OnStart is called
	prior to any other callback.
	*/

	EnableHook();

	// Always return 0.
	return 0;
}

////////////////////////////////////////////////////////////////////////
// Called when the main window is closing.
int WINAPI BKC_OnExit()
{
	DisableHook();
	// Return -1 if you don't want to quit.
	return 0;
}

////////////////////////////////////////////////////////////////////////
// Called when menu is intialized.
int WINAPI BKC_OnMenuInit(HWND hWnd, HMENU hMenu, int nType)
{
	switch (nType) {
	case BKC_MENU_MAIN:
		{
			/* Sample of adding menu items
			HMENU hSubMenu = GetSubMenu(hMenu, 4);
			// Define CmdProc as "void WINAPI CmdProc(HWND, LPARAM)"
			UINT nID = bka.RegisterCommand("Information about this Command", nType,CmdProc);
			AppendMenu(hSubMenu, MF_STRING, nID, "&Menu item");
			*/
			/* If needed, you can register the command UI update callback.
			// Define CmdUIProc as "UINT WINAPI CmdUIProc(HWND, LPARAM)"
			bka.RegisterUICallback(nID, CmdUIProc);
			*/
		}
		break;
	case BKC_MENU_LISTVIEW:
		break;
	case BKC_MENU_TREEVIEW:
		break;
	case BKC_MENU_MSGVIEW:
		break;
	case BKC_MENU_MSGEDIT:
		break;
	case BKC_MENU_TASKTRAY:
		break;
	case BKC_MENU_COMPOSE:
		break;
	case BKC_MENU_COMPEDIT:
		break;
	case BKC_MENU_COMPREF:
		break;
	default:
		break;
	}
	// Always return 0.
	return 0;
}

////////////////////////////////////////////////////////////////////////
// Called when a folder is opened.
int WINAPI BKC_OnOpenFolder(LPCTSTR lpFolderID)
{
	HWND hwnd[4] = {};

	if (!g_enableRefresh) return 0;

	if (strcount(lpFolderID, '\\') == 1) {
		bka.GetWindowHandles(&hwnd[0], &hwnd[1], &hwnd[2], &hwnd[3]);
		//SendNotifyMessage(hwnd, WM_KEYDOWN, VK_F5, 1);
		PostMessage(hwnd[0], WM_COMMAND, BK_COMMAND_RELOAD_QUERY, NULL);
	}
	// Always return 0.
	return 0;
}

////////////////////////////////////////////////////////////////////////
// Called when a mail is selected.
int WINAPI BKC_OnOpenMail(LPCTSTR lpMailID)
{
	// Always return 0.
	return 0;
}

////////////////////////////////////////////////////////////////////////
// Called every minute.
int WINAPI BKC_OnEveryMinute()
{
	// Always return 0.
	return 0;
}

////////////////////////////////////////////////////////////////////////
// Called when a compose windows is opened.
int WINAPI BKC_OnOpenCompose(HWND hWnd, int nMode/* See COMPOSE_MODE_* in BeckyApi.h */)
{
	LPSTR text, newTxt;
	char mimeType[80], *ctx, *line;
	int textLen, ntextLen;
	bool snipFlag = false;
	const char *newLine = "\r\n";
	const char *delim = "\r";

	if (!g_enableQuote) return 0;

	if ((nMode == COMPOSE_MODE_REPLY1) ||
		(nMode == COMPOSE_MODE_REPLY2) ||
		(nMode == COMPOSE_MODE_REPLY3)) {
			text = bka.CompGetText(hWnd, mimeType, sizeof(mimeType));
#ifdef _DEBUG
			MessageBox(hWnd, text, "BkAutoRefresh", MB_OK);
#endif
			if (strcmp(mimeType, "text/plain") != 0) return 0;

			textLen = strlen(text);
			ntextLen = textLen + (textLen / 2);
			newTxt = (LPSTR)bka.Alloc(sizeof(char) * ntextLen);
			memset(newTxt, '\0', ntextLen);

			line = strtok_s(text, delim, &ctx);
			strcat_s(newTxt, ntextLen, newLine);
			while (line) {
				if (*line == '\n') line++;

				if ((strlen(line) > 0) && (line[0] == RFC2646_QUOTE_INDICATOR)) {
					if (ends_with(line, RFC2646_SIGNATURE_SEPARATOR)) {
						snipFlag = true;
					}
				} else if (snipFlag) {
					snipFlag = false;
				}

				if (!snipFlag) {
					strcat_s(newTxt, ntextLen, line);
					strcat_s(newTxt, ntextLen, newLine);
				}

				line = strtok_s(NULL, delim, &ctx);
			}

#ifdef _DEBUG
			MessageBox(hWnd, newTxt, "BkAutoRefresh", MB_OK);
#endif
			bka.CompSetText(hWnd, 0, newTxt);

			bka.Free(text);
			bka.Free(newTxt);
	}

	// Always return 0.
	return 0;
}

////////////////////////////////////////////////////////////////////////
// Called when the composing message is saved.
int WINAPI BKC_OnOutgoing(HWND hWnd, int nMode/* 0:SaveToOutbox, 1:SaveToDraft, 2:SaveToReminder*/) 
{
	// Return -1 if you do not want to send it yet.
	return 0;
}

////////////////////////////////////////////////////////////////////////
// Called when a key is pressed.
int WINAPI BKC_OnKeyDispatch(HWND hWnd, int nKey/* virtual key code */, int nShift/* Shift state. 0x40=Shift, 0x20=Ctrl, 0x60=Shift+Ctrl, 0xfe=Alt*/)
{
	// Return TRUE if you want to suppress subsequent command associated to this key.
	return 0;
}

////////////////////////////////////////////////////////////////////////
// Called when a message is retrieved and saved to a folder
int WINAPI BKC_OnRetrieve(LPCTSTR lpMessage/* Message source*/, LPCTSTR lpMailID/* Mail ID*/)
{
	// Always return 0.
	return 0;
}

////////////////////////////////////////////////////////////////////////
// Called when a message is spooled
int WINAPI BKC_OnSend(LPCTSTR lpMessage/* Message source */)
{
	// Return BKC_ONSEND_PROCESSED, if you have processed this message
	// and don't need Becky! to send it.
	// Becky! will move this message to Sent box when the sending
	// operation is done.
	// CAUTION: You are responsible for the destination of this
	// message if you return BKC_ONSEND_PROCESSED.

	// Return BKC_ONSEND_ERROR, if you want to cancel the sending operation.
	// You are responsible for displaying an error message.

	// Return 0 to proceed the sending operation.
	return 0;
}

////////////////////////////////////////////////////////////////////////
// Called when all messages are retrieved
int WINAPI BKC_OnFinishRetrieve(int nNumber/* Number of messages*/)
{
	LPCSTR curFolder;
	HWND hwnd[4] = {};

	if (nNumber == 0 || !g_enableRefresh) return 0;
	curFolder = bka.GetCurrentFolder();
	if (strcount(curFolder, '\\') == 1) {
		bka.GetWindowHandles(&hwnd[0], &hwnd[1], &hwnd[2], &hwnd[3]);
		//SendNotifyMessage(hwnd, WM_KEYDOWN, VK_F5, 1);
		PostMessage(hwnd[0], WM_COMMAND, BK_COMMAND_RELOAD_QUERY, NULL);
	}
	// Always return 0.
	return 0;
}


////////////////////////////////////////////////////////////////////////
// Called when plug-in setup is needed.
int WINAPI BKC_OnPlugInSetup(HWND hWnd)
{
	int nRC = DialogBox(g_hInstance, MAKEINTRESOURCE(IDD_SETTINGS), hWnd, (DLGPROC)SetupProc);
	// Return nonzero if you have processed.
	// return 1;
	return 1;
}


////////////////////////////////////////////////////////////////////////
// Called when plug-in information is being retrieved.
typedef struct tagBKPLUGININFO
{
	char szPlugInName[80]; // Name of the plug-in
	char szVendor[80]; // Name of the vendor
	char szVersion[80]; // Version string
	char szDescription[256]; // Short description about this plugin
} BKPLUGININFO, *LPBKPLUGININFO;

int WINAPI BKC_OnPlugInInfo(LPBKPLUGININFO lpPlugInInfo)
{
	/* You MUST specify at least szPlugInName and szVendor.
	   otherwise Becky! will silently ignore your plug-in. */
	char buf[255];

	LoadString(g_hInstance, IDS_BAR_NAME, buf, sizeof(buf));
	strcpy(lpPlugInInfo->szPlugInName, buf);
	LoadString(g_hInstance, IDS_BAR_VENDOR, buf, sizeof(buf));
	strcpy(lpPlugInInfo->szVendor, buf);
	strcpy(lpPlugInInfo->szVersion, "2.0");
	LoadString(g_hInstance, IDS_BAR_DESC, buf, sizeof(buf));
	strcpy(lpPlugInInfo->szDescription, buf);
	
	// Always return 0.
	return 0;
}

////////////////////////////////////////////////////////////////////////
// Called when drag and drop operation occurs.
int WINAPI BKC_OnDragDrop(LPCSTR lpTgt, LPCSTR lpSrc, int nCount, int dropEffect)
{
	/*
	lpTgt:	A folder ID of the target folder.
			You can assume it is a root mailbox, if the string
			contains only one '\' character.
	lpSrc:	Either a folder ID or mail IDs. Multiple mail IDs are
			separated by '\n' (0x0a).
			You can assume it is a folder ID, if the string
			doesn't contain '?' character.
	nCount:	Number of items to be dropped.
			It can be more than one, if you drop mail items.
	dropEffect: Type of drag and drop operation
			1: Copy
			2: Move
			4: Link (Used for filtering setup in Becky!)
	*/
	// If you want to cancel the default drag and drop action,
	// return -1;
	// Do not assume the default action (copy, move, etc.) is always
	// processed, because other plug-ins might cancel the operation.
	return 0;
}


////////////////////////////////////////////////////////////////////////
// Called when a message was retrieved and about to be filtered.
int WINAPI BKC_OnBeforeFilter2(LPCSTR lpMessage, LPCSTR lpMailBox, int* lpnAction, char** lppParam)
{
	/*
    lpMessage: A complete message source, which ends with
    "<CRLF>.<CRLF>".
    lpnAction:	[out] Returns the filtering action to be applied.
    	#define ACTION_NOTHING		-1	// Do nothing
		#define ACTION_MOVEFOLDER	0	// Move to a folder
		#define ACTION_COLORLABEL	1	// Set the color label
		#define ACTION_SETFLAG		2	// Set the flag
		#define ACTION_SOUND		3	// Make a sound
		#define ACTION_RUNEXE		4	// Run executable file
		#define ACTION_REPLY		5	// Reply to the message
		#define ACTION_FORWARD		6	// Forward the message
		#define ACTION_LEAVESERVER	7	// Leave/delete on the server.
		#define ACTION_ADDHEADER	8	// Add "X-" header to the top of the message. (Plug-in only feature)
	lpMailBox: ID of the mailbox that is retrieving the message. (XXXXXXXX.mb\)
	lppParam:	[out] Returns the pointer to the filtering parameter string.
		ACTION_MOVEFOLDER:	Folder name.
							e.g. XXXXXXXX.mb\FolderName\
							or \FolderName\ (begin with '\') to use
							the mailbox the message belongs.
		ACTION_COLORLABEL:	Color code(BGR) in hexadecimal. e.g. 0088FF
		ACTION_SETFLAG:		"F" to set flag. "R" to set read.
		ACTION_SOUND:		Name of the sound file.
		ACTION_RUNEXE:		Command line to execute. %1 will be replaced with the path to the file that contains the entire message.
		ACTION_REPLY:		Path to the template file without extension.
								e.g. #Reply\MyReply
		ACTION_FORWARD:		Path to the template file without extension. + "*" + Address to forward.
								e.g. #Forward\MyForward*mail@address
									 *mail@address (no template)
		ACTION_LEAVESERVER:	The string consists of one or two decimals. The second decimal is optional.
							Two decimals must be separated with a space.
							First decimal	1: Delete the message from the server.
											0: Leave the message on the server.
							Second decimal	1: Do not store the message to the folder.
											0: Store the message to the folder. (default action)
							e.g. 0 (Leave the message on the server.)
								 1 1 (Delete the message on the server and do not save. (Means KILL))
		ACTION_ADDHEADER	"X-Header:data" that will be added at the top of the incoming message.
							You can specify multiple headers by separating CRLF, but each header must
							begin with "X-". e.g. "X-Plugindata1: test\r\nX-Plugindata2: test2";
	*/
	
	/* Return values
	BKC_FILTER_DEFAULT	Do nothing and apply default filtering rules.
	BKC_FILTER_PASS		Apply default filtering rules after applying the rule it returns.
	BKC_FILTER_DONE		Do not apply default rules.
	BKC_FILTER_NEXT		Request Becky! to call this callback again so that another rules can be added.
	*/
    return BKC_FILTER_DEFAULT;
}

#ifdef __cplusplus
}
#endif

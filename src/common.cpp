#include "stdafx.h"
#include "../res/resource.h"

static char* __THIS_FILE__ = __FILE__;

Common::CComConfig* comcfg;

void com_load_config(void)
{
	char mp[MAX_PATH]={0};
	GetModuleFileName(NULL, mp, __ARRAY_SIZE(mp));
	strcpy(strrchr(mp, '\\')+1, "common.ini");
	comcfg = new Common::CComConfig;
	comcfg->LoadFile(mp);
}

void com_unload_config(void)
{
	comcfg->SaveFile();
	delete comcfg;
}

Common::c_the_app theApp;

int CALLBACK WinMain(HINSTANCE hInstance,HINSTANCE hPrevInstance,LPSTR lpCmdLine,int nShowCmd)
{
	//InitCommonControls();
	LoadLibrary("RichEd20.dll");

#ifdef _DEBUG
	AllocConsole();
	freopen("CONIN$", "r", stdin);
	freopen("CONOUT$", "w", stdout);
	freopen("CONOUT$", "w", stderr);
#endif
	debug_out(("����������\n"));

	com_load_config();

	Common::CComWnd maindlg;
	maindlg.Create(nullptr, MAKEINTRESOURCE(IDD_DLG_MAIN));
	maindlg.CenterWindow();
	maindlg.ShowWindow();

	Common::CWindowManager::MessageLoop();
	
	com_unload_config();

	debug_out(("�����ѽ���\n"));
#ifdef _DEBUG
	Sleep(500);
	FreeConsole();
#endif
	MessageBeep(MB_OK);
	return 0;
}


#include<windows.h>
#include "config.h"
#include <CommCtrl.h>
#include<TlHelp32.h>
#pragma comment(lib,"comctl32.lib")

void AdjustPrivilege() //提权函数
{
	HANDLE hToken;
	if (OpenProcessToken(GetCurrentProcess(), TOKEN_ADJUST_PRIVILEGES, &hToken))
	{
		TOKEN_PRIVILEGES tp;
		tp.PrivilegeCount = 1;
		tp.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;
		if (LookupPrivilegeValue(NULL, SE_DEBUG_NAME, &tp.Privileges[0].Luid))
		{
			AdjustTokenPrivileges(hToken, FALSE, &tp, sizeof(tp), NULL, NULL);
		}
		CloseHandle(hToken);
	}
}

BOOL FreeMyDll(HMODULE hMod, DWORD pid)
{
	HANDLE hPro = OpenProcess(PROCESS_ALL_ACCESS, FALSE, pid);
	if (hPro)
	{
		PVOID pLoad = GetProcAddress(GetModuleHandle("Kernel32.dll"), "FreeLibrary");
		if (pLoad)
		{
			HANDLE hRemo = CreateRemoteThread(hPro, NULL, 0, (LPTHREAD_START_ROUTINE)pLoad, hMod, 0, NULL);
			if (hRemo)
			{
				CloseHandle(hRemo);
				return TRUE;
			}

		}
	}

	return FALSE;
}

void RemoteFree(DWORD pid)
{
	HANDLE hMou = CreateToolhelp32Snapshot(TH32CS_SNAPMODULE, pid);
	if (hMou)
	{
		HMODULE hFreeMod = NULL;
		MODULEENTRY32 mod32 = { 0 };
		mod32.dwSize = sizeof(MODULEENTRY32);
		BOOL bRet = Module32First(hMou, &mod32);

		while (bRet)
		{
			if (_strcmpi(mod32.szModule, "afdhook.dll") == 0)
			{
				hFreeMod = (HMODULE)mod32.modBaseAddr;
				FreeMyDll(hFreeMod, pid);
				break;
			}
			bRet = Module32Next(hMou, &mod32);
			
		}
		CloseHandle(hMou);
	}
}


int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, PSTR szCmdLine, int iCmdShow)
{
	static TCHAR szAppName[] = TEXT("StandModeforProgram"); //主窗口类名
	HWND hwnd;
	MSG msg;
	WNDCLASS wndclass;

	

	wndclass.style = CS_HREDRAW | CS_VREDRAW;
	wndclass.lpfnWndProc = WndProc;//窗口过程
	wndclass.cbClsExtra = 0;
	wndclass.cbWndExtra = 0;
	wndclass.hInstance = hInstance;
	wndclass.hIcon = LoadIcon(NULL, IDI_APPLICATION);//图标
	wndclass.hCursor = LoadCursor(NULL, IDC_ARROW);//鼠标指针
	wndclass.hbrBackground = (HBRUSH)GetStockObject(WHITE_BRUSH);//背景
	wndclass.lpszMenuName = NULL;//菜单
	wndclass.lpszClassName = szAppName;

	if (!RegisterClass(&wndclass))//注册窗口类
	{
		MessageBox(NULL, TEXT("Program requires Windows NT!"), szAppName, MB_ICONERROR);
		return 0;
	}
	hwnd = CreateWindow(
		szAppName,//窗口类名称
		TEXT("我爱抓抓抓"),//窗口标题
		WS_OVERLAPPEDWINDOW&~WS_MAXIMIZEBOX&~WS_THICKFRAME,//窗口风格
		CW_USEDEFAULT, //初始x坐标
		CW_USEDEFAULT, //初始y坐标
		WINDOW_WIDTH, //x
		WINDOW_HIGHT, //y
		NULL, //父窗口句柄
		NULL, //窗口菜单句柄
		hInstance, //程序实例句柄 
		NULL //创建参数
	);

	//更新窗口
	ShowWindow(hwnd, iCmdShow);
	UpdateWindow(hwnd);

	//消息循环
	while (GetMessage(&msg, NULL, 0, 0))
	{
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}
	return msg.wParam;
}



char leixing[4] = {0};

DWORD chosepid=0;

LRESULT CALLBACK WndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{

	//
	static char *buff = NULL;
	
	static char DllName[MAX_PATH] = {0};
	GetCurrentDirectory(MAX_PATH, DllName);
	strcat_s(DllName, "\\afdhook.dll");
	static int n_index = 0;
	
	int cbSize;

	//Edit
	static HWND hwndEdit;

	//button 
	static HANDLE hwndButton, hwndButton2,hwndButton3;
	static int cxchar, cychar, xclient, yclient;

	
	//listview
	static	HWND listview1, listview2;
	static	LVCOLUMN list1, list2;
	static  LVITEM  item2;

	//process inject
	static int chose = 0; //所选择的项数
	DWORD TargetProcessId; 
	HANDLE hProcess;
	LPVOID lpRemoteDllName;
	HMODULE hModule;
	HANDLE hRemoteThread;
	LPTHREAD_START_ROUTINE pfnStartRoutine;
	
	//DATA 
	COPYDATASTRUCT* cs=NULL;


	switch (uMsg) 
	{
	case WM_CREATE:

		//创建编辑框
		hwndEdit = CreateWindow(TEXT("edit"), NULL, WS_CHILD | WS_VISIBLE | WS_VSCROLL | WS_HSCROLL | WS_BORDER | ES_LEFT | ES_MULTILINE | ES_AUTOHSCROLL | ES_AUTOVSCROLL, 320, 620, 1200,100, hwnd, (HMENU)ID_EDIT, ((LPCREATESTRUCT)lParam)->hInstance, NULL);
		
		//创建按钮
		cxchar = LOWORD(GetDialogBaseUnits());
		cychar = HIWORD(GetDialogBaseUnits());

		hwndButton = CreateWindow(TEXT("button"), "注入", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON, 100 , 620, 10 * cxchar, 20 * cychar / 8, hwnd, (HMENU)ID_BUTTON_INJ, ((LPCREATESTRUCT)lParam)->hInstance, NULL);
	
		hwndButton2= CreateWindow(TEXT("button"), "卸载", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON, 200, 620, 10 * cxchar, 20 * cychar / 8, hwnd, (HMENU)ID_BUTTON_UHOOK, ((LPCREATESTRUCT)lParam)->hInstance, NULL);

		hwndButton3 = CreateWindow(TEXT("button"), "刷新", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON, 10, 620, 10 * cxchar, 20 * cychar / 8, hwnd, (HMENU)ID_BUTTON_FLUSH, ((LPCREATESTRUCT)lParam)->hInstance, NULL);
		//创建listview  
		listview1 = CreateWindowEx(WS_EX_STATICEDGE, TEXT("SysListView32"), NULL, WS_CHILD | WS_VISIBLE | LVS_REPORT/*报表列表*/ | LVS_SINGLESEL/*同时只能选一项*/, 10, 10, 300, 600, hwnd, (HMENU)ID_LIST_PROCESS, (HINSTANCE)GetWindowLong(hwnd, GWL_HINSTANCE), NULL);
		ListView_SetExtendedListViewStyle(listview1, LVS_EX_FULLROWSELECT/*整行选中*/ | LVS_EX_GRIDLINES/*显示网格线*/);//设置listview扩展风格  
		SendMessage(listview1, WM_SETFONT, (WPARAM)GetStockObject(17), 0);//获取系统字体

		listview2 = CreateWindowEx(WS_EX_STATICEDGE, TEXT("SysListView32"), NULL, WS_CHILD | WS_VISIBLE | LVS_REPORT/*报表列表*/ | LVS_SINGLESEL/*同时只能选一项*/, 320, 10, 1200, 600, hwnd, (HMENU)ID_LIST_DETAIL, (HINSTANCE)GetWindowLong(hwnd, GWL_HINSTANCE), NULL);
		ListView_SetExtendedListViewStyle(listview2, LVS_EX_FULLROWSELECT/*整行选中*/ | LVS_EX_GRIDLINES/*显示网格线*/);//设置listview扩展风格  
		SendMessage(listview2, WM_SETFONT, (WPARAM)GetStockObject(17), 0);//获取系统字体

		//内存清零  
		RtlZeroMemory(&list1, sizeof(LVCOLUMN));
		

		RtlZeroMemory(&list2, sizeof(LVCOLUMN));
		RtlZeroMemory(&item2, sizeof(LVITEM));

		//listview1  创建列  
		list1.mask = LVCF_FMT | LVCF_WIDTH | LVCF_TEXT;//掩码  
		list1.fmt = LVCFMT_LEFT;//左对齐  

		list1.cx = 100;//列宽  
		list1.pszText = TEXT("进程ID");
		SendMessage(listview1, LVM_INSERTCOLUMN, 0, (LPARAM)&list1);//创建列  

		list1.pszText = TEXT("进程名");
		list1.cx = 250;
		SendMessage(listview1, LVM_INSERTCOLUMN, 1, (LPARAM)&list1);
		
		//listview2 创建列
		list2.mask = LVCF_FMT | LVCF_WIDTH | LVCF_TEXT| LVCF_SUBITEM;//掩码  
		list2.fmt =  LVCFMT_LEFT;//左对齐  

		list2.cchTextMax = 2048;
		list2.cx = 50;//列宽  
		list2.pszText = TEXT("类型");
		list2.iSubItem = 0;
		SendMessage(listview2, LVM_INSERTCOLUMN, 0, (LPARAM)&list2);//创建列  

		list2.cx =1200*2 ;//列宽  
		list2.pszText = TEXT("内容");
		list2.iSubItem = 1;
		SendMessage(listview2, LVM_INSERTCOLUMN, 1, (LPARAM)&list2);//创建列  

	
		//初始化的时候就插入数据到listview1   
		if (InsertItemIntoListview1(listview1)==FALSE)
		{
			MessageBox(hwnd,"插入失败！或者无process数据！","错误",MB_ICONERROR|MB_OK);
		}
		return 0;
	case WM_NOTIFY:
		switch (wParam) 
		{
			case ID_LIST_DETAIL://ID为listview的ID  
			LPNMITEMACTIVATE now;
			now = (LPNMITEMACTIVATE)lParam;//得到NMITEMACTIVATE结构指针  
			char * tbuff;
			
			switch (now->hdr.code)
			{//判断通知码  
			case NM_CLICK:
				tbuff = (char*)malloc(1026);
				memset(tbuff, 0, 1026);
				ListView_GetItemText(listview2, now->iItem, 1, tbuff, 1026);
				SetWindowText(hwndEdit, tbuff);
				free(tbuff);
				break;
			case NM_RCLICK:
				//右击清空listview2
				SendMessage(listview2, LVM_DELETEALLITEMS, 0, 0);
				n_index = 0;
				break;
			}
			
			break;
		}
		return 0;
	case WM_COPYDATA:

		cs = (COPYDATASTRUCT*)lParam;
		
		buff = (char*)malloc(sizeof(char)*cs->cbData + 2);

		memset(buff, 0, sizeof(char)*cs->cbData+2);

		//strcpy_s(buff, (char*)cs->lpData);
		memcpy(buff, cs->lpData,cs->cbData);
	
		item2.mask = LVIF_TEXT;
		item2.iItem = n_index++;//项目号  
		
		
		memset(leixing, 0, sizeof(leixing));
		switch (wParam)
		{
		case 0:
			leixing[0]='t';
			leixing[1] = 's';
			break;
		case 1:
			leixing[0] = 't';
			leixing[1] = 'r';
			break;
		case 2:
			leixing[0] = 'u';
			leixing[1] = 's';
			break;
		case 3:
			leixing[0] = 'u';
			leixing[1] = 'r';
			break;
		default:
			break;
		}
		
		item2.pszText =TEXT(leixing) ;
	    item2.iSubItem = 0;
		
		ListView_InsertItem(listview2, &item2);
		ListView_SetItemText(listview2, item2.iItem, 1, TEXT(buff));

	
		free(buff);
	
			

		return 0;
	case WM_COMMAND:
		switch (LOWORD(wParam))
		{
		case ID_BUTTON_INJ:
		{
			char s[10];
			chose = ListView_GetSelectionMark(listview1);
			ListView_GetItemText(listview1, chose, 0, s, 10);
			//_itoa_s(chose, s, 10);
			chose = atoi(s);
			TargetProcessId = (DWORD)chose;
			chosepid = TargetProcessId;
			AdjustPrivilege();

			hProcess = OpenProcess(PROCESS_ALL_ACCESS, FALSE, TargetProcessId);
			cbSize = lstrlen(DllName) + 1;
			lpRemoteDllName = VirtualAllocEx(hProcess, NULL, (DWORD)cbSize, MEM_COMMIT, PAGE_READWRITE);
			WriteProcessMemory(hProcess, lpRemoteDllName, DllName, (DWORD)cbSize, NULL);
			hModule = GetModuleHandle("kernel32.dll");
			pfnStartRoutine = (LPTHREAD_START_ROUTINE)GetProcAddress(hModule, "LoadLibraryA");
			hRemoteThread = CreateRemoteThread(hProcess, NULL, 0, pfnStartRoutine, lpRemoteDllName, 0, NULL);
			WaitForSingleObject(hRemoteThread, INFINITE);
			CloseHandle(hRemoteThread);
			CloseHandle(hProcess);
			break;
		}
		case ID_BUTTON_UHOOK:
		{
		if (chosepid != 0)
		{
			RemoteFree(chosepid);
		}
		break;
		}
		case ID_BUTTON_FLUSH:
		{
			SendMessage(listview1, LVM_DELETEALLITEMS, 0, 0);
			InsertItemIntoListview1(listview1);
			break;
		}
		}
		return 0;
	case WM_DESTROY: 
		if (chosepid != 0)
		{
			RemoteFree(chosepid);
		}
		PostQuitMessage(0);
		return 0;
	}
	return DefWindowProc(hwnd, uMsg, wParam, lParam);
}

BOOL InsertItemIntoListview1(HWND listview1)
{
	LVITEM item1;
	
	int k = 0;
	char cspid[20] = {0};
	HANDLE hProcessSnap = NULL;
	PROCESSENTRY32 pe32 = { sizeof(pe32) };
	
	RtlZeroMemory(&item1, sizeof(LVITEM));
	item1.mask = LVIF_TEXT;

	hProcessSnap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);

	if (Process32First(hProcessSnap, &pe32))
	{
		do
		{
			item1.iItem = k++;//项目号  
			memset(cspid, 0, sizeof(cspid));
			_itoa_s(pe32.th32ProcessID, cspid, 10);

			item1.iSubItem = 0;
			item1.pszText = cspid;
			SendMessage(listview1, LVM_INSERTITEM, 0, (LPARAM)&item1);

			//创建子项  
			item1.iSubItem = 1;
			item1.pszText = TEXT(pe32.szExeFile);
			SendMessage(listview1, LVM_SETITEM, 0, (LPARAM)&item1);

		} while (Process32Next(hProcessSnap, &pe32));
		CloseHandle(hProcessSnap);
		return TRUE;
	}

	return FALSE;
}

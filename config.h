#pragma once

#define WINDOW_WIDTH  1600 //窗体长
#define WINDOW_HIGHT  800  //窗体高

#define ID_LIST_PROCESS		1
#define ID_LIST_DETAIL		2
#define ID_BUTTON_INJ		3
#define ID_BUTTON_UHOOK		4
#define ID_BUTTON_FLUSH		5
#define ID_EDIT			    6

LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);
BOOL InsertItemIntoListview1(HWND listview1);
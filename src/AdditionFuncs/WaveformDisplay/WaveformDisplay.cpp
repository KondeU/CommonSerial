/************************************************************

GNU GENERAL PUBLIC LICENSE, Version 3, 29 June 2007
Copyright (c) 2017, KondeU, All rights reserved.

Project:     CommonSerial
File:        WaveformDisplay.cpp
Description: Add functions to original common.
Date:        2017-11-22
Version:     2.01
Authors:     Deyou Kong <370242479@qq.com>
History:     01, 17-11-22, Deyou Kong, Create file and implement it.

************************************************************/

#include "StdAfx.h"
#include "../res/resource.h"
#include "AdditionFuncs/CConfig.h"

#include "WaveformDisplay.h"

#define _DEBUG_FORCE_FLASH false

#define WM_WAVEFORMDISPLAY_RECVDATA (WM_USER + 0x0101)

#define PRECISION 6

#define TRANS_CHAR  0x66
#define TRANS_ALIGN 0x01
#define TRANS_NODET 0x02

#define DATA_BYTE   0
#define DATA_WORD   1
#define DATA_DWORD  2
#define DATA_FLOAT  3
#define DATA_DOUBLE 4

namespace
{
	TCHAR szAppNameEng[] = TEXT("WaveformDisplay");
	TCHAR szAppNameChn[] = TEXT("波形显示");

	Common::CComWnd * pMain = nullptr;

	HWND hwndDraw = NULL;
	HWND hdlgValue = NULL, hdlgSetting = NULL;

	#ifdef _DEBUG
	CConfig cCfg("..\\bin\\WaveformDisp.cfg");
	#else
	CConfig cCfg("WaveformDisp.cfg");
	#endif

	bool bWaveformPause = false;
	bool bMouseSelect   = false;

	double dMousePosX  = 0.0;
	double dMousePosY  = 0.0;
	double dMouseRealX = 0.0;
	double dMouseRealY = 0.0;

	int iWndPixWidth = 640, iWndPixHeight = 480;
	int iDotSize = 1;
	int iSampleDrawGap = 1;
	double dUpVal = 255.0, dDnVal = 0.0;
	string szTypeSelect = TEXT("BYTE");

	const int iTotalType = 5;
	TCHAR szType[iTotalType][16] = // Must be sorted by macro definition
	{
		TEXT("BYTE"),
		TEXT("WORD"),
		TEXT("DWORD"),
		TEXT("FLOAT"),
		TEXT("DOUBLE")
	};

	string szDrawType = TEXT("DOT");

	double * pdData = nullptr;

	bool bInternalUsed = false;
}

#pragma pack(push, 1)
struct TByte
{
	union UByte
	{
		BYTE by;
		BYTE byData[sizeof(BYTE)];
	} u;
};
struct TWord
{
	union UWord
	{
		WORD wd;
		BYTE byData[sizeof(WORD)];
	} u;
};
struct TDword
{
	union UDword
	{
		DWORD dw;
		BYTE byData[sizeof(DWORD)];
	} u;
};
struct TFloat
{
	union UFloat
	{
		FLOAT f;
		BYTE byData[sizeof(FLOAT)];
	} u;
};
struct TDouble
{
	union UDouble
	{
		DOUBLE d;
		BYTE byData[sizeof(DOUBLE)];
	} u;
};
#pragma pack(pop)

// Internal functions
bool RegisterDrawWindow();
void LoadConfig();
void SaveConfig();
int JudgeType(string sz);
void Redraw();
void ProcessNewRecv(BYTE * pbyData, DWORD dwDataBytes);
LRESULT CALLBACK WndProcWaveformDisplay(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);
BOOL CALLBACK DlgProcWaveformValue(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);
BOOL CALLBACK DlgProcWaveformSetting(HWND hdlg, UINT message, WPARAM wParam, LPARAM lParam);

bool RegisterDrawWindow()
{
	WNDCLASS wndclass;
	ZeroMemory(&wndclass, sizeof(wndclass));
	wndclass.style         = 0;
	wndclass.lpfnWndProc   = WndProcWaveformDisplay;
	wndclass.cbClsExtra    = 0;
	wndclass.cbWndExtra    = 0;
	wndclass.hInstance     = GetWindowInstance(pMain->GetHWND());
	wndclass.hIcon         = LoadIcon(GetWindowInstance(pMain->GetHWND()), MAKEINTRESOURCE(IDI_ICON1));
	wndclass.hCursor       = LoadCursor(NULL, IDC_ARROW);
	wndclass.hbrBackground = (HBRUSH)GetStockObject(WHITE_BRUSH);
	wndclass.lpszMenuName  = NULL;
	wndclass.lpszClassName = szAppNameEng;

	if (!RegisterClass(&wndclass))
	{
		MessageBox(NULL, TEXT("This program requires Windows NT!"),
			szAppNameEng, MB_ICONERROR);
		return false;
	}

	return true;
}

void LoadConfig()
{
	cCfg.LoadFile();

	iWndPixWidth  = cCfg.GetInteger("default", "iWndPixWidth",  640);
	iWndPixHeight = cCfg.GetInteger("default", "iWndPixHeight", 480);
	iDotSize = cCfg.GetInteger("default", "iDotSize", 1);
	dUpVal = cCfg.GetDouble("default", "dUpVal", 255.0);
	dDnVal = cCfg.GetDouble("default", "dDnVal",   0.0);
	szTypeSelect = cCfg.GetString("default", "szTypeSelect", "BYTE");

	bInternalUsed = cCfg.GetBoolean("default", "bInternalUsed");

	szDrawType = cCfg.GetString("default", "szDrawType", "DOT");
	iSampleDrawGap = cCfg.GetInteger("default", "iSampleDrawGap", 1);

	iSampleDrawGap = max(1, iSampleDrawGap);
	iSampleDrawGap = min(iWndPixWidth, iSampleDrawGap);

	#ifdef _DEBUG

	if (bInternalUsed)
	{
		if (szDrawType != "DOT")
		{
			TCHAR szMsg[64];
			wsprintf(szMsg, TEXT("%s%s"),
				TEXT("[内部功能]：注意！\r\n"),
				TEXT("当前绘图样式不为散点！\r\n")
				TEXT("当前样式为：折线"));
			MessageBox(pMain->GetHWND(), szMsg, szAppNameChn, MB_ICONWARNING);
		}

		if (iSampleDrawGap != 1)
		{
			TCHAR szMsg[64];
			wsprintf(szMsg, TEXT("%s%s%d"),
				TEXT("[内部功能]：注意！\r\n"),
				TEXT("当前采样绘图间隔不为1！\r\n")
				TEXT("当前值为："),
				iSampleDrawGap);
			MessageBox(pMain->GetHWND(), szMsg, szAppNameChn, MB_ICONWARNING);
		}
	}

	#endif
}

void SaveConfig()
{
	cCfg.SetInteger("default", "iWndPixWidth",   iWndPixWidth);
	cCfg.SetInteger("default", "iWndPixHeight", iWndPixHeight);
	cCfg.SetInteger("default", "iDotSize",           iDotSize);
	cCfg.SetDouble("default", "dUpVal", dUpVal);
	cCfg.SetDouble("default", "dDnVal", dDnVal);
	cCfg.SetString("default", "szTypeSelect", szTypeSelect);

	cCfg.SetBoolean("default", "bInternalUsed", bInternalUsed);

	cCfg.SetString("default", "szDrawType", szDrawType);
	cCfg.SetInteger("default", "iSampleDrawGap", iSampleDrawGap);

	cCfg.SaveFile();
}

int JudgeType(string sz)
{
	for (int i = 0; i < iTotalType; i++)
	{
		if (!lstrcmp(sz.c_str(), szType[i]))
		{
			return i;
		}
	}

	return -1;
}

void Redraw()
{
	/*HRGN hrgnRedraw = CreateRectRgn(0, 0, 1, 1);

	RECT rcRedraw;
	HRGN hrgnAdd;

	for (int i = 0; i < (iWndPixWidth / iSampleDrawGap); i++)
	{
		double dValuePerPixel = (dUpVal - dDnVal) / iWndPixHeight;

		double dPosY1 = iWndPixHeight - (pdData[i] - dDnVal) / dValuePerPixel;
		double dPosY2 = (dUpVal - pdData[i]) / dValuePerPixel;

		double dErrVal = fabs(dPosY1 - dPosY2);

		double dPosY = (dPosY1 + dPosY2) / 2;

		SetRect(&rcRedraw,
			(i * iSampleDrawGap - iDotSize / 2) - 1,
			(pdData[i] - iDotSize / 2) - 1,
			(i * iSampleDrawGap + iDotSize / 2) + 1,
			(pdData[i] + iDotSize / 2) + 1);
		hrgnAdd = CreateRectRgn(rcRedraw.left, rcRedraw.top, rcRedraw.right, rcRedraw.bottom);
		CombineRgn(hrgnRedraw, hrgnRedraw, hrgnAdd, RGN_OR);
		DeleteRgn(hrgnAdd);
	}
	
	SetRect(&rcRedraw, 0, dMousePosY - 1, iWndPixWidth, dMousePosY + 1);
	hrgnAdd = CreateRectRgn(rcRedraw.left, rcRedraw.top, rcRedraw.right, rcRedraw.bottom);
	CombineRgn(hrgnRedraw, hrgnRedraw, hrgnAdd, RGN_OR);
	DeleteRgn(hrgnAdd);

	SetRect(&rcRedraw, dMousePosX - 1, 0, dMousePosX + 1, iWndPixHeight);
	hrgnAdd = CreateRectRgn(rcRedraw.left, rcRedraw.top, rcRedraw.right, rcRedraw.bottom);
	CombineRgn(hrgnRedraw, hrgnRedraw, hrgnAdd, RGN_OR);
	DeleteRgn(hrgnAdd);*/

	//InvalidateRgn(hwndDraw, hrgnRedraw, TRUE);
	InvalidateRect(hwndDraw, NULL, TRUE);

	UpdateWindow(hwndDraw);

	/*DeleteRgn(hrgnRedraw);*/
}

void ProcessNewRecv(BYTE * pbyData, DWORD dwDataBytes)
{
	static bool bTrans = false;

	static bool bDetect = false;

	static int iDataIndex[iTotalType] = { 0 };

	int iDataType = JudgeType(szTypeSelect);
	static int iLastDataType = DATA_BYTE; // Just only a init value, has no effect on
	                                      // the after process.
	if (iLastDataType != iDataType) // Select type has changed
	{
		bDetect = false;
		memset(iDataIndex, 0, sizeof(iDataIndex));
	}
	iLastDataType = iDataType;

	for (DWORD dw = 0; dw < dwDataBytes; dw++)
	{
		if (bTrans) // Last one is a trans char
		{
			bTrans = false;

			if (TRANS_CHAR == pbyData[dw]) // Info char is same as TRANS_CHAR
			{
				if (bDetect)
				{
					goto RECV_INFO;
				}
			}
			else // Control char
			{
				switch (pbyData[dw])
				{
				case TRANS_ALIGN: // Control char of align, and begin detect info
					bDetect = true;
					memset(iDataIndex, 0, sizeof(iDataIndex));
					break;

				case TRANS_NODET: // Stop detect info
					bDetect = false;
					memset(iDataIndex, 0, sizeof(iDataIndex));
					break;
				}
			}
		}
		else // Last one is not a trans char
		{
			if (TRANS_CHAR == pbyData[dw]) // This one is a trans char
			{
				bTrans = true;
			}
			else // This one is a info char
			{
				if (bDetect)
				{
					goto RECV_INFO;
				}
			}
		}
		goto INFO_CONTINUE; // Control char, or detect is not open, or error

	RECV_INFO:

		static TByte   tByte;
		static TWord   tWord;
		static TDword  tDword;
		static TFloat  tFloat;
		static TDouble tDouble;

		switch (iDataType)
		{
		case DATA_BYTE:

			tByte.u.byData[iDataIndex[iDataType]] = pbyData[dw];
			iDataIndex[iDataType]++;
			
			if (iDataIndex[iDataType] >= sizeof(BYTE))
			{
				iDataIndex[iDataType] = 0;

				if (!bWaveformPause)
				{
					memcpy(pdData, (pdData + 1), (iWndPixWidth / iSampleDrawGap - 1) * sizeof(double));
					pdData[iWndPixWidth / iSampleDrawGap - 1] = static_cast<double>(tByte.u.by);
				}
			}
			
			break;

		case DATA_WORD:

			tWord.u.byData[iDataIndex[iDataType]] = pbyData[dw];
			iDataIndex[iDataType]++;

			if (iDataIndex[iDataType] >= sizeof(WORD))
			{
				iDataIndex[iDataType] = 0;

				if (!bWaveformPause)
				{
					memcpy(pdData, (pdData + 1), (iWndPixWidth / iSampleDrawGap - 1) * sizeof(double));
					pdData[iWndPixWidth / iSampleDrawGap - 1] = static_cast<double>(tWord.u.wd);
				}
			}

			break;

		case DATA_DWORD:

			tDword.u.byData[iDataIndex[iDataType]] = pbyData[dw];
			iDataIndex[iDataType]++;

			if (iDataIndex[iDataType] >= sizeof(DWORD))
			{
				iDataIndex[iDataType] = 0;

				if (!bWaveformPause)
				{
					memcpy(pdData, (pdData + 1), (iWndPixWidth / iSampleDrawGap - 1) * sizeof(double));
					pdData[iWndPixWidth / iSampleDrawGap - 1] = static_cast<double>(tDword.u.dw);
				}
			}

			break;

		case DATA_FLOAT:

			tFloat.u.byData[iDataIndex[iDataType]] = pbyData[dw];
			iDataIndex[iDataType]++;

			if (iDataIndex[iDataType] >= sizeof(FLOAT))
			{
				iDataIndex[iDataType] = 0;

				if (!bWaveformPause)
				{
					memcpy(pdData, (pdData + 1), (iWndPixWidth / iSampleDrawGap - 1) * sizeof(double));
					pdData[iWndPixWidth / iSampleDrawGap - 1] = static_cast<double>(tFloat.u.f);
				}
			}

			break;

		case DATA_DOUBLE:

			tDouble.u.byData[iDataIndex[iDataType]] = pbyData[dw];
			iDataIndex[iDataType]++;

			if (iDataIndex[iDataType] >= sizeof(DOUBLE))
			{
				iDataIndex[iDataType] = 0;

				if (!bWaveformPause)
				{
					memcpy(pdData, (pdData + 1), (iWndPixWidth / iSampleDrawGap - 1) * sizeof(double));
					pdData[iWndPixWidth / iSampleDrawGap - 1] = static_cast<double>(tDouble.u.d);
				}
			}

			break;

		default: // Error occur
			goto INFO_CONTINUE;
		}

	INFO_CONTINUE:
		continue;
	}
}

LRESULT CALLBACK WndProcWaveformDisplay(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	static char szStaticTextPosX [32] = { 0 }, szStaticTextPosY [32] = { 0 };
	static char szStaticTextRealX[32] = { 0 }, szStaticTextRealY[32] = { 0 };

	static int iMouseRBtnX = 0, iMouseRBtnY = 0;

	switch (message)
	{
	case WM_CREATE:
		hdlgValue = CreateDialog(
			GetWindowInstance(pMain->GetHWND()),
			MAKEINTRESOURCE(IDD_DIALOG_WAVEFORM_VALUE),
			pMain->GetHWND(),
			DlgProcWaveformValue);
		ShowWindow(hdlgValue, SW_NORMAL);
		#if _DEBUG_FORCE_FLASH
		SetTimer(hwnd, 0, 55, NULL);
		#endif
		return 0;

	case WM_PAINT:
		{
			PAINTSTRUCT ps;
			HDC hdc = BeginPaint(hwnd, &ps);

			if (pdData)
			{
				HDC hdcBkgd = CreateCompatibleDC(hdc);
				HBITMAP hbmpBkgd = CreateCompatibleBitmap(hdc, iWndPixWidth, iWndPixHeight);
				SelectBitmap(hdcBkgd, hbmpBkgd);
				RECT rcClient = { 0, 0, iWndPixWidth, iWndPixHeight };
				FillRect(hdcBkgd, &rcClient, GetStockBrush(WHITE_BRUSH));

				HBRUSH hBrush = GetStockBrush(BLACK_BRUSH);
				HBRUSH hOldBrush = (HBRUSH)SelectObject(hdcBkgd, hBrush);
				HPEN hPen = GetStockPen(BLACK_PEN);
				HPEN hOldPen = (HPEN)SelectObject(hdcBkgd, hPen);

				for (int i = 0; i < (iWndPixWidth / iSampleDrawGap); i++)
				{
					double dValuePerPixel = (dUpVal - dDnVal) / iWndPixHeight;

					double dPosY1 = iWndPixHeight - (pdData[i] - dDnVal) / dValuePerPixel;
					double dPosY2 = (dUpVal - pdData[i]) / dValuePerPixel;

					double dErrVal = fabs(dPosY1 - dPosY2);

					double dPosY = (dPosY1 + dPosY2) / 2;

					if (szDrawType == TEXT("DOT"))
					{
						if (iDotSize >= 2)
						{
							Ellipse(hdcBkgd,
								(int)(i * iSampleDrawGap - iDotSize / 2),
								(int)(dPosY - iDotSize / 2),
								(int)(i * iSampleDrawGap + iDotSize / 2),
								(int)(dPosY + iDotSize / 2));
						}
						else
						{
							SetPixel(hdcBkgd, i * iSampleDrawGap, (int)dPosY, RGB(0, 0, 0));
						}
					}
					else if (szDrawType == TEXT("LINE"))
					{
						if (0 == i) // First point
						{
							MoveToEx(hdcBkgd, i * iSampleDrawGap, (int)dPosY, NULL);
						}
						else
						{
							LineTo(hdcBkgd, i * iSampleDrawGap, (int)dPosY);
						}
					}
				}

				if (bMouseSelect)
				{
					static HPEN hPenSelectCross = CreatePen(PS_SOLID, 1, RGB(255, 0, 0));
					SelectObject(hdcBkgd, hPenSelectCross);

					MoveToEx(hdcBkgd, 0, (int)dMousePosY, NULL);
					LineTo(hdcBkgd, iWndPixWidth, (int)dMousePosY);

					MoveToEx(hdcBkgd, (int)dMousePosX, 0, NULL);
					LineTo(hdcBkgd, (int)dMousePosX, iWndPixHeight);
				}

				SelectObject(hdcBkgd, hOldBrush);
				SelectObject(hdcBkgd, hOldPen);

				BitBlt(hdc, 0, 0, iWndPixWidth, iWndPixHeight, hdcBkgd, 0, 0, SRCCOPY);
				DeleteBitmap(hbmpBkgd);
				DeleteDC(hdcBkgd);
			}

			EndPaint(hwnd, &ps);
		}
		return 0;

	case WM_WAVEFORMDISPLAY_RECVDATA:
		ProcessNewRecv((PBYTE)wParam, (DWORD)lParam);
		if (!bWaveformPause)
		{
			Redraw();
		}
		return 0;

	case WM_MOUSEMOVE:
		if (!bMouseSelect)
		{
			dMousePosX = GET_X_LPARAM(lParam);
			dMousePosY = GET_Y_LPARAM(lParam);
			dMouseRealX = dMousePosX / iSampleDrawGap;
			dMouseRealY = dDnVal + (dUpVal - dDnVal) * (iWndPixHeight - dMousePosY) / iWndPixHeight;
			
			sprintf_s(szStaticTextPosX,  "%.*lf", PRECISION, dMousePosX);
			sprintf_s(szStaticTextPosY,  "%.*lf", PRECISION, dMousePosY);
			sprintf_s(szStaticTextRealX, "%.*lf", PRECISION, dMouseRealX);
			sprintf_s(szStaticTextRealY, "%.*lf", PRECISION, dMouseRealY);

			SetDlgItemTextA(hdlgValue, IDC_STATIC_POSX, szStaticTextPosX);
			SetDlgItemTextA(hdlgValue, IDC_STATIC_POSY, szStaticTextPosY);
			SetDlgItemTextA(hdlgValue, IDC_STATIC_REALX, szStaticTextRealX);
			SetDlgItemTextA(hdlgValue, IDC_STATIC_REALY, szStaticTextRealY);
		}
		return 0;

	case WM_LBUTTONUP:
		if (wParam & MK_CONTROL)
		{
			bMouseSelect = !bMouseSelect;
			Redraw();
		}
		return 0;

	case WM_MOUSEWHEEL:
		if (GET_KEYSTATE_WPARAM(wParam) & MK_CONTROL)
		{
			int iScale = GET_WHEEL_DELTA_WPARAM(wParam) / WHEEL_DELTA;

			if (GET_KEYSTATE_WPARAM(wParam) & MK_SHIFT)
			{
				if (bInternalUsed)
				{
					iSampleDrawGap += iScale;
					iSampleDrawGap = max(1, iSampleDrawGap);
					iSampleDrawGap = min(iWndPixWidth, iSampleDrawGap);
				}
			}
			else
			{
				if (iScale > 0) // Zoom out
				{
					double dDelta = (dUpVal - dDnVal) / 2;
					dUpVal += dDelta;
					dDnVal -= dDelta;
				}
				else // Zoom in
				{
					double dDelta = (dUpVal - dDnVal) / 4;
					dUpVal -= dDelta;
					dDnVal += dDelta;
				}
			}

			SaveConfig();

			bMouseSelect = false;
			Redraw();
		}
		return 0;

	case WM_RBUTTONDOWN:
		iMouseRBtnX = GET_X_LPARAM(lParam);
		iMouseRBtnY = GET_Y_LPARAM(lParam);
		return 0;

	case WM_RBUTTONUP:
		if (wParam & MK_CONTROL)
		{
			int iMouseRBtnCrntX = GET_X_LPARAM(lParam);
			int iMouseRBtnCrntY = GET_Y_LPARAM(lParam);

			double dOffset = iMouseRBtnCrntY - iMouseRBtnY;
			dOffset = dOffset * ((dUpVal - dDnVal) / iWndPixHeight);
			dUpVal += dOffset;
			dDnVal += dOffset;

			Redraw();
		}
		iMouseRBtnX = 0;
		iMouseRBtnY = 0;
		return 0;

	case WM_CHAR:
		if (wParam == VK_SPACE)
		{
			bWaveformPause = !bWaveformPause;
			SetDlgItemText(hdlgValue, IDOK, bWaveformPause ? TEXT("继续显示") : TEXT("截停显示"));
		}
		return 0;

	case WM_TIMER:

		switch ((UINT_PTR)wParam)
		{
		case 0:
			#if _DEBUG_FORCE_FLASH
			Redraw();
			return 0;
			#endif
			break;
		}

		break;

	case WM_DESTROY:
		DestroyWindow(hdlgValue);
		DestroyWindow(hwnd);
		hdlgValue = NULL;
		hwndDraw = NULL;
		bWaveformPause = false;
		bMouseSelect = false;
		#if _DEBUG_FORCE_FLASH
		KillTimer(hwnd, 0);
		#endif
		return 0;
	}

	return DefWindowProc(hwnd, message, wParam, lParam);
}

BOOL CALLBACK DlgProcWaveformValue(HWND hdlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	static char szStaticTextPosX [32] = { 0 }, szStaticTextPosY [32] = { 0 };
	static char szStaticTextRealX[32] = { 0 }, szStaticTextRealY[32] = { 0 };

	switch (message)
	{
	case WM_INITDIALOG:
		sprintf_s(szStaticTextPosX,  "%.*lf", PRECISION, 0.0);
		sprintf_s(szStaticTextPosY,  "%.*lf", PRECISION, 0.0);
		sprintf_s(szStaticTextRealX, "%.*lf", PRECISION, 0.0);
		sprintf_s(szStaticTextRealY, "%.*lf", PRECISION, 0.0);
		SetDlgItemTextA(hdlg, IDC_STATIC_POSX,   szStaticTextPosX);
		SetDlgItemTextA(hdlg, IDC_STATIC_POSY,   szStaticTextPosY);
		SetDlgItemTextA(hdlg, IDC_STATIC_REALX, szStaticTextRealX);
		SetDlgItemTextA(hdlg, IDC_STATIC_REALY, szStaticTextRealY);
		SetDlgItemText(hdlg, IDOK, bWaveformPause ? TEXT("继续显示") : TEXT("截停显示"));
		return TRUE;

	case WM_COMMAND:
		switch (LOWORD(wParam))
		{
		case IDOK: // Pause/Continue
			bWaveformPause = !bWaveformPause;
			SetDlgItemText(hdlg, IDOK, bWaveformPause ? TEXT("继续显示") : TEXT("截停显示"));
			return TRUE;

		case IDCANCEL: // Rlease mouse select
			bMouseSelect = false;
			Redraw();
			return TRUE;

		case IDC_BTN_WDSETTING: // Waveform setting
			DialogBox(
				GetWindowInstance(pMain->GetHWND()),
				MAKEINTRESOURCE(IDD_DIALOG_WAVEFORM_SETTING),
				hdlg,
				DlgProcWaveformSetting);
			return TRUE;
		}
	}

	return FALSE;
}

BOOL CALLBACK DlgProcWaveformSetting(HWND hdlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	static char szUpVal[32] = { 0 }, szDnVal[32] = { 0 };

	switch (message)
	{
	case WM_INITDIALOG:
		
		SetDlgItemInt(hdlg, IDC_EDIT_WINDOWWIDTH,  iWndPixWidth,  TRUE);
		SetDlgItemInt(hdlg, IDC_EDIT_WINDOWHEIGHT, iWndPixHeight, TRUE);
		SetDlgItemInt(hdlg, IDC_EDIT_DOTSIZE, iDotSize, TRUE);
		
		sprintf_s(szUpVal, "%lf", dUpVal);
		SetDlgItemTextA(hdlg, IDC_EDIT_UPVAL, szUpVal);

		sprintf_s(szDnVal, "%lf", dDnVal);
		SetDlgItemTextA(hdlg, IDC_EDIT_DNVAL, szDnVal);

		for (int i = 0; i < iTotalType; i++)
		{
			SendMessage(GetDlgItem(hdlg, IDC_COMBO_INPUTSELECT),
				CB_ADDSTRING, 0, (LPARAM)szType[i]);
		}
		SendMessage(GetDlgItem(hdlg, IDC_COMBO_INPUTSELECT),
			CB_SETCURSEL, (WPARAM)JudgeType(szTypeSelect), 0);

		return TRUE;

	case WM_COMMAND:

		switch (LOWORD(wParam))
		{
		case IDOK:
			iWndPixWidth  = GetDlgItemInt(hdlg, IDC_EDIT_WINDOWWIDTH,  NULL, TRUE);
			iWndPixHeight = GetDlgItemInt(hdlg, IDC_EDIT_WINDOWHEIGHT, NULL, TRUE);
			iDotSize = GetDlgItemInt(hdlg, IDC_EDIT_DOTSIZE, NULL, TRUE);
			GetDlgItemText(hdlg, IDC_EDIT_UPVAL, szUpVal, sizeof(szUpVal) / sizeof(char));
			GetDlgItemText(hdlg, IDC_EDIT_DNVAL, szDnVal, sizeof(szDnVal) / sizeof(char));
			dUpVal = atof(szUpVal);
			dDnVal = atof(szDnVal);
			szTypeSelect = szType[SendMessage(
				GetDlgItem(hdlg, IDC_COMBO_INPUTSELECT),
				CB_GETCURSEL, 0, 0)];
			SaveConfig();
			EndDialog(hdlg, 0);
			SendMessage(hwndDraw, WM_DESTROY, 0, 0);
			WaveformDisplay(pMain);
			return TRUE;

		case IDCANCEL:
			EndDialog(hdlg, 0);
			return TRUE;
		}
	}

	return FALSE;
}

bool WaveformDisplay(Common::CComWnd * pParent)
{
	if (!pParent)
	{
		return false;
	}
	pMain = pParent;

	static bool bInited = false;
	if (!bInited)
	{
		if (!RegisterDrawWindow())
		{
			return false;
		}
		
		LoadConfig();

		bInited = true;
	}

	if (!hwndDraw)
	{
		hwndDraw = CreateWindow(
			szAppNameEng, szAppNameChn, WS_OVERLAPPED | WS_SYSMENU,
			CW_USEDEFAULT, CW_USEDEFAULT,
			iWndPixWidth, iWndPixHeight,
			pMain->GetHWND(), NULL, GetWindowInstance(pMain->GetHWND()), NULL);

		if (NULL == hwndDraw)
		{
			return false;
		}

		RECT rcWindow;
		RECT rcClient;
		int iBorderWidth = 0, iBorderHeight = 0;

		GetWindowRect(hwndDraw, &rcWindow);
		GetClientRect(hwndDraw, &rcClient);

		iBorderWidth  = (rcWindow.right - rcWindow.left) - (rcClient.right - rcClient.left);
		iBorderHeight = (rcWindow.bottom - rcWindow.top) - (rcClient.bottom - rcClient.top);

		SetWindowPos(hwndDraw, NULL,
			0, 0, iBorderWidth + iWndPixWidth, iBorderHeight + iWndPixHeight,
			SWP_NOMOVE | SWP_NOZORDER);

		if (pdData)
		{
			delete [] pdData;
		}
		pdData = new double [iWndPixWidth / iSampleDrawGap];
		for (int i = 0; i < iWndPixWidth / iSampleDrawGap; i++)
		{
			pdData[i] = (dUpVal + dDnVal) / 2;
		}
	}

	ShowWindow(hwndDraw, SW_NORMAL);

	return true;
}

void WDReceiveData(BYTE * pbyData, DWORD dwDataBytes)
{
	// Recv data buffer, it must be exactly the same as or over of comm.cpp(Line368)
	//static BYTE byRecvBuff[1 << 20];
	static BYTE byRecvBuff[1 << 22];

	if (hwndDraw) // Waveform display has been open
	{
		memcpy(byRecvBuff, pbyData, dwDataBytes);
		PostMessage(hwndDraw,
			WM_WAVEFORMDISPLAY_RECVDATA, (WPARAM)byRecvBuff, (LPARAM)dwDataBytes);
	}
}

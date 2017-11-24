#pragma once

#include <Windows.h>

bool WaveformDisplay(Common::CComWnd * pParent);
void WDReceiveData(BYTE * pbyData, DWORD dwDataBytes);
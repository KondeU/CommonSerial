/************************************************************

GNU GENERAL PUBLIC LICENSE, Version 3, 29 June 2007
Copyright (c) 2017, KondeU, All rights reserved.

Project:     CommonSerial
File:        WaveformDisplay.h
Description: Add functions to original common.
Date:        2017-11-22
Version:     2.01
Authors:     Deyou Kong <370242479@qq.com>
History:     01, 17-11-22, Deyou Kong, Create file and implement it.

************************************************************/

#pragma once

#include <Windows.h>

bool WaveformDisplay(Common::CComWnd * pParent);
void WDReceiveData(BYTE * pbyData, DWORD dwDataBytes);
/*

BSD 2-Clause License

Copyright (c) 2017, KondeU
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

* Redistributions of source code must retain the above copyright notice, this
list of conditions and the following disclaimer.

* Redistributions in binary form must reproduce the above copyright notice,
this list of conditions and the following disclaimer in the documentation
and/or other materials provided with the distribution.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

*/

//--------------------------------------------//
// CConfig.h                                  //
//                                            //
// Class CConfig is declared in this header   //
// file.                                      //
// Can run in multi OS.                       //
//                                            //
// Writed by Deyou Kong, 2016-12-10           //
// Checked by Deyou Kong, 2016-12-11          //
// Found BUGs, see the "Note" in CConfig.cpp, //
//                     Deyou Kong, 2017-06-03 //
//--------------------------------------------//

#pragma once

#include <fstream>
#include <sstream>
#include <string>
#include <map>
#include <xfunctional>
#include <cctype>
using namespace std;

class CConfig
{

public:

	CConfig();
	CConfig(string szFilePath);
	~CConfig();

	inline void SetFilePath(string szFilePath);
	inline string GetFilePath();

	inline void SetAutoSave(bool bAutoSave);
	inline bool GetAutoSave();

	bool LoadFile(unsigned int * puiLine = nullptr);
	bool SaveFile();
	bool SaveFileAs(string szFilePath);

	void   SetString(string szGroup, string szKey, string szValue);
	string GetString(string szGroup, string szKey, string szDefaultValue = "");

	void SetInteger(string szGroup, string szKey, int iValue);
	int  GetInteger(string szGroup, string szKey, int iDefaultValue = 0);

	void   SetDouble(string szGroup, string szKey, double dValue);
	double GetDouble(string szGroup, string szKey, double dDefaultValue = 0.0);

	void SetBoolean(string szGroup, string szKey, bool bValue);
	bool GetBoolean(string szGroup, string szKey, bool bDefaultValue = false);

protected:

	const string mk_szGroupDefault = "default";

	static void TrimString(string & szStr);

	void   SetValue(string szGroup, string szKey, string szValue);
	string GetValue(string szGroup, string szKey);

private:

	string m_szFilePath;
	
	bool m_bAutoSave; 
	bool m_bModifyed;
	// m_bAutoSave is true means the config will be saved in current file
	// if m_bModifyed is true when the class is being destructed.
	// m_bModifyed is true means the config has been modifyed.

	typedef map<string, string, greater<string>> Pair;
	typedef map<string, Pair,   greater<string>> Paragraph;

	Paragraph m_paraGroup;

};

inline void CConfig::SetFilePath(string szFilePath)
{
	m_szFilePath = szFilePath;
}

inline string CConfig::GetFilePath()
{
	return m_szFilePath;
}

inline void CConfig::SetAutoSave(bool bAutoSave)
{
	m_bAutoSave = bAutoSave;
}

inline bool CConfig::GetAutoSave()
{
	return m_bAutoSave;
}
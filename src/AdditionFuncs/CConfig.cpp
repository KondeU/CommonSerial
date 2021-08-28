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

#include "StdAfx.h" // Using in this "common" project.

#include "CConfig.h"

CConfig::CConfig()
{
	m_bAutoSave = false;
	m_bModifyed = false;
}

CConfig::CConfig(string szFilePath)
{
	m_szFilePath = szFilePath;
	m_bAutoSave = false;
	m_bModifyed = false;
}

CConfig::~CConfig()
{
	if (m_bAutoSave && m_bModifyed)
	{
		SaveFile();
	}
}

bool CConfig::LoadFile(unsigned int * puiLine)
{
	m_paraGroup.clear();
	unsigned int uiLine = 0; // Lines counter

	ifstream fin(m_szFilePath.c_str());

	if (!fin)
	{
		return false;
	}

	//-- Begin process data --//

	Pair pairKey;                            // Readed keys in current group
	string szCrntGroup = mk_szGroupDefault;  // Current group
	string szData;                           // Readed text data

	while (getline(fin, szData))
	{
		uiLine++;

		TrimString(szData); // Delete the line's forward and back space

		if (0 == szData.length()) // Empty line
		{
			continue;
		}

		if ('#' == szData[0]) // Comment
		{
			continue;
		}

		string::size_type idxEqualSign = szData.find("="); // The equal sign position in szData

		if (idxEqualSign == std::string::npos) // Cannot find the equal sign
		{
			if (szData[0] == '[' && szData[szData.length() - 1] == ']')
			{
				szData.erase(szData.length() - 1); // Delete the ']'
				szData.erase(0, 1);                // Delete the '['

				m_paraGroup[szCrntGroup] = pairKey; // Add keys to group

				szCrntGroup = szData; // Change to next group

				pairKey.clear(); // Clear the keys container
			}
			else
			{
				fin.close();

				if (puiLine)
				{
					(*puiLine) = uiLine;
				}
				return false;
			}
		}
		else
		{
			// Obtain the key
			string szKey = szData.substr(0, idxEqualSign);
			TrimString(szKey);

			// Obtain the value
			string szValue;
			string szValueTemp = szData.substr(idxEqualSign + 1);
			string::size_type idxCommentSign = szValueTemp.find("#"); // Find '#' for comment
			if (idxCommentSign == std::string::npos) // Cannot find comment
			{
				szValue = szValueTemp;
			}
			else
			{
				szValue = szValueTemp.substr(0, idxCommentSign);
			}
			TrimString(szValue);

			// Add to pairKey
			pairKey[szKey] = szValue;
		}
	}
	m_paraGroup[szCrntGroup] = pairKey; // Add the last group of keys to group

	//-- End process data --//

	fin.close();

	if (puiLine)
	{
		(*puiLine) = uiLine;
	}

	return true;
}

bool CConfig::SaveFile()
{
	bool bRet = SaveFileAs(m_szFilePath);

	if (bRet)
	{
		m_bModifyed = false;
	}

	return bRet;
}

bool CConfig::SaveFileAs(string szFilePath)
{
	// Note 1: the comments will disappear when saving config
	// Note 2: the group and key will be sorted by reversed alpha order when saving config

	ofstream fout(szFilePath.c_str());

	if (!fout)
	{
		return false;
	}

	Pair pairKey = m_paraGroup[mk_szGroupDefault];

	Pair     ::iterator pairKeyItrtr;
	Paragraph::iterator paraGroupItrtr;

	// Output default group
	for (pairKeyItrtr = pairKey.begin(); pairKeyItrtr != pairKey.end(); ++pairKeyItrtr)
	{
		fout << pairKeyItrtr->first << " = " << pairKeyItrtr->second << endl;
	}
	fout << endl;

	// Output other groups
	for (paraGroupItrtr = m_paraGroup.begin(); paraGroupItrtr != m_paraGroup.end(); ++paraGroupItrtr)
	{
		// Omit the default group
		if (paraGroupItrtr->first == mk_szGroupDefault)
		{
			continue;
		}
		// Output group name which is in the paraGroup first segment
		fout << '[' << paraGroupItrtr->first << ']' << endl;
		// Output key and value which is in the paraGroup second segment
		pairKey = paraGroupItrtr->second;
		for (pairKeyItrtr = pairKey.begin(); pairKeyItrtr != pairKey.end(); ++pairKeyItrtr)
		{
			fout << pairKeyItrtr->first << " = " << pairKeyItrtr->second << endl;
		}
		fout << endl;
	}

	return true;
}

void CConfig::SetString(string szGroup, string szKey, string szValue)
{
	SetValue(szGroup, szKey, szValue);
}

string CConfig::GetString(string szGroup, string szKey, string szDefaultValue)
{
	string szValue = GetValue(szGroup, szKey);
	return ((szValue.length() > 0) ? szValue : szDefaultValue);
}

void CConfig::SetInteger(string szGroup, string szKey, int iValue)
{
	stringstream ss;
	string sz;
	ss << iValue;
	ss >> sz;
	SetValue(szGroup, szKey, sz);
}

int CConfig::GetInteger(string szGroup, string szKey, int iDefaultValue)
{
	int iValue = iDefaultValue;

	string sz = GetValue(szGroup, szKey);
	if (sz.length() > 0)
	{
		stringstream ss;
		ss << sz;
		ss >> iValue;
	}
	return iValue;
}

void CConfig::SetDouble(string szGroup, string szKey, double dValue)
{
	stringstream ss;
	string sz;
	ss << dValue;
	ss >> sz;
	SetValue(szGroup, szKey, sz);
}

double CConfig::GetDouble(string szGroup, string szKey, double dDefaultValue)
{
	double dValue = dDefaultValue;

	string sz = GetValue(szGroup, szKey);
	if (sz.length() > 0)
	{
		stringstream ss;
		ss << sz;
		ss >> dValue;
	}
	return dValue;
}

void CConfig::SetBoolean(string szGroup, string szKey, bool bValue)
{
	string sz = bValue ? "true" : "false";
	SetValue(szGroup, szKey, sz);
}

bool CConfig::GetBoolean(string szGroup, string szKey, bool bDefaultValue)
{
	bool bValue = bDefaultValue;

	string sz = GetValue(szGroup, szKey);
	if (sz.length() > 0)
	{
		if (sz == string("true"))
		{
			bValue = true;
		}
		else
		{
			bValue = false;
		}
	}
	return bValue;
}

void CConfig::TrimString(string & szStr)
{
	unsigned int uiPos = 0;

	// szStr is empty
	if (szStr.length() <= 0)
	{
		return;
	}

	// Delete forward space
	for (uiPos = 0; uiPos < szStr.length() && isspace(szStr[uiPos]); uiPos++);
	szStr = szStr.substr(uiPos);

	// After delete forward space, szStr become empty
	if (szStr.length() <= 0)
	{
		return;
	}

	// Delete back space
	for (uiPos = szStr.length() - 1; uiPos >= 0 && isspace(szStr[uiPos]); uiPos--);
	szStr.erase(uiPos + 1);
}

void CConfig::SetValue(string szGroup, string szKey, string szValue)
{
	m_bModifyed = true;

	Pair & pairKey = m_paraGroup[szGroup];
	pairKey[szKey] = szValue;
}

string CConfig::GetValue(string szGroup, string szKey)
{
	string szValue;	// If donot have that group or key, szValue.length will be zero

	Paragraph::iterator paraGroupItrtr = m_paraGroup.find(szGroup);

	if (paraGroupItrtr != m_paraGroup.end())
	{
		Pair pairKey = paraGroupItrtr->second;

		Pair::iterator paraKeyItrtr = pairKey.find(szKey);

		if (paraKeyItrtr != pairKey.end())
		{
			szValue = paraKeyItrtr->second;
		}
	}

	return szValue;
}

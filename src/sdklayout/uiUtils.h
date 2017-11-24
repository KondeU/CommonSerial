#ifndef __UTILS_H__
#define __UTILS_H__

#pragma once

namespace SdkLayout
{
	UINT HashKey(LPCTSTR Key);
	void split_string(std::vector<std::string>* vec, const char* str, char delimiter);

	/////////////////////////////////////////////////////////////////////////////////////
	//

	class CDuiRect : public tagRECT
	{
	public:
		CDuiRect();
		CDuiRect(const RECT& src);
		CDuiRect(int iLeft, int iTop, int iRight, int iBottom);

		int GetWidth() const;
		int GetHeight() const;
		void Empty();
		bool IsNull() const;
		void Join(const RECT& rc);
		void ResetOffset();
		void Normalize();
		void Offset(int cx, int cy);
		void Inflate(int cx, int cy);
		void Deflate(int cx, int cy);
		void Union(CDuiRect& rc);
	};

	/////////////////////////////////////////////////////////////////////////////////////
	//

	class CStdPtrArray
	{
	public:
		CStdPtrArray(int iPreallocSize = 0);
		CStdPtrArray(const CStdPtrArray& src);
		~CStdPtrArray();

		void Empty();
		void Resize(int iSize);
		bool IsEmpty() const;
		int Find(LPVOID iIndex) const;
		bool Add(LPVOID pData);
		bool SetAt(int iIndex, LPVOID pData);
		bool InsertAt(int iIndex, LPVOID pData);
		bool Remove(int iIndex);
		int GetSize() const;
		LPVOID* GetData();

		LPVOID GetAt(int iIndex) const;
		LPVOID operator[] (int nIndex) const;

	protected:
		LPVOID* m_ppVoid;
		int m_nCount;
		int m_nAllocated;
	};

	class CTinyString
	{
	public:
		CTinyString();
		CTinyString(LPCTSTR lpsz);
		const CTinyString& operator=(LPCTSTR lpsz);
		operator LPCTSTR() const;

		void clear();
		int size();
		bool empty();

		friend bool operator==(const CTinyString& lhs, const CTinyString& rhs);
		friend bool operator==(const CTinyString& lhs, LPCTSTR rhs);
		friend bool operator==(LPCTSTR lhs, const CTinyString& rhs);

	protected:
		TCHAR _szBuff[64];
	};

	bool operator==(const CTinyString& lhs, const CTinyString& rhs);
	bool operator==(const CTinyString& lhs, LPCTSTR rhs);
	bool operator==(LPCTSTR lhs, const CTinyString& rhs);

}// namespace SdkLayout

#endif // __UTILS_H__

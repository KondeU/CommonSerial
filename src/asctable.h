#pragma once

namespace Common{
	class c_asctable_dlg : public c_dialog_builder
	{
	public:
		c_asctable_dlg();
		virtual LRESULT		handle_message(UINT uMsg, WPARAM wParam, LPARAM lParam, bool& bHandled);
		virtual void		on_final_message(HWND hwnd) { __super::on_final_message(hwnd); delete this;}
		virtual LPCTSTR		get_skin_xml() const override;
		virtual LPCTSTR		get_window_name() const override { return "ASCII���: ���: ǰ��ɫ, �Ҽ�: ����ɫ"; }
		virtual DWORD		get_window_style() const override;

	protected:
		static int axisx, axisy;
		HFONT _hFont;
		static int _fgcolor;
		static int _bgcolor;
	};
}
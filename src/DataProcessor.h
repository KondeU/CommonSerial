#pragma once

namespace Common{
	//////////////////////////////////////////////////////////////////////////
	//////////////////////////////////////////////////////////////////////////
	//////////////////////////////////////////////////////////////////////////
	// ���ݴ������ӿ�: �����ı������� 16���ƹ�����, ����������ݽ���������
	// һ������Ҫ�к�����������ݴ���̳д˽ӿ�, �������ֱ�Ӵ���, ����'\t'�Ĵ���Ͳ���Ҫ
	class i_data_processor
	{
	public:
		// ����������: 
		//		follow:	ǰһ�ε����Ƿ�Ϊ��������, Ҳ��������������
		//		ba:		Byte Array, �ֽ�����
		//		cb:		Count of Bytes, �ֽ���
		//		*pn:	���δ����˶�������
		// ����ֵ:
		//		bool:	�Ƿ�ϣ����������, Ӱ����һ�ε���ʱfollow��ֵ
		virtual bool process_some(bool follow, const unsigned char* ba, int cb, int* pn) = 0;

		// �������ݴ�����: ����, �ڹرմ��ں�, �����16�������ݺ�
		virtual void reset_buffer() = 0;

		virtual operator i_data_processor*() = 0;
	};

	// ���ݽ������ӿ�: �����ڽ��յ����ݺ�������еĽ�����
	class i_data_receiver
	{
	public:
		// ���ݽ��պ���, ���߳̽��յ�����ʱ���ô˺���
		// baָ������ݲ�Ӧ�ñ�����!
		virtual void receive(const unsigned char* ba, int cb) = 0;
		virtual void reset_buffer() = 0;
	protected:
		// һ�����ô�����������ʣ��������������ñ�־�ĸ�������
		virtual bool process(i_data_processor* proc, bool follow, const unsigned char** pba, int* pcb, i_data_processor** ppre)
		{
			int n;
			bool c = proc->process_some(follow, *pba, *pcb, &n);
			SMART_ASSERT(n <= *pcb)(n)(*pcb).Fatal();
			*pba += n;
			*pcb -= n;
			*ppre = c ? proc : NULL;
			return c;
		}
	};

	//////////////////////////////////////////////////////////////////////////
	//////////////////////////////////////////////////////////////////////////
	//////////////////////////////////////////////////////////////////////////
	class c_single_byte_processor : public i_data_processor
	{
	public:
		virtual operator i_data_processor*() { return static_cast<i_data_processor*>(this); }
		virtual bool process_some(bool follow, const unsigned char* ba, int cb, int* pn);
		virtual void reset_buffer() {}

	public:
		Window::c_rich_edit*	_richedit;
	};

	class c_crlf_data_processor : public i_data_processor
	{
	public:
		virtual operator i_data_processor*() { return static_cast<i_data_processor*>(this); }
		virtual bool process_some(bool follow, const unsigned char* ba, int cb, int* pn);
		virtual void reset_buffer();

		c_crlf_data_processor()
			: _post_len(0)
		{}

	protected:
		int _post_len;
		c_byte_array<16, 64> _data;
	public:
		Window::c_rich_edit* _richedit;
	};

	// Linux�����ַ�����
	// http://www.cnblogs.com/memset/p/linux_printf_with_color.html
	// http://ascii-table.com/ansi-escape-sequences.php
	class c_escape_data_processor : public i_data_processor
	{
	public:
		virtual operator i_data_processor*() { return static_cast<i_data_processor*>(this); }
		virtual bool process_some(bool follow, const unsigned char* ba, int cb, int* pn);
		virtual void reset_buffer();

	protected:
		enum lcs_state{
			LCS_NONE,LCS_ESC,LCS_BRACKET,LCS_VAL,LCS_SEMI,
			LCS_H,LCS_f,
			LCS_A,LCS_B,LCS_C,LCS_D,
			LCS_s,LCS_u,LCS_j,LCS_K,
			LCS_h,LCS_l,LCS_EQU,
			LCS_m,LCS_P
		} _state;
		c_byte_array<64, 64> _data;		// ����ջ
		std::vector<lcs_state> _stack;	// ״̬ջ
	public:
		Window::c_rich_edit* _richedit;
	};

	class c_ascii_data_processor : public i_data_processor
	{
	public:
		virtual operator i_data_processor*() { return static_cast<i_data_processor*>(this); }
		virtual bool process_some(bool follow, const unsigned char* ba, int cb, int* pn);
		virtual void reset_buffer();

	public:
		Window::c_rich_edit* _richedit;
	};

	// ������չASCII�ַ����� (CodePage936 compatible, EUC-CN)
	// http://en.wikipedia.org/wiki/GB_2312
	// http://zh.wikipedia.org/wiki/GB_2312
	// http://www.knowsky.com/resource/gb2312tbl.htm
	class c_gb2312_data_processor : public i_data_processor
	{
	public:
		c_gb2312_data_processor()
			: _lead_byte(0)
		{}
		virtual operator i_data_processor*() { return static_cast<i_data_processor*>(this); }
		virtual bool process_some(bool follow, const unsigned char* ba, int cb, int* pn);
		virtual void reset_buffer();

	public:
		unsigned char _lead_byte;			// ����ǰ���ַ�
		Window::c_rich_edit* _richedit;
	};

	//////////////////////////////////////////////////////////////////////////
	class c_text_data_receiver : public i_data_receiver
	{
	public:
		c_text_data_receiver()
			: _pre_proc(0)
			, _rich_editor(0)
		{}

		// interface i_data_receiver
		virtual void receive(const unsigned char* ba, int cb);
		virtual void reset_buffer(){
			_pre_proc = 0;
			_proc_ascii.reset_buffer();
			_proc_escape.reset_buffer();
			_proc_crlf.reset_buffer();
			_proc_gb2312.reset_buffer();
		}
		void set_editor(Window::c_rich_edit* edt) {
			_rich_editor = edt;
			_proc_byte._richedit = edt;
			_proc_ascii._richedit = edt;
			_proc_escape._richedit = edt;
			_proc_crlf._richedit = edt;
			_proc_gb2312._richedit = edt;
		}

	protected:
		Window::c_rich_edit*		_rich_editor;
		i_data_processor*			_pre_proc;
		c_single_byte_processor		_proc_byte;
		c_crlf_data_processor		_proc_crlf;
		c_escape_data_processor		_proc_escape;
		c_ascii_data_processor		_proc_ascii;
		c_gb2312_data_processor		_proc_gb2312;
	};

	//////////////////////////////////////////////////////////////////////////
	class c_hex_data_processor : public i_data_processor
	{
	public:
		c_hex_data_processor()
			: _editor(0)
			, _count(0)
		{}

		virtual operator i_data_processor*() { return static_cast<i_data_processor*>(this); }
		virtual bool process_some(bool follow, const unsigned char* ba, int cb, int* pn);
		virtual void reset_buffer();
		void set_count(int n) { _count = n; }

		Window::c_edit*			_editor;

	private:
		int _count;
	};

	class c_hex_data_receiver : public i_data_receiver
	{
	public:
		c_hex_data_receiver()
			: _pre_proc(0)
			, _editor(0)
		{}

		virtual void receive(const unsigned char* ba, int cb);
		virtual void reset_buffer(){
			_pre_proc = 0;
			_proc_hex.reset_buffer();
		}
		void set_editor(Window::c_edit* edt) {
			_editor = edt;
			_proc_hex._editor = edt;
		}

		void set_count(int n){ _proc_hex.set_count(n); }
	protected:
		Window::c_edit*			_editor;
		i_data_processor*		_pre_proc;
		c_hex_data_processor	_proc_hex;
	};

	//////////////////////////////////////////////////////////////////////////
	class c_file_data_receiver : public i_data_receiver {
	public:
		c_file_data_receiver()
		{}

		virtual void receive(const unsigned char* ba, int cb) {
			_data.append(ba, cb);
		}
		virtual void reset_buffer() {
			_data.empty();
		}

		size_t size() { 
			return _data.get_size(); 
		}
		unsigned char* data() {
			return reinterpret_cast<unsigned char*>(_data.get_data());
		}
	protected:
		c_byte_array<1 << 20, 1 << 20> _data;
	};
}

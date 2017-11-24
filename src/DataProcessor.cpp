#include "stdafx.h"

namespace Common{
	//////////////////////////////////////////////////////////////////////////
	bool c_hex_data_processor::process_some(bool follow, const unsigned char* ba, int cb, int* pn)
	{
		char buf[1024];
		char* str;

		int pos = _count % (COMMON_LINE_CCH_RECV);

		int outlen = cb;
		str = c_text_formatting::hex2str(const_cast<unsigned char*>(ba), &outlen, COMMON_LINE_CCH_RECV, pos,
			buf, sizeof(buf), c_text_formatting::newline_type::NLT_CRLF);

		_editor->append_text(str);

		if (str != buf) memory.free((void**)&str, "");

		_count += cb;

		*pn = cb;
		return false;
	}

	void c_hex_data_processor::reset_buffer()
	{
		_count = 0;
	}

	void c_hex_data_receiver::receive(const unsigned char* ba, int cb)
	{
		for (; cb > 0;){
			if (_pre_proc){ // ���ܴ����cb==0, ���Բ���process�ķ���ֵ
				process(_pre_proc, true, &ba, &cb, &_pre_proc);
				continue;
			}

			if (process(_proc_hex, false, &ba, &cb, &_pre_proc) || cb==0)
				continue;
		}
	}

	//////////////////////////////////////////////////////////////////////////
	bool c_single_byte_processor::process_some(bool follow, const unsigned char* ba, int cb, int* pn)
	{
		SMART_ASSERT(_richedit != NULL).Fatal();
		SMART_ASSERT(follow == false).Warning();
		//SMART_ASSERT(cb == 1)(cb).Fatal();

		char buf[5];

		sprintf(buf, "<%02X>", *ba);
		_richedit->append_text(buf);

		*pn = 1;
		return false;
	}

	//////////////////////////////////////////////////////////////////////////
	bool c_crlf_data_processor::process_some(bool follow, const unsigned char* ba, int cb, int* pn)
	{
		char inner_buf[1024];		// �ڲ�����
		int n = 0;					// ��¼��crlf�ĸ���
		char* str;

		if (!follow){
			_post_len = 0;
			_data.empty();
		}

		while (n < cb && (ba[n] == '\r' || ba[n] == '\n')){
			n++;
		}

		if (n <= 0){
			*pn = 0;
			return false;
		}

		_data.append(ba, n);

		str = c_text_formatting::hex2chs((unsigned char*)_data.get_data(), _data.get_size(),
			inner_buf, __ARRAY_SIZE(inner_buf), c_text_formatting::NLT_CR);

		do{
			int newcrlen = strlen(str);
			int diff = newcrlen - _post_len;

			if (diff > 0){
				_richedit->append_text(str + _post_len);
			}
			else{
				_richedit->back_delete_char(-diff);
			}
		} while ((0));

		_post_len = strlen(str);

		if (str != inner_buf)
			memory.free((void**)&str, "");

		*pn = n;
		return true;
	}

	void c_crlf_data_processor::reset_buffer()
	{
		_post_len = 0;
		_data.empty();
	}

	//////////////////////////////////////////////////////////////////////////
	static int read_integer(char* str, int* pi)
	{
		int r = 0;
		char* p = str;

		while (*p >= '0' && *p <= '9'){
			r *= 10;
			r += *p - '0';
			p++;
		}

		*pi = r;
		return (int)p - (int)str;
	}

	bool c_escape_data_processor::process_some(bool follow, const unsigned char* ba, int cb, int* pn)
	{
		int i = 0;
		int step;

		if (!follow){
			reset_buffer();
		}

		bool r = true;

		for (i = 0; i < cb; i += step){
			step = 0;
			switch (_state)
			{
			case LCS_NONE:
				if (ba[i] == '\033'){
					_state = LCS_ESC;
					step = 1;
					_data.append_char(ba[i]);
				}
				else{
					debug_out(("state: LCS_NONE: expect:\\033, but 0x%02X!\n", ba[i]));
					r = false;
					goto _exit;
				}
				break;
			case LCS_ESC:
				if (ba[i] == '['){
					_state = LCS_BRACKET;
					step = 1;
					_data.append_char(ba[i]);
				}
				else{
					debug_out(("state: LCS_ESC: expect: [, but 0x%02X\n", ba[i]));
					_state = LCS_NONE;
					r = false;
					goto _exit;
				}
				break;
			case LCS_BRACKET:
				if (ba[i] >= '0' && ba[i] <= '9'){
					while (i + step<cb && ba[i + step] >= '0' && ba[i + step] <= '9'){
						_data.append_char(ba[i + step]);
						step++;
					}
					_state = LCS_VAL;
				}
				else if (ba[i] == 'H'){
					_data.append_char('H');
					step = 1;
					_state = LCS_H;
				}
				else if (ba[i] == 'f'){
					_data.append_char('f');
					step = 1;
					_state = LCS_f;
				}
				else if (ba[i] == 'm'){
					_data.append_char('m');
					step = 1;
					_state = LCS_m;
				}
				else if (ba[i] == 's'){
					_data.append_char('s');
					step = 1;
					_state = LCS_s;
				}
				else if (ba[i] == 'u'){
					_data.append_char('u');
					step = 1;
					_state = LCS_u;
				}
				else if (ba[i] == 'K'){
					_data.append_char('K');
					step = 1;
					_state = LCS_K;
				}
				else if (ba[i] == 'm'){
					_data.append_char('m');
					step = 1;
					_state = LCS_m;
				}
				else if (ba[i] == '='){
					_data.append_char('=');
					step = 1;
					_state = LCS_EQU;
				}
				else if (ba[i] == 'A'){
					_data.append_char('A');
					step = 1;
					_state = LCS_A;
				}
				else if (ba[i] == 'B'){
					_data.append_char('B');
					step = 1;
					_state = LCS_B;
				}
				else if (ba[i] == 'C'){
					_data.append_char('C');
					step = 1;
					_state = LCS_C;
				}
				else if (ba[i] == 'D'){
					_data.append_char('D');
					step = 1;
					_state = LCS_D;
				}
				else{
					debug_out(("state: LCS_BRACKET: unexpected token: %02X\n", ba[i]));
					_state = LCS_NONE;
					r = false;
					goto _exit;
				}
				break;
			case LCS_VAL:
				if (ba[i] >= '0' && ba[i] <= '9'){
					while (i + step < cb && ba[i + step] >= '0' && ba[i + step] <= '9'){
						_data.append_char(ba[i + step]);
						step++;
					}
					_state = LCS_VAL;
					goto _no_push_state;
				}
				if (_stack[_stack.size() - 2] == LCS_BRACKET){
					if (ba[i] == ';'){
						_state = LCS_SEMI;
						step = 1;
						_data.append_char(';');
					}
					else if (ba[i] == 'A'){
						_state = LCS_A;
						step = 1;
						_data.append_char('A');
					}
					else if (ba[i] == 'B'){
						_state = LCS_B;
						step = 1;
						_data.append_char('B');
					}
					else if (ba[i] == 'C'){
						_state = LCS_C;
						step = 1;
						_data.append_char('C');
					}
					else if (ba[i] == 'D'){
						_state = LCS_D;
						step = 1;
						_data.append_char('D');
					}
					else if (ba[i] == 'j'){
						_state = LCS_j;
						step = 1;
						_data.append_char('j');
					}
					else if (ba[i] == 'm'){
						_state = LCS_m;
						step = 1;
						_data.append_char('m');
					}
					else{
						debug_out(("state: LCS_VAL: unknown token: %02X\n", ba[i]));
						_state = LCS_NONE;
						r = false;
						goto _exit;

					}
				}
				else if (_stack[_stack.size() - 2] == LCS_EQU){
					if (ba[i] == 'h'){
						_state = LCS_h;
						step = 1;
						_data.append_char('h');
					}
					else if (ba[i] == 'l'){
						_state = LCS_l;
						step = 1;
						_data.append_char('l');
					}
					else{
						debug_out(("state: LCS_EQU+LCS_VAL: unexpected token: %02X\n", ba[i]));
						_state = LCS_NONE;
						r = false;
						goto _exit;
					}
				}
				else if (_stack[_stack.size() - 2] == LCS_SEMI){
					if (_stack.size() == 5){
						if (ba[i] == 'H'){
							_state = LCS_H;
							step = 1;
							_data.append_char('H');
							goto _push_state;
						}
						else if (ba[i] == 'f'){
							_state = LCS_f;
							step = 1;
							_data.append_char('f');
							goto _push_state;
						}
					}

					if(ba[i] == ';'){
						_state = LCS_SEMI;
						step = 1;
						_data.append_char(';');
					}
					else if (ba[i] == 'm'){
						_state = LCS_m;
						step = 1;
						_data.append_char('m');
					}
					else{
						debug_out(("state: LCS_SEMI+LCS_VAL: unexpected token: %02X\n", ba[i]));
						_state = LCS_NONE;
						r = false;
						goto _exit;
					}
				}
				else{
					debug_out(("state: LCS_VAL: unexpected token: %02X\n", ba[i]));
					r = false;
					goto _exit;
				}
				break;
			case LCS_SEMI:
				if (ba[i] >= '0' && ba[i] <= '9'){
					while (i+step<cb && ba[i + step] >= '0' && ba[i + step] <= '9'){
						_data.append_char(ba[i + step]);
						step++;
					}
					_state = LCS_VAL;
				}
				else if (ba[i] == 'm'){
					_state = LCS_m;
					step = 1;
					_data.append_char('m');
				}
				else if (ba[i] == 'H'){
					_state = LCS_H;
					step = 1;
					_data.append_char('H');
				}
				else if (ba[i] == 'f'){
					_state = LCS_f;
					step = 1;
					_data.append_char('f');
				}
				else if (ba[i] == ';'){
					_state = LCS_SEMI;
					step = 1;
					_data.append_char(';');
				}
				else{
					debug_out(("state: LCS_SEMI: unknown token: %02X\n", ba[i]));
					r = false;
					goto _exit;
				}
				break;
			case LCS_EQU:
				if (ba[i] >= '0' && ba[i] <= '9'){
					while (i + step < cb && ba[i + step] >= '0' && ba[i + step] <= '9'){
						_data.append_char(ba[i + step]);
						step++;
					}
					_state = LCS_VAL;
				}
				else if (ba[i] == 'h'){
					_state = LCS_h;
					step = 1;
					_data.append_char('h');
				}
				else if (ba[i] == 'l'){
					_state = LCS_l;
					step = 1;
					_data.append_char('l');
				}
				else{
					debug_out(("state: LCS_EQU: unknown token %02X\n", ba[i]));
					r = false;
					_state = LCS_NONE;
					goto _exit;
				}
				break;
			case LCS_H:	case LCS_f:
			case LCS_A:	case LCS_B:	case LCS_C:	case LCS_D:
			case LCS_s:	case LCS_u:
			case LCS_j:
			case LCS_K:
			case LCS_m:
			case LCS_h:	case LCS_l:
				debug_out(("parsing completed\n"));
				r = false;
				_state = LCS_NONE;
				_data.append_char('\0');
				_richedit->apply_linux_attributes((char*)_data.get_data());
				goto _exit;
			default:
				debug_out(("unknown lcs token\n"));
				r = false;
				_state = LCS_NONE;
				goto _exit;
			}
		_push_state:
			_stack.push_back(_state);
		_no_push_state:
			;
		}

	_exit:
		*pn = i;
		return r;
	}

	void c_escape_data_processor::reset_buffer()
	{
		_state = LCS_NONE;
		_stack.clear();
		_stack.push_back(_state);
		_data.empty();
	}

	//////////////////////////////////////////////////////////////////////////
	bool c_ascii_data_processor::process_some(bool follow, const unsigned char* ba, int cb, int* pn)
	{
		char buf[1024];
		int n = 0;

		while (n < cb && n<sizeof(buf)-1 && ba[n] >= 0x20 && ba[n] <=0x7F){
			buf[n] = ba[n];
			n++;
		}

		buf[n] = '\0';
		_richedit->append_text(buf);

		*pn = n;
		return false;
	}

	void c_ascii_data_processor::reset_buffer()
	{

	}

	//////////////////////////////////////////////////////////////////////////
	void c_text_data_receiver::receive(const unsigned char* ba, int cb)
	{
		for (; cb > 0;){
			if (_pre_proc){// ���ܴ����cb==0, ���Բ���process�ķ���ֵ
				process(_pre_proc, true, &ba, &cb, &_pre_proc);
				continue;
			}

			// �����ַ�/�����ַ�����
			if (0 <= *ba && *ba <= 0x1F){
				// Bell, ����, 7
				if (*ba == 7){
					::Beep(800, 300);
					ba += 1;
					cb -= 1;
				}
				// ɾ��(�˸�), 8
				else if (*ba == '\b'){
					int n = 0;
					while (n < cb && ba[n] == '\b') n++;
					_rich_editor->back_delete_char(n);
					ba += n;
					cb -= n;
				}
				// ˮƽ�Ʊ�, 9
				else if (*ba == '\t'){
					int n = 0;
					char buf[64];
					while (n < cb && n < sizeof(buf) - 1 && ba[n] == '\t'){
						buf[n] = '\t';
						n++;
					}

					buf[n] = '\0';
					_rich_editor->append_text(buf);

					ba += n;
					cb -= n;
				}
				// �س��뻻�� (13,10)
				else if (*ba == '\r' || *ba == '\n'){
					process(_proc_crlf, false, &ba, &cb, &_pre_proc);
				}
				// Linux�ն�, nCursors �����ַ�����
				else if (*ba == '\033'){
					process(_proc_escape, false, &ba, &cb, &_pre_proc);
				}
				// ���� δ������/������ʾ �ַ�����
				else{
					process(_proc_byte, false, &ba, &cb, &_pre_proc);
				}
			}
			// �ո��Ժ��ASCII��׼�ַ�����
			else if (0x20 <= *ba && *ba <= 0x7F){
				process(_proc_ascii, false, &ba, &cb, &_pre_proc);
			}
			// ��չASCII(Extended ASCII / EUC)�ַ�����
			else{
				// ��ǰֻ����GB2312

				// ��gb2312������
				if (0x80 <= *ba && *ba <= 0xA0){
					process(_proc_byte, false, &ba, &cb, &_pre_proc);
				}
				// gb2312������
				else if (0xA1 <= *ba && *ba <= 0xFE){
					process(_proc_gb2312, false, &ba, &cb, &_pre_proc);
				}
				//��gb2312������
				else{
					process(_proc_byte, false, &ba, &cb, &_pre_proc);
				}
			}
		}
	}

	//////////////////////////////////////////////////////////////////////////
	// ���� gb2312
	// ���ֽ�:
	//		01-94λ, ȫ��ʹ��, ��Χ: 0xA1->0xFE
	// ���ֽ�:
	//		01-09��Ϊ�������: ��Χ: 0xA1->0xA9
	//		10-15��: δʹ��, ��Χ: 0xAA->0xAF
	//		16-55��: һ������, ��ƴ������, ��Χ: 0xB0->0xD7
	//		56-87��: ��������, ������/�ʻ�����, ��Χ: 0xD8->0xF7
	//		88-94��: δʹ��, ��Χ: 0xF8->0xFE
	//    
	// �������Ĵ������ȷ�Ա�֤:
	// �����豸����Э���ĳ�ֲ�������, ���ѱ�֤����������ȫ����,
	// ����ڴ����������������ı���ͺ�����ʾ����ȷ��������, �����������ַ�, ���ܵ���һ����
	bool c_gb2312_data_processor::process_some(bool follow, const unsigned char* ba, int cb, int* pn)
	{
		// �Ƿ������һ��δ��Ĵ���?
		if (follow){
			unsigned char chs[16];

			if (*ba >= 0xA1 && *ba <= 0xFE){
				// �б�����
				if (0xA1 <= _lead_byte && _lead_byte <= 0xAF
					|| 0xB0 <= _lead_byte && _lead_byte <= 0xF7)
				{
					chs[0] = _lead_byte;
					chs[1] = *ba;
					chs[2] = '\0';
					_richedit->append_text((const char*)chs);
				}
				// �ޱ�����
				else{
					sprintf((char*)chs, "<A+%02X%02X>", _lead_byte, *ba);
					_richedit->append_text((const char*)chs);
				}


				_lead_byte = 0;
				*pn = 1;
				return false;
			}
			else{
				// �������δ����Ǻ�? 
				// ����1?
				// ���ǰ�_lead_byte�͵�ǰ�ֽ���ʾ��?
				sprintf((char*)chs, "<%02X>", _lead_byte);
				_richedit->append_text((const char*)chs);

				_lead_byte = 0;
				*pn = 0;
				return false;
			}
		}
		// ��ʼ�µĴ���
		else{
			_lead_byte = 0;

			const int kPairs = 512;	// һ����ദ��512�������ַ�
			int npairs = 0;			// �ж��ٶ�������Ҫ����
			char buf[kPairs*2+1];

			// �Ȱ��ܴ���Ĵ����, �����Ľ�����һ�δ���
			while (npairs < kPairs && (npairs + 1) * 2 <= cb){	// �����npairs��ʱҪ����ֽ���, ��0��ʼ, ����3: ��Ҫ����8���ֽ�
				unsigned char b1 = ba[npairs * 2 + 0];
				unsigned char b2 = ba[npairs * 2 + 1];
				if ((0xA1 <= b1 && b1 <= 0xFE) && (0xA1 <= b2 && b2 <= 0xFE)){
					npairs++;
				}
				else{
					break;
				}
			}

			if (npairs){
				// BUG: δ����Ǳ�����
				::memcpy(buf, ba, npairs * 2);
				buf[npairs * 2] = '\0';
				_richedit->append_text(buf);

				*pn = npairs * 2;
				return false;
			}
			else{
				// ֻ����һ���ֽ���������
				// ���ǵ�ǰ��ʣ��һ���ֽ���Ҫ����
				if (cb < 2){ // only can equal to 1
					SMART_ASSERT(cb == 1)(cb).Fatal();
					_lead_byte = *ba;
					*pn = 1;
					return true;
				}
				else{
					sprintf(buf, "<%02X>", ba[0]);
					_richedit->append_text(buf);

					*pn = 1;
					return false;
				}
			}
		}
	}

	void c_gb2312_data_processor::reset_buffer()
	{
		_lead_byte = 0;	//����ǰ��������Ϊ��, ��������������ȫû�����
	}
}

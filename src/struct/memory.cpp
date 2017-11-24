#include "StdAfx.h"
#include "list.h"
#include "../debug.h"

/**********************************************************
�ļ�����:memory.c/memory.h
�ļ�·��:./common/struct/memory.c/.h
����ʱ��:2013-07-14,09:55
�ļ�����:Ů������
�ļ�˵��:�ڴ����
**********************************************************/

static char* __THIS_FILE__ = __FILE__;

/**************************************************
��  ��:get_mem@4
��  ��:�����ڴ�,������
��  ��:size-������Ĵ�С
����ֵ:�ڴ���ָ��
˵  ��:
	2013-07-13:�޸ķ��䷽ʽ,������Գ���
**************************************************/
#pragma pack(push,1)
typedef struct {
	unsigned char sign_head[2];
	char* file;
	int line;
	size_t size;
	list_s entry;
}common_mem_context;
typedef struct {
	unsigned char sign_tail[2];
}common_mem_context_tail;
#pragma pack(pop)

namespace Common{

	c_memory memory;

	void* c_memory::get( size_t size,char* file,int line )
	{
		size_t all = sizeof(common_mem_context)+size+sizeof(common_mem_context_tail);
		void* pv = malloc(all);
		common_mem_context* pc = (common_mem_context*)pv;
		void* user_ptr = (unsigned char*)pc+sizeof(*pc);
		common_mem_context_tail* pct = (common_mem_context_tail*)((unsigned char*)user_ptr+size);
		if(!pv){
			_notifier->msgbox(MB_ICONERROR,NULL,"�ڴ�������");
			return NULL;
		}

		memset(pv,0,all);

		pc->sign_head[0] = 'J';
		pc->sign_head[1] = 'J';

		pc->size = size;
		pc->file = file;
		pc->line = line;

		pct->sign_tail[0] = 'J';
		pct->sign_tail[1] = 'J';

		_insert(&pc->entry);


		return user_ptr;
	}

	void c_memory::free( void** ppv,char* prefix )
	{
		common_mem_context* pc = NULL;
		common_mem_context_tail* pct = NULL;
		if(ppv==NULL || *ppv==NULL)
		{
			_notifier->msgbox(MB_ICONEXCLAMATION,NULL,"memory.free_mem �ͷſ�ָ��, ����:%s",prefix);
			return;
		}

		__try{
			pc = (common_mem_context*)((size_t)*ppv-sizeof(*pc));
			pct = (common_mem_context_tail*)((unsigned char*)pc + sizeof(common_mem_context) + pc->size);

			if((pc->sign_head[0]=='J'&&pc->sign_head[1]=='J') &&
				pct->sign_tail[0]=='J'&&pct->sign_tail[1]=='J')
			{
				_remove(&pc->entry);
				::free(pc);
				*ppv = NULL;			
			}else{
				_remove(&pc->entry);
				_notifier->msgbox(MB_ICONERROR,"debug error",
					"���ͷ��ڴ�ǩ������ȷ!\n\n"
					"�ļ�:%s\n"
					"����:%d",pc->file,pc->line);
			}
		}
		__except(EXCEPTION_EXECUTE_HANDLER){
			_notifier->msgbox(MB_ICONERROR,COMMON_NAME,
				"%s:ָ�뱻������ͷ�,�뱨���쳣!\n\n"
				"�ļ�:%s\n"
				"����:%d",prefix?prefix:"<null-function-name>",pc->file,pc->line);
		}
	}

	c_memory::c_memory()
	{
		list_init(&_head);
		InitializeCriticalSection(&_cs);
	}

	c_memory::~c_memory()
	{
		_delect_leak();
		list_init(&_head);
		DeleteCriticalSection(&_cs);
	}

	void c_memory::set_notifier( i_notifier* pn )
	{
		_notifier = pn;
	}

	void c_memory::_insert( list_s* p )
	{
		::EnterCriticalSection(&_cs);
		list_insert_tail(&_head, p);
		::LeaveCriticalSection(&_cs);
	}

	int c_memory::_remove( list_s* p )
	{
		int r;
		::EnterCriticalSection(&_cs);
		r = list_remove(&_head, p);
		::LeaveCriticalSection(&_cs);
		return r;
	}

	void c_memory::_delect_leak()
	{
		if(!list_is_empty(&_head)){
			int i=1;
#ifdef _DEBUG
			_notifier->msgbox(MB_ICONERROR,NULL,"����δ���ͷŵ��ڴ�!\n\n���������ύ�ڴ������Ϣ~");
#endif
			//����free_mem���Ƴ�������,��������ֻ�ܱ���,�����Ƴ�
			while(!list_is_empty(&_head)){
				list_s* node = _head.next;
				common_mem_context* pc = list_data(node,common_mem_context,entry);
				void* user_ptr = (void*)((unsigned char*)pc+sizeof(*pc));
#ifdef _DEBUG
				_notifier->msgbox(MB_ICONEXCLAMATION,NULL,
					"�ڴ���%d:\n\n"
					"�������ڴ������Ϣ:\n\n"
					"�����С:%u\n"
					"�����ļ�:%s\n"
					"�ļ��к�:%d\n",
					i++,pc->size,pc->file,pc->line);
#endif
				free(&user_ptr,"MANMEM_FREE");
			}
		}
	}

}

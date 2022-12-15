#include "bbscript.h"
#include "arcsys.h"
#include <cctype>
#include <cstring>

#include "sigscan.h"

namespace bbscript {
	//const short *instruction_sizes = (short*)(	sigscan::get().scan("\24\x00\x04\x00\x28\x00", "xxxxxx"));

	using get_func_addr_t = char*(*)(char* bbs_file, char func_name[32], char* table);
	const auto get_func_addr = (get_func_addr_t)(
		sigscan::get().scan("\x8D\x3C\x2E\xD1\xFF", "xxxxx") - 0x40);

	void code_pointer::read_state()
	{
		if (next_op() != opcode::end_state)
		{

		}
	}

	void code_pointer::read_subroutine(char* name)
	{
		char cmn[4];
		memcpy(cmn, ptr + 4, 3);
		cmn[3] = 0;
		for (int i = 0; i < 3; i++)
			cmn[i] = std::tolower(cmn[i]);

		char* subroutine_ptr;
		if (strcmp(cmn, "cmn") == 0)
		{
			subroutine_ptr = reinterpret_cast<char*>(asw_engine::get()) + 2104;
		}
		else
		{
			subroutine_ptr = get_func_addr_base(base_ptr, name);
		}
	}

	void code_pointer::execute_instruction()
	{
		switch (opcode code = *reinterpret_cast<opcode*>(ptr))
		{
		case opcode::call_subroutine:
			{
				char subroutine_name[32];
				memcpy(subroutine_name, ptr + 4, 32);
				read_subroutine(subroutine_name);
			}
		}
	}

	char* code_pointer::get_func_addr_base(char* bbs_file, char* func_name)
	{
		return get_func_addr(bbs_file, func_name, (char*)*bbs_file + 48);
	}
} // bbscript
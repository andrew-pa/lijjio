#include "bo_file.h"


struct bo_header_entry
{
	uint size;
	uint type;
	uint data_offset;
	char name[32];
};

bo_file::bo_file(datablob<byte>* data)
	: _entries(), type(*(uint32*)data->data)
{
	uint32* d = ((uint32*)data->data) + 1;
	uint32* cd = d;
	while (*cd != 0xE22DC0DE && cd < d+data->length) //0xendcode
	{
		bo_header_entry* bhe = (bo_header_entry*)cd;
		_entries.push_back(
			bo_entry(datablob<uint32>(d + bhe->data_offset, bhe->size), 
				bhe->type, string(bhe->name)));
		cd += sizeof(bo_header_entry);
	}
}

void bo_file::save(const string& filename)
{
	throw;
}
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
	uint32 size = 0;
	for (auto e : _entries)
		size += e.data.length;
	uint32* d = new uint32[size];
	d[0] = type;
	uint32* cd = d+1;
	uint32 next_data_offset = (_entries.size()*sizeof(bo_header_entry))+1;
	for (auto e : _entries)
	{
		bo_header_entry* he = (bo_header_entry*)cd;
		he->size = e.data.length;
		he->type = e.type;
		memcpy(he->name, e.name.c_str(), 32);
		he->data_offset = next_data_offset;
		memcpy(d + next_data_offset, e.data.data, e.data.length);
		next_data_offset += e.data.length;
		cd += sizeof(bo_header_entry);
	}
	*cd = 0xE22DC0DE;
	FILE* f = fopen(filename.c_str(), "wb");
	fwrite(d, sizeof(uint32), size, f);
	fclose(f);
}
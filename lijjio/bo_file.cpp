#include "bo_file.h"


struct bo_header_entry
{
	uint size;
	uint type;
	uint data_offset;
	char name[16];
};

bo_file::bo_file(datablob<byte>* data)
	: _entries()
{
	uint32* d = ((uint32*)data->data);
	uint32* cd = d;
	while (*cd != 0xE22DC0DE && cd < d+data->length) //0xendcode is the end of the header
	{
		bo_header_entry* bhe = (bo_header_entry*)cd;
		_entries.push_back(
			bo_entry(_strange_datablob<uint32>(d + bhe->data_offset, bhe->size), 
				bhe->type, string(bhe->name)));
		cd += sizeof(bo_header_entry);
	}
}

void bo_file::save(const string& filename)
{
	bo_header_entry* header = new bo_header_entry[_entries.size()];
	int i = 0;
	uint ndo = sizeof(bo_header_entry)*_entries.size();
	for (auto& e : _entries)
	{
		header[i].size = e.data.length;
		header[i].type = e.type;
		memcpy(header[i].name, e.name.c_str(), 16);
		header[i].data_offset = ndo;
		ndo += e.data.length*sizeof(uint32);
		i++;
	}
	FILE* f = fopen(filename.c_str(), "wb");
	fwrite(header, sizeof(bo_header_entry), _entries.size(), f);
	for (int k = 0; k < _entries.size(); ++k)
	{
		fseek(f, header[k].data_offset, 0);
		fwrite(_entries[k].data.data, sizeof(uint32), _entries[k].data.length, f);
	}
	fclose(f);
	delete[] header;
}
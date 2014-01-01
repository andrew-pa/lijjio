#include "bo_file.h"


struct bo_header_entry
{
	//size in bytes!
	uint size; 
	uint type;
	uint data_offset;
	char name[18];
};

bo_file::bo_file(datablob<byte>* data)
	: _entries()
{
	uint32* dat = (uint32*)data->data;
	uint32 entry_count = dat[0]; //read entry count
	dat += 1;	//move dat ptr forward to account for entry count
	uint32* data_ptr = dat + ((entry_count * sizeof(bo_header_entry))/sizeof(uint32)); //data_ptr points to data section of file
	bo_header_entry* her = (bo_header_entry*)dat;
	
	for (uint i = 0; i < entry_count; ++i)
	{
		_entries.push_back(bo_entry(data_ptr + her[i].data_offset, her[i].size, her[i].type, her[i].name));
	}
}

void bo_file::save(const string& filename)
{
	//size in bytes!
	size_t size = 1 + _entries.size()*sizeof(bo_header_entry);
	for (auto& e : _entries)
		size += e.data.length*sizeof(uint32);
	uint32* dat = new uint32[size / sizeof(uint32)];
	dat[0] = _entries.size(); //write entry count
	uint32* data_ptr = dat + 1 + ((_entries.size() * sizeof(bo_header_entry)) / sizeof(uint32));
	bo_header_entry* her = (bo_header_entry*)(dat + 1);
	int i = 0;
	uint32 dofs = 0;
	for (auto& e : _entries)
	{
		her[i].size = e.data.length*sizeof(uint32);
		her[i].type = e.type;
		memcpy(her[i].name, e.name.c_str(), 16);
		her[i].name[17] = '\0';
		her[i].data_offset = dofs;
		memcpy(data_ptr + dofs, e.data.data, her[i].size);
		dofs += her[i].size;
	}
	FILE* f = fopen(filename.c_str(), "wb");
	fwrite(dat, sizeof(byte), size, f);
	fclose(f);
	delete[] dat;
}
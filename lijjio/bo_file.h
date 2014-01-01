#pragma once
#include <helper.h>
using namespace aldx;

//bo_entry
// entry in a BO file
struct bo_entry
{
	datablob<uint32> data;
	uint type;
	string name;
	bo_entry(datablob<uint32> _data, uint _type, const string& _name)
		: data(_data), type(_type), name(_name){}
	bo_entry(uint32* d, uint32 l, uint _t, const string& n)
		: data(new uint32[l], l), type(_t), name(n) 
	{
		memcpy(data.data, d, l * sizeof(uint32));
	}
};

//bo_file
// a BO file
class bo_file
{
protected:
	vector<bo_entry> _entries;
public:
	bo_file()
		: _entries() {}
	//loads BO from the data in data. you can delete data when it finishes
	bo_file(datablob<byte>* data);

	inline datablob<uint32>& operator[](const string& name)
	{
		auto v = find_if(_entries.begin(), _entries.end(), [&](bo_entry b) 
		{
			return b.name == name;
		});
		if (v != _entries.end())
			return v->data;
		else
			throw exception(("can't find " + name + " in file").c_str());
	}
	inline const datablob<uint32>& operator[](const string& name) const
	{
		auto v = find_if(_entries.begin(), _entries.end(), [&](bo_entry b) 
		{
			return b.name == name;
		});
		if (v != _entries.end())
			return v->data;
		else
			throw exception(("can't find " + name + " in file").c_str());
	}

	proprw(vector<bo_entry>, entries, { return _entries; })

	void save(const string& filename);
};


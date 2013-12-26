#pragma once
#include <helper.h>
using namespace aldx;

struct bo_entry
{
	datablob<uint32> data;
	uint type;
	string name;
	bo_entry(datablob<uint32>& _data, uint _type, const string& _name)
		: data(_data), type(_type), name(_name){}
};

class bo_file
{
protected:
	vector<bo_entry> _entries;
	uint type;
public:
	bo_file(datablob<byte>* data);

	inline datablob<byte>& operator[](const string& name)
	{
		auto v = find_if(_entries.begin(), _entries.end(), [&](bo_entry b) {});
		if (v != _entries.end())
			return v->data;
		else
			throw exception(("can't find " + name + " in file").c_str());
	}
	inline const datablob<byte>& operator[](const string& name) const
	{
		auto v = find_if(_entries.begin(), _entries.end(), [&](bo_entry b) {});
		if (v != _entries.end())
			return v->data;
		else
			throw exception(("can't find " + name + " in file").c_str());
	}

	proprw(vector<bo_entry>, entries, { return _entries; })

	void save(const string& filename);
};


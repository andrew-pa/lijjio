#include <map>
#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <DirectXMath.h>
using namespace DirectX;
using namespace std;

typedef uint32_t uint;

struct vertex
{
	XMFLOAT3 pos, norm;
	XMFLOAT2 texc;
};

bool operator ==(vertex a, vertex b)
{
	return a.pos.x == b.pos.x && a.pos.y == b.pos.y && a.pos.z == b.pos.z;
}

struct bo_data
{
	void* data;
	uint size;
	uint type;
	bo_data(void* d, uint s, uint t)
		: data(d), size(s), type(t) {} 
	bo_data()
		: data(nullptr), size(-1), type(-1) { }
};

typedef map<string, bo_data> bo_map;

struct bo_file
{
	bo_map* data;
	uint type;
	bo_file(bo_map* d, uint t)
		: data(d), type(t) { }
};

struct bo_object
{
	char name[16];
	uint data_offset;
	uint size;
	uint type;
};

struct bo_header
{
	uint number_of_objects;
	uint file_type;
	bo_object objects[1024];
};


struct mesh
{
	vector<vertex> vertices;
	vector<uint32_t> indices;

	size_t size()
	{
		return 2*sizeof(uint)+(vertices.size()*sizeof(vertex)) + (indices.size()*sizeof(uint));
	}

	void* mesb_rep()
	{
		size_t s = size();
		char* bd = new char[s];
		uint* d = (uint*)bd;
		d[0] = vertices.size();
		d[1] = indices.size();
		memcpy(bd+2*sizeof(uint), &vertices[0], vertices.size()*sizeof(vertex));
		memcpy(bd+2*sizeof(uint)+(vertices.size()*sizeof(vertex)),
			&indices[0], indices.size()*sizeof(uint));
		return bd;
	}
};

int main(int argc, char* argv[])
{
	cout << "obj2bo";
	vector<XMFLOAT3> poss;
	vector<XMFLOAT3> norms;
	vector<XMFLOAT2> texcs;

	ifstream inf(argv[1]);
	char comm[256] = {0};
	
	cout << " [" << argv[1] << " > " << argv[2] << "]\n";

	string curr_objname;
	map<string, mesh> objects;

#pragma region parseobjfile
	while(inf)
	{
		inf >> comm;
		if(!inf) break;
		if(strcmp(comm, "#")==0)
		{
			string comment;
			getline(inf, comment);
			if(comment.length() == 0)
				continue;
			cout << "Comment: " << comment << "\n";
		}
		else if(strcmp(comm, "v")==0)
		{
			float x, y, z;
			inf >> x >> y >> z;
			poss.push_back(XMFLOAT3(x, y, z));
		}
		else if(strcmp(comm, "vn")==0)
		{
			float x, y, z;
			inf >> x >> y >> z;
			norms.push_back(XMFLOAT3(x, y, z));
		}
		else if(strcmp(comm, "vt")==0)
		{
			float x, y;
			inf >> x >> y;
			texcs.push_back(XMFLOAT2(x, y));
		}
		else if(strcmp(comm, "o")==0 || strcmp(comm, "g")==0)
		{
			inf >> curr_objname;
			//if(curr_objname.size() % 16 != 0)
			//{
			//	while((curr_objname.size() % 16) != 0)
			//	{
			//		curr_objname += '\0';
			//	}
			//}
			objects[curr_objname] = mesh();
			cout << "Object " << curr_objname << "\n";
		}
		else if(strcmp(comm, "f")==0)
		{
			for(uint32_t ifc = 0; ifc < 3; ++ifc)
			{
				vertex v;
				uint32_t ip, it, in;
				inf >> ip;
				v.pos = poss[ip-1];
				if('/' == inf.peek())
				{
					inf.ignore();
					if('/' != inf.peek())
					{
						inf >> it;
						v.texc = texcs[it - 1];
					}
					if('/' == inf.peek())
					{
						inf.ignore();
						inf >> in;
						v.norm = norms[in - 1];
					}
				}
				auto iv = find(objects[curr_objname].vertices.begin(), objects[curr_objname].vertices.end(), v);
				if(iv == objects[curr_objname].vertices.end()) //unique vertex
				{
					objects[curr_objname].vertices.push_back(v);
					objects[curr_objname].indices.push_back(objects[curr_objname].vertices.size()-1);
				}
				else //already got this vertex
				{
					objects[curr_objname].indices.push_back(iv - objects[curr_objname].vertices.begin());
				}
			}
		}
	}
#pragma endregion

	//each obj object is mapped to a bo object
	//each one is much like a simple mesb object
	size_t acsiz = sizeof(uint)* 2 + sizeof(bo_object)*objects.size(); //calulate actual size of the bo header vs. what the compiler thinks the size is
	char* actual_header_data = new char[acsiz];
	bo_header* boh = (bo_header*)actual_header_data;
	boh->file_type = 8; //BO Std File type - Mesh Data
	boh->number_of_objects = objects.size();
	//if(objects.size() > 1024) //max bo objects
	//{
	//	cerr << "Number of objects in mesh greater than max number of objects a BO file can hold";
	//	return -1;
	//}
	
	int bo_objidx = 0;

	const uint dt_size = objects.size() * sizeof(bo_object) + sizeof(uint)*2;
	//uint next_object_ptr = dt_size;

	FILE* f = fopen(argv[2], "wb");
	fwrite(boh, dt_size, 1, f);
	
	for(auto o = objects.begin(); o != objects.end(); ++o)
	{
		memcpy(boh->objects[bo_objidx].name, o->first.c_str(), 16);
		boh->objects[bo_objidx].size = o->second.size();
		boh->objects[bo_objidx].type = 0; //object
		boh->objects[bo_objidx].data_offset = ftell(f);
		fwrite(o->second.mesb_rep(), o->second.size(), 1, f);

		//next_object_ptr = ftell(f);
		bo_objidx++;
	}

	fseek(f, 0, SEEK_SET);
	fwrite(boh, dt_size, 1, f);

	fclose(f);
	return 0;
}
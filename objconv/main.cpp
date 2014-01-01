#include <helper.h>


//#define OLD_BO
#ifndef OLD_BO
#include "../lijjio/bo_file.h"
#else
#include <bofile.h>
#endif

using namespace aldx;

struct vertex
{
	float3 pos, norm;
	float2 texc;
};
bool operator ==(vertex a, vertex b)
{
	return a.pos.x == b.pos.x 
		&& a.pos.y == b.pos.y 
		&& a.pos.z == b.pos.z
		&& a.norm.x == b.norm.x 
		&& a.norm.y == b.norm.y
		&& a.norm.z == b.norm.z
		&& a.texc.x == b.texc.x
		&& a.texc.y == b.texc.y;
}

struct mesh
{
	vector<vertex> vertices;
	vector<uint32_t> indices;

	size_t size()
	{
		return 2 * sizeof(uint)+(vertices.size()*sizeof(vertex)) + (indices.size()*sizeof(uint));
	}

	void* mesb_rep()
	{
		size_t s = size();
		char* bd = new char[s];
		uint* d = (uint*)bd;
		d[0] = vertices.size();
		d[1] = indices.size();
		memcpy(bd + 2 * sizeof(uint), &vertices[0], vertices.size()*sizeof(vertex));
		memcpy(bd + 2 * sizeof(uint)+(vertices.size()*sizeof(vertex)),
			&indices[0], indices.size()*sizeof(uint));
		return bd;
	}
};


int main(int argc, char* argv[])
{
	bo_file f = bo_file();
	char* data = new char[2 * sizeof(uint32)];
	for (int i = 0; i < 7; ++i)
		data[i] = 65 + (char)i;
	data[7] = '\0';
	f.entries().push_back(bo_entry((uint32*)data, 2, 24, "hello_data"));
	f.save("hello.bo");
	return 5125;

	if (argc < 3)
	{
		cout << "objconv usage: " << endl;
		cout << "objconv name_of_obj_file name_of_bo_file" << endl;
		for (int i = 1; i < argc; ++i)
		{
			cout << "invalid arg: " << argv[i] << endl;
		}
		return -1;
	}
	cout << "objconv " << argv[1] << " >>> " << argv[2] << endl;

#pragma region parse obj file
	map<string, mesh> objects;
	{
		vector<float3> positions, normals;
		vector<float2> texturecoords;
		ifstream inf(argv[1]);
		string com;
		string current_object;

		while (inf)
		{
			inf >> com;
			if (!inf) break;
			if (com == "#")
			{
				string comment;
				getline(inf, comment);
				if (comment.length() == 0)
					continue;
				cout << "# " << comment << endl;
			}
			else if (com == "v")
			{
				float x, y, z;
				inf >> x >> y >> z;
				positions.push_back(float3(x, y, z));
			}
			else if (com == "vn")
			{
				float x, y, z;
				inf >> x >> y >> z;
				normals.push_back(float3(x, y, z));
			}
			else if (com == "vt")
			{
				float x, y;
				inf >> x >> y;
				texturecoords.push_back(float2(x, y));
			}
			else if (com == "o" || com == "g")
			{
				inf >> current_object;
				objects[current_object] = mesh();
				cout << "object " << current_object << endl;
			}
			else if (com == "f")
			{
				for (uint vc = 0; vc < 3; ++vc)
				{
					vertex v;
					uint ip, it, in;
					inf >> ip;
					v.pos = positions[ip - 1];
					if ('/' == inf.peek())
					{
						inf.ignore();
						if ('/' != inf.peek())
						{
							inf >> it;
							v.texc = texturecoords[it - 1];
						}
						if ('/' == inf.peek())
						{
							inf.ignore();
							inf >> in;
							v.norm = normals[in - 1];
						}
					}
					auto idx = find(objects[current_object].vertices.begin(), objects[current_object].vertices.end(), v);
					if (idx == objects[current_object].vertices.end())
					{
						objects[current_object].indices.push_back(objects[current_object].vertices.size());
						objects[current_object].vertices.push_back(v);
					}
					else
					{
						objects[current_object].indices.push_back(idx - objects[current_object].vertices.begin());
					}
				}
			}
		}
	}
#pragma endregion
#ifdef OLD_BO
	{
		//each obj object is mapped to a bo object
		//each one is much like a simple mesb object
		bo_header* boh = new bo_header();
		boh->file_type = 8; //BO Std File type - Mesh Data
		boh->number_of_objects = objects.size();
		if (objects.size() > 1024) //max bo objects
		{
			cerr << "Number of objects in mesh greater than max number of objects a BO file can hold";
			return -1;
		}

		int bo_objidx = 0;

		const uint dt_size = objects.size() * sizeof(bo_object)+sizeof(uint)* 2;
		//uint next_object_ptr = dt_size;

		FILE* f = fopen(argv[2], "wb");
		fwrite(boh, dt_size, 1, f);

		for (auto o = objects.begin(); o != objects.end(); ++o)
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
#else

	//bo_file bf{ 8 };
	//for (auto o : objects)
	//{
	//	bf.entries().push_back(bo_entry(
	//		(uint32*)o.second.mesb_rep(), o.second.size(),
	//		0, o.first));
	//}
	//bf.save(argv[2]);
	return 0;
#endif
}
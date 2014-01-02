#include "mesh_load_from_obj.h"

bool operator ==(dvertex a, dvertex b)
{
	return a.pos.x == b.pos.x && a.pos.y == b.pos.y && a.pos.z == b.pos.z &&
		   a.norm.x == b.norm.x && a.norm.y == b.norm.y && a.norm.z == b.norm.z &&
		   a.texc.x == b.texc.x && a.texc.y == b.texc.y;
}

struct obj_mesh
{
	vector<dvertex> vertices;
	vector<uint> indices;
};

model* model_load_from_obj(ComPtr<ID3D11Device> device, const string& fn)
{
	vector<float3> poss;
	vector<float3> norms;
	vector<float2> texcs;

	ifstream inf(fn);
	char comm[256] = { 0 };

	string curr_objname;
	map<string, obj_mesh> objects;

#pragma region parseobjfile
	while (inf)
	{
		inf >> comm;
		if (!inf) break;
		if (strcmp(comm, "#") == 0)
		{
			string comment;
			getline(inf, comment);
			if (comment.length() == 0)
				continue;
			cout << "Comment: " << comment << "\n";
		}
		else if (strcmp(comm, "v") == 0)
		{
			float x, y, z;
			inf >> x >> y >> z;
			poss.push_back(float3(x, y, z));
		}
		else if (strcmp(comm, "vn") == 0)
		{
			float x, y, z;
			inf >> x >> y >> z;
			norms.push_back(float3(x, y, z));
		}
		else if (strcmp(comm, "vt") == 0)
		{
			float x, y;
			inf >> x >> y;
			texcs.push_back(float2(x, y));
		}
		else if (strcmp(comm, "o") == 0 || strcmp(comm, "g") == 0)
		{
			inf >> curr_objname;
			//if(curr_objname.size() % 16 != 0)
			//{
			//	while((curr_objname.size() % 16) != 0)
			//	{
			//		curr_objname += '\0';
			//	}
			//}
			objects[curr_objname] = obj_mesh();
			cout << "Object " << curr_objname << "\n";
		}
		else if (strcmp(comm, "f") == 0)
		{
			for (uint32_t ifc = 0; ifc < 3; ++ifc)
			{
				dvertex v;
				uint32_t ip, it, in;
				inf >> ip;
				v.pos = poss[ip - 1];
				if ('/' == inf.peek())
				{
					inf.ignore();
					if ('/' != inf.peek())
					{
						inf >> it;
						v.texc = texcs[it - 1];
					}
					if ('/' == inf.peek())
					{
						inf.ignore();
						inf >> in;
						v.norm = norms[in - 1];
					}
				}
				auto iv = find_if(objects[curr_objname].vertices.begin(), objects[curr_objname].vertices.end(), [&] (dvertex ov) 
				{
					return v.pos.x == ov.pos.x && v.pos.y == ov.pos.y && v.pos.z == ov.pos.z &&
						v.norm.x == ov.norm.x && v.norm.y == ov.norm.y && v.norm.z == ov.norm.z &&
						v.texc.x == ov.texc.x && v.texc.y == ov.texc.y;
				});
				if (iv == objects[curr_objname].vertices.end()) //unique vertex
				{
					objects[curr_objname].vertices.push_back(v);
					objects[curr_objname].indices.push_back(objects[curr_objname].vertices.size() - 1);
				}
				else //already got this vertex
				{
					objects[curr_objname].indices.push_back(iv - objects[curr_objname].vertices.begin());
				}
			}
		}
	}
#pragma endregion

	vector<mesh*> meshes;
	vector<float4x4> worlds;
	for (auto& m : objects)
	{
		meshes.push_back(new mesh(device, &m.second.vertices[0], &m.second.indices[0], m.second.indices.size(),
			m.second.vertices.size(), sizeof(dvertex), m.first));
		worlds.push_back(float4x4::identity());
	}
	return new model(meshes, worlds);
}
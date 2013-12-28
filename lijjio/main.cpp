#define LINK_DIRECTX
#include <helper.h>
#include <dx_app.h>
#include <shader.h>
#include <render_shader.h>
#include <constant_buffer.h>
#include <mesh.h>
#include <model.h>
#include <bofile.h>
#include <camera.h>
#include <texture2d.h>
#include <states.h>
//#include "bo_file.h"
using namespace aldx;


static const D3D11_INPUT_ELEMENT_DESC posnormtex_layout[] =
{
	{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
	{ "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
	{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
};

struct basic_material
{
	float4 dif;
	float3 spec;
	float spec_exp;
	float3 amb; bool alpha_clip;
	basic_material() { }
	basic_material(float4 d, float3 s, float sx, float3 a, bool c)
		: dif(d), spec(s), spec_exp(sx), amb(a), alpha_clip(c)
	{
	}
};

struct point_light
{
	float4 pos;
	float4 col;
};

struct dir_light
{
	float4 dir;
	float4 col;
};

class basic_shader : public render_shader
{
	struct model_dep
	{
		float4x4 world;
		float4x4 inv_world;
	};
	struct cam_dep
	{
		float4x4 view;
		float4x4 proj;
		float3 cam_pos;
		float extra;
	};
	struct lights_cb
	{
		point_light point_lights[8];
		dir_light dir_lights[8];
		int num_point_lights;
		int num_dir_lights;
		float2 extra;
	};
	struct object_cb
	{
		basic_material m;
	};

	constant_buffer<model_dep> model_depcb;
	constant_buffer<cam_dep> cam_depcb;
	constant_buffer<lights_cb> light_cb;
	constant_buffer<object_cb> obj_cb;
	sampler_state samp;
	texture2d* tex;
public:
	basic_shader(){}
	basic_shader(ComPtr<ID3D11Device> device, datablob<byte>* vs_data, datablob<byte>* ps_data)
		: render_shader(device, vs_data, ps_data, posnormtex_layout, _countof(posnormtex_layout)),
		model_depcb(device, 0, model_dep()), cam_depcb(device, 1, cam_dep()), 
		light_cb(device, 0, lights_cb()), obj_cb(device, 2, object_cb()),
		samp(device, 0)
	{
	}

	inline void world(const float4x4& m) override
	{
		model_depcb.data().world = m;
		model_depcb.data().inv_world = inverse(m);
	}

	inline void camera_position(const float3& p)
	{
		cam_depcb.data().cam_pos = p;
	}

	inline void view(const float4x4& m) override
	{
		cam_depcb.data().view = m;
	}

	inline void proj(const float4x4& m) override
	{
		cam_depcb.data().proj = m;
	}

	inline point_light& point_light(int idx)
	{
		return light_cb.data().point_lights[idx];
	}
	inline dir_light& dir_light(int idx)
	{
		return light_cb.data().dir_lights[idx];
	}
	inline void set_num_point_lights(int num)
	{
		light_cb.data().num_point_lights = num;
	}
	inline void set_num_dir_lights(int num)
	{
		light_cb.data().num_dir_lights = num;
	}

	inline void set_material(basic_material& m)
	{
		obj_cb.data().m = m;
	}

	inline void set_texture(texture2d* t) override
	{
		tex = t;
	}
	proprw(texture2d*, texture, { return tex; })

	inline void bind(ComPtr<ID3D11DeviceContext> context) override
	{
		render_shader::bind(context);
		model_depcb.bind(context, shader_stage::Vertex);
		cam_depcb.bind(context, shader_stage::Vertex);
		cam_depcb.bind(context, shader_stage::Pixel);
		light_cb.bind(context, shader_stage::Pixel);
		obj_cb.bind(context, shader_stage::Pixel);
		samp.bind(context, shader_stage::Pixel);
	}
	inline void unbind(ComPtr<ID3D11DeviceContext> context) override
	{
		render_shader::unbind(context);
		model_depcb.unbind(context, shader_stage::Vertex);
		cam_depcb.unbind(context, shader_stage::Vertex);
		cam_depcb.unbind(context, shader_stage::Pixel);
		light_cb.unbind(context, shader_stage::Pixel);
		obj_cb.unbind(context, shader_stage::Pixel);
		samp.unbind(context, shader_stage::Pixel);
	}
	inline void update(ComPtr<ID3D11DeviceContext> context) override
	{
		model_depcb.update(context);
		cam_depcb.update(context);
		light_cb.update(context);
		obj_cb.update(context);
		if(tex) tex->bind(context, shader_stage::Pixel);
	}
};

class game_object
{
	model* _m;
	float4x4 w;
	basic_material mm;
	texture2d* _tex;
	float3 pos, rot, scl;
public:
	proprw(model*, mmodel, { return _m; })
		propr(float4x4, world, { return w; })
		proprw(float3, position, { return pos; })
		proprw(float3, rotation, { return rot; })
		proprw(float3, scale, { return scl; })
		proprw(basic_material, mmaterial, { return mm; })
		proprw(texture2d*, texture, { return _tex; })

		game_object(model* model_, texture2d* tex, float3 pos_ = float3(), float3 rot_ = float3(), float3 scl_ = float3(1),
			basic_material mm_ = basic_material(float4(.8f, .8f, .8f, 1.f), float3(.8f, .8f, .8f), 0, float3(.3f, .3f, .3f), false))
		: _m(model_), _tex(tex), pos(pos_), rot(rot_), scl(scl_), mm(mm_)
	{
	}

	virtual void update(float t, float dt)
	{
		w = XMMatrixScalingFromVector(scl) * XMMatrixRotationRollPitchYawFromVector(rot) * XMMatrixTranslationFromVector(pos);
	}

	virtual void draw(ComPtr<ID3D11DeviceContext> context, basic_shader& shd)
	{
		shd.set_material(mm);
		shd.set_texture(_tex);
		for (int i = 0; i < _m->meshes().size(); ++i)
		{
			shd.world(w * _m->worlds()[i]);
			shd.update(context);
			_m->meshes()[i]->draw(context);
		}
	}
};

class lijjio_app : public dx_app
{
	vector<game_object*> gameobjects;
	camera cam;
	basic_shader shd;
public:
	lijjio_app()
		: dx_app(4, true),
		cam(float3(0, 4, -5), float3(0, 0.1f, 0), 0.1f, 1000.f, to_radians(45.f))
	{}

	void load() override
	{
		/*bo_file knot_mesh(read_data_from_package(L"cube.bo"));
		auto tkd = knot_mesh["Cube"];
		void* vertices = tkd.data + 2;
		void* indices = tkd.data + (tkd.data[0] * sizeof(dvertex)) + (2*sizeof(uint));
		uint vc = tkd.data[0];
		uint ic = tkd.data[1];
		knot = mesh(device, vertices,
			indices, ic, vc, sizeof(dvertex), "cube");
		*/
		/*obk = model(mesh::create_sphere(device, 1.f, 32, 32));
		grnd = model(mesh::create_grid(device, 16, 16, 4, 4));
		*/
		
		gameobjects.push_back(new game_object(new model(mesh::create_sphere(device, 1.f, 32, 32)), new texture2d(device, read_data_from_package(L"stone.dds")), float3(0, 2, 0)));
		gameobjects.push_back(new game_object(new model(mesh::create_grid(device, 16, 16, 4, 4)), new texture2d(device, read_data_from_package(L"floor.dds")), float3(), float3(), float3(1),
			basic_material(float4(.8f, .8f, .8f,1), float3(.8f, .8f, .8f), 32, float3(.2f, .2f, .2f), false)));

		shd = basic_shader(device, read_data_from_package(L"basic_vs.cso"), read_data_from_package(L"basic_ps.cso"));
		
		shd.set_num_dir_lights(0);
		shd.set_num_point_lights(2);
		shd.point_light(0).pos = float4(0, 3, 0, 20);
		shd.point_light(0).col = float4(.8f, .8f, .8f, 1);
		shd.point_light(1).pos = float4(0, 3, 0, 10);
		shd.point_light(1).col = float4(.0f, .8f, .8f, 1);
		basic_material knotmat;
		knotmat.dif = float4(.8f, .8f, .8f, 1.f);
		knotmat.spec = float3(.8f, .8f, .8f);
		knotmat.spec_exp = 0;
		knotmat.amb = float3(.3f, .3f, .3f);
		knotmat.alpha_clip = false;
		shd.set_material(knotmat);
	}

	void update(float t, float dt) override
	{
		if (windowSizeChanged)
		{
			cam.update_proj(windowBounds.width / windowBounds.height);
			shd.proj(cam.proj());
			windowSizeChanged = false;
		}

		shd.point_light(0).pos.x = sinf(t) * 3;
		shd.point_light(0).pos.z = cosf(t) * 3;
		shd.point_light(1).pos.x = sinf(t) * -3;
		shd.point_light(1).pos.z = cosf(t) * -3;

		const float spd = 2.75f;
		if (keyboard::key_down('A'))
			cam.strafe(-spd*dt);
		else if (keyboard::key_down('D'))
			cam.strafe(spd*dt);
		if (keyboard::key_down('W'))
			cam.forward(spd*dt);
		else if (keyboard::key_down('S'))
			cam.forward(-spd*dt);
		if (keyboard::key_down('Q'))
			cam.position().y += spd*dt;
		else if (keyboard::key_down('E'))
			cam.position().y -= spd*dt;

		static const float d = XMConvertToRadians(1.f);
		if (keyboard::key_down(VK_LEFT))
			cam.rotate_worldY(-d);
		else if (keyboard::key_down(VK_RIGHT))
			cam.rotate_worldY(d);
		if (keyboard::key_down(VK_UP))
			cam.pitch(-d);
		else if (keyboard::key_down(VK_DOWN))
			cam.pitch(d);

		cam.update_view();
		shd.camera_position(cam.position());
		shd.view(cam.view());

		gameobjects[0]->position().x = sinf(-t) * 2;
		gameobjects[0]->position().z = cosf(t) * 2;

		for (auto& g : gameobjects)
			g->update(t, dt);
	}

	void render(float t, float dt) override
	{
		dx_app::render(t, dt);
		shd.bind(context);

		for (auto& g : gameobjects)
			g->draw(context, shd);

		shd.unbind(context);
	}
};

int CALLBACK WinMain(
	_In_  HINSTANCE inst,
	_In_  HINSTANCE pinst,
	_In_  LPSTR cmdLine,
	_In_  int cmds
	)
{
	lijjio_app app;
	app.run(inst, cmds, L"lijjio", 1280, 960);
}
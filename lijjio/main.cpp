#define LINK_DIRECTX
#include <helper.h>
#include <dx_app.h>
#include <shader.h>
#include <render_shader.h>
#include <constant_buffer.h>
#include <mesh.h>
#include <model.h>
#include <camera.h>
#include <texture2d.h>
#include <states.h>
#include "bo_file.h"
using namespace aldx;


static const D3D11_INPUT_ELEMENT_DESC posnormtex_layout[] =
{
	{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
	{ "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
	{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
};

struct mat
{
	float4 dif;
	float3 spec;
	float spec_exp;
	float3 amb; bool alpha_clip;
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
		mat m;
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

	inline void set_material(mat& m)
	{
		obj_cb.data().m = m;
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

class lijjio_app : public dx_app
{
	model obk;
	camera cam;
	basic_shader shd;
public:
	lijjio_app()
		: dx_app(4, true),
		cam(float3(0, 0, -5), float3(0, 0.1f, 0), 0.1f, 1000.f, to_radians(45.f))
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
		obk = model(device, load_bo(read_data_from_package(L"knot.bo")));
		shd = basic_shader(device, read_data_from_package(L"basic_vs.cso"), read_data_from_package(L"basic_ps.cso"));
		shd.texture() = new texture2d(device, read_data_from_package(L"floor.dds"));
		shd.set_num_dir_lights(0);
		shd.set_num_point_lights(1);
		shd.point_light(0).pos = float4(0, 4, 0, 1);
		shd.point_light(0).col = float4(.8f, .8f, .8f, 1);
		mat knotmat;
		knotmat.dif = float4(.8f, .4f, 0.f, 1.f);
		knotmat.spec = float3(.8f, .8f, .8f);
		knotmat.spec_exp = 64;
		knotmat.amb = float3(.3f, .15f, 0.f);
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
		cam.update_view();
		shd.camera_position(cam.position());
		shd.view(cam.view());
	}

	void render(float t, float dt) override
	{
		dx_app::render(t, dt);
		shd.bind(context);

		shd.world(float4x4::identity());
		shd.update(context);
		knot.draw(context);
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
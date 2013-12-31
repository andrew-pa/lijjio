#pragma once
#include <helper.h>
#include <shader.h>
#include <render_shader.h>
using namespace aldx;

static const D3D11_INPUT_ELEMENT_DESC posnormtex_layout[] =
{
	{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
	{ "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
	{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
};

struct basic_material : public material
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

	size_t get_size() 
	{
		return sizeof(basic_material);
	}
};

struct point_light
{
	float4 pos;
	float4 col;
	point_light() { }
	point_light(float4 p, float4 c)
		: pos(p), col(c)
	{
	}
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
		float4 dif;
		float3 spec;
		float spec_exp;
		float3 amb; bool alpha_clip;
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
		obj_cb.data().dif = m.dif;
		obj_cb.data().spec = m.spec;
		obj_cb.data().spec_exp = m.spec_exp;
		obj_cb.data().amb = m.amb;
		obj_cb.data().alpha_clip = m.alpha_clip;
	}
	inline void set_material(const material* mat) override
	{
		auto m = dynamic_cast<const basic_material*>(mat);
		obj_cb.data().dif = m->dif;
		obj_cb.data().spec = m->spec;
		obj_cb.data().spec_exp = m->spec_exp;
		obj_cb.data().amb = m->amb;
		obj_cb.data().alpha_clip = m->alpha_clip;
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
		if (tex) tex->bind(context, shader_stage::Pixel);
	}
};


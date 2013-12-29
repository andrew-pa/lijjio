#pragma once
#include <helper.h>
#include <shader.h>
#include <render_shader.h>
#include <constant_buffer.h>
#include <mesh.h>
#include <model.h>
#include <bofile.h>
#include <camera.h>
#include <texture2d.h>
#include <states.h>
#include <render_texture.h>
//#include "bo_file.h"
using namespace aldx;

#include "basic_shader.h"
#include "game_object.h"

class deferred_renderer : public render_shader
{
public:
	struct renderpass
	{
		shader* s;
		render_texture* rt;
		renderpass(shader* _s, render_texture* _rt)
			: s(_s), rt(_rt){}
	};
	typedef function<void(ComPtr<ID3D11DeviceContext>, render_shader&)> draw_scene_func;
protected:
	draw_scene_func _draw_scene;

	vector<renderpass> _rps;
	shader* _current_shader;

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
	struct object_cb
	{
		float4 dif;
		float3 spec;
		float spec_exp;
		float3 amb; bool alpha_clip;
	};

	constant_buffer<model_dep> model_depcb;
	constant_buffer<cam_dep> cam_depcb;
	constant_buffer<object_cb> obj_cb;
	constant_buffer<point_light> light_cb;
	sampler_state samp;
	texture2d* tex;
public:
#pragma region render shader impl
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
			_current_shader->bind(context);
			model_depcb.bind(context, shader_stage::Vertex);
			cam_depcb.bind(context, shader_stage::Vertex);
			cam_depcb.bind(context, shader_stage::Pixel, 0);
			obj_cb.bind(context, shader_stage::Pixel);
			samp.bind(context, shader_stage::Pixel);
	}
	inline void unbind(ComPtr<ID3D11DeviceContext> context) override
	{
		_current_shader->unbind(context);
		model_depcb.unbind(context, shader_stage::Vertex);
		cam_depcb.unbind(context, shader_stage::Vertex);
		cam_depcb.unbind(context, shader_stage::Pixel, 0);
		obj_cb.unbind(context, shader_stage::Pixel);
		samp.unbind(context, shader_stage::Pixel);
	}
	inline void update(ComPtr<ID3D11DeviceContext> context) override
	{
		_current_shader->update(context);
		model_depcb.update(context);
		cam_depcb.update(context);
		obj_cb.update(context);
		if (tex) tex->bind(context, shader_stage::Pixel);
	}
#pragma endregion
	deferred_renderer() {}
	deferred_renderer(ComPtr<ID3D11Device> device, 
		draw_scene_func drawscene, 
		vector<renderpass>& passes_)
		: model_depcb(device, 0, model_dep()), cam_depcb(device, 1, cam_dep()),
		obj_cb(device, 1, object_cb()), samp(device, 0), _draw_scene(drawscene), _rps(passes_)
	{
	}

	proprw(vector<renderpass>, render_passes, { return _rps; });
	proprw(shader*, current_shader, { return _current_shader; });
	proprw(function<void(ComPtr<ID3D11DeviceContext>, render_shader&)>, draw_scene, { return _draw_scene; });

	void render(ComPtr<ID3D11DeviceContext> context, render_target_stack* rts)
	{
		for (auto& rp : _rps)
		{
			rp.rt->push(rts);
			_current_shader = rp.s;
			_draw_scene(context, *this);
			rts->pop_render_target();
		}
	}
};
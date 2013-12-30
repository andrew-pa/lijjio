
#pragma once
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
#include <render_texture.h>
//#include "bo_file.h"
using namespace aldx;

#include "basic_shader.h"
#include "game_object.h"
#include "deferred_renderer.h"

mesh* create_ndc_quad(ComPtr<ID3D11Device> device, float r = 1.0f, float z = 0.f)
{
	dvertex v[] =
	{
		dvertex(-r, -r, z, 0, 0, -r, r, 0, 0, 0, r),
		dvertex(-r, r, z, 0, 0, -r, 0, r, 0, 0, 0),
		dvertex(r, r, z, 0, 0, -r, r, 0, 0, r, 0),
		dvertex(r, -r, z, 0, 0, -r, r, 0, 0, r, r),
	};
	uint i[] =
	{
		0, 1, 2, 0, 2, 3,
	};
	return new mesh(device, v, i, 6, 4, sizeof(dvertex), "NDC_QUAD");
}

class simple_shader : public render_shader
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
	sampler_state samp;
	texture2d* tex;
public:
	simple_shader(){}
	simple_shader(ComPtr<ID3D11Device> device, datablob<byte>* vs_data, datablob<byte>* ps_data)
		: render_shader(device, vs_data, ps_data, posnormtex_layout, _countof(posnormtex_layout)),
		model_depcb(device, 0, model_dep()), cam_depcb(device, 1, cam_dep()),  obj_cb(device, 1, object_cb()),
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
			cam_depcb.bind(context, shader_stage::Pixel, 0);
			obj_cb.bind(context, shader_stage::Pixel);
			samp.bind(context, shader_stage::Pixel);
		}
	inline void unbind(ComPtr<ID3D11DeviceContext> context) override
	{
		render_shader::unbind(context);
		model_depcb.unbind(context, shader_stage::Vertex);
		cam_depcb.unbind(context, shader_stage::Vertex);
		cam_depcb.unbind(context, shader_stage::Pixel, 0);
		obj_cb.unbind(context, shader_stage::Pixel);
		samp.unbind(context, shader_stage::Pixel);
	}
	inline void update(ComPtr<ID3D11DeviceContext> context) override
	{
		model_depcb.update(context);
		cam_depcb.update(context);
		obj_cb.update(context);
		if (tex) tex->bind(context, shader_stage::Pixel);
	}
};




class df_lijjio_app : public dx_app, public render_shader
{
	vector<game_object*> gameobjects;
	camera cam;

	
	shader normals_shader;
	shader diffuse_shader;
	shader positions_shader;
	shader spec_shader;

	shader* current_shader;

	render_texture normals_buffer;
	render_texture diffuse_buffer;
	render_texture positions_buffer;
	render_texture spec_buffer;

	shader pntlight_shader;

	mesh* quad;
	mesh* quad2;

	deferred_renderer* dr;


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

#pragma region render_shader impl
public:
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

	texture2d* tex;
	inline void set_texture(texture2d* t) override
	{
		tex = t;
	}
	proprw(texture2d*, texture, { return tex; })

	inline void bind(ComPtr<ID3D11DeviceContext> context) override
	{
			current_shader->bind(context);
			model_depcb.bind(context, shader_stage::Vertex);
			cam_depcb.bind(context, shader_stage::Vertex);
			cam_depcb.bind(context, shader_stage::Pixel, 0);
			obj_cb.bind(context, shader_stage::Pixel);
			samp.bind(context, shader_stage::Pixel);
	}
	inline void unbind(ComPtr<ID3D11DeviceContext> context) override
	{
		current_shader->unbind(context);
		model_depcb.unbind(context, shader_stage::Vertex);
		cam_depcb.unbind(context, shader_stage::Vertex);
		cam_depcb.unbind(context, shader_stage::Pixel, 0);
		obj_cb.unbind(context, shader_stage::Pixel);
		samp.unbind(context, shader_stage::Pixel);
	}
	inline void update(ComPtr<ID3D11DeviceContext> context) override
	{
		model_depcb.update(context);
		cam_depcb.update(context);
		obj_cb.update(context);
		if (tex) tex->bind(context, shader_stage::Pixel);
	}
#pragma endregion

	blend_state bls;
public:
	df_lijjio_app()
		: dx_app(4, true),
		cam(float3(0, 5, 0.1f), float3(0, 0.1f, 0), 0.1f, 1000.f, to_radians(45.f))
	{
		this->clear_color = float4(0, 0, 0, 1);
	}

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

		gameobjects.push_back(new game_object(new model(mesh::create_sphere(device, 1.f, 32, 32)), 
			new texture2d(device, read_data_from_package(L"stone.dds")), 
			float3(0, 1.5f, 0), float3(), float3(1), 
			basic_material(float4(.8f, .8f, .8f, 1), float3(.8f, .8f, .8f), 16, 
				float3(.2f, .2f, .2f), false)));
		gameobjects.push_back(new game_object(new model(mesh::create_grid(device, 64, 64, 4, 4)), 
			new texture2d(device, read_data_from_package(L"floor.dds")), float3(), float3(), float3(1)));
		
		auto crate_model = new model(mesh::create_box(device, 1.f, 1.f, 1.f));
		auto crate_texture = new texture2d(device, read_data_from_package(L"crate.dds"));/*
		gameobjects.push_back(new game_object(crate_model, crate_texture, float3(3, .5f, -3)));
		gameobjects.push_back(new game_object(crate_model, crate_texture, float3(4, .5f, -3)));
		gameobjects.push_back(new game_object(crate_model, crate_texture, float3(3.5f, 1.5f, -3)));*/

		for (float y = 0; y < 5; ++y)
		{
			float vx = (5.f - y)*.5f;
			for (float x = -vx; x < vx; ++x)
			{
				gameobjects.push_back(new game_object(crate_model, crate_texture, float3(x, y + .5f, -4)));
			}
		}

		for (float y = 0; y < 5; ++y)
		{
			float vx = (5.f - y)*.5f;
			for (float x = -vx; x < vx; ++x)
			{
				gameobjects.push_back(new game_object(crate_model, crate_texture, float3(x, y + .5f, 4)));
			}
		}

		for (float y = 0; y < 5; ++y)
		{
			float vx = (5.f - y)*.5f;
			for (float x = -vx; x < vx; ++x)
			{
				gameobjects.push_back(new game_object(crate_model, crate_texture, float3(x+16, y + .5f, -20)));
			}
		}
		for (float y = 0; y < 5; ++y)
		{
			float vx = (5.f - y)*.5f;
			for (float x = -vx; x < vx; ++x)
			{
				gameobjects.push_back(new game_object(crate_model, crate_texture, float3(x - 16, y + .5f, 20)));
			}
		}

		auto basic_vs_data = read_data_from_package(L"basic_vs.cso");
		normals_shader = shader(device, basic_vs_data, read_data_from_package(L"dr_normals_ps.cso"), posnormtex_layout, _countof(posnormtex_layout));
		normals_buffer = render_texture(device, float2(windowBounds.width, windowBounds.height));
		diffuse_shader = shader(device, basic_vs_data, read_data_from_package(L"dr_diffuse_ps.cso"), posnormtex_layout, _countof(posnormtex_layout));
		diffuse_buffer = render_texture(device, float2(windowBounds.width, windowBounds.height));
		positions_shader = shader(device, basic_vs_data, read_data_from_package(L"dr_positions_ps.cso"), posnormtex_layout, _countof(posnormtex_layout));
		positions_buffer = render_texture(device, float2(windowBounds.width, windowBounds.height));
		spec_shader = shader(device, basic_vs_data, read_data_from_package(L"dr_spec_ps.cso"), posnormtex_layout, _countof(posnormtex_layout));
		spec_buffer = render_texture(device, float2(windowBounds.width, windowBounds.height));

		model_depcb = constant_buffer<model_dep>(device, 0, model_dep());
		cam_depcb = constant_buffer<cam_dep>(device, 1, cam_dep());
		obj_cb = constant_buffer<object_cb>(device, 1, object_cb());
		samp = sampler_state(device, 0);

		dr = new deferred_renderer(device, 
			deferred_renderer::draw_scene_func([&](ComPtr<ID3D11DeviceContext> ctx, render_shader& s) { this->draw_scene_dr(ctx, s); }), //strange hack around issue with STL
			vector<deferred_renderer::renderpass>({
				deferred_renderer::renderpass(
					new shader(device, basic_vs_data, read_data_from_package(L"dr_diffuse_ps.cso"), posnormtex_layout, _countof(posnormtex_layout)), 
					new render_texture(device, float2(windowBounds.width, windowBounds.height))),
				deferred_renderer::renderpass(
					new shader(device, basic_vs_data, read_data_from_package(L"dr_positions_ps.cso"), posnormtex_layout, _countof(posnormtex_layout)),
					new render_texture(device, float2(windowBounds.width, windowBounds.height))),
				deferred_renderer::renderpass(
					new shader(device, basic_vs_data, read_data_from_package(L"dr_normals_ps.cso"), posnormtex_layout, _countof(posnormtex_layout)),
					new render_texture(device, float2(windowBounds.width, windowBounds.height))),
				deferred_renderer::renderpass(
					new shader(device, basic_vs_data, read_data_from_package(L"dr_spec_ps.cso"), posnormtex_layout, _countof(posnormtex_layout)),
					new render_texture(device, float2(windowBounds.width, windowBounds.height))),
			})
		);

		pntlight_shader = shader(device, read_data_from_package(L"ndc_vs.cso"), read_data_from_package(L"dr_pointlight_ps.cso"), posnormtex_layout, _countof(posnormtex_layout));
		light_cb = constant_buffer<point_light>(device, 2, point_light());
		light_cb.data().pos = float4(0, 5, 0, .005f);
		light_cb.data().col = float4(1.f);
		light_cb.update(context);

		CD3D11_BLEND_DESC blend_desc;
		blend_desc.RenderTarget[0].BlendEnable = true;
		blend_desc.RenderTarget[0].BlendOp = D3D11_BLEND_OP_ADD;
		blend_desc.RenderTarget[0].DestBlend = D3D11_BLEND_ONE;
		blend_desc.RenderTarget[0].SrcBlend = D3D11_BLEND_ONE;
		bls = blend_state(device, true, D3D11_BLEND_ONE, D3D11_BLEND_ONE, D3D11_BLEND_OP_ADD);

		quad = create_ndc_quad(device);
		quad2 = create_ndc_quad(device, 1, .1f);
	}

	void update(float t, float dt) override
	{
		if (windowSizeChanged)
		{
			cam.update_proj(windowBounds.width / windowBounds.height);
			delete dr;
			auto basic_vs_data = read_data_from_package(L"basic_vs.cso");
			dr = new deferred_renderer(device,
				deferred_renderer::draw_scene_func([&](ComPtr<ID3D11DeviceContext> ctx, render_shader& s) { this->draw_scene_dr(ctx, s); }), //strange hack around issue with STL
				vector<deferred_renderer::renderpass>({
				deferred_renderer::renderpass(
				new shader(device, basic_vs_data, read_data_from_package(L"dr_diffuse_ps.cso"), posnormtex_layout, _countof(posnormtex_layout)),
				new render_texture(device, float2(windowBounds.width, windowBounds.height))),
				deferred_renderer::renderpass(
				new shader(device, basic_vs_data, read_data_from_package(L"dr_positions_ps.cso"), posnormtex_layout, _countof(posnormtex_layout)),
				new render_texture(device, float2(windowBounds.width, windowBounds.height))),
				deferred_renderer::renderpass(
				new shader(device, basic_vs_data, read_data_from_package(L"dr_normals_ps.cso"), posnormtex_layout, _countof(posnormtex_layout)),
				new render_texture(device, float2(windowBounds.width, windowBounds.height))),
				deferred_renderer::renderpass(
				new shader(device, basic_vs_data, read_data_from_package(L"dr_spec_ps.cso"), posnormtex_layout, _countof(posnormtex_layout)),
				new render_texture(device, float2(windowBounds.width, windowBounds.height))),
			})
			);
			dr->proj(cam.proj());
			windowSizeChanged = false;
		}

		//Quit via ESC
		{
			static bool last_esc_down = false;
			if (keyboard::key_down(VK_ESCAPE) && !last_esc_down)
			{
				BOOL fullscreen;
				chr(swapChain->GetFullscreenState(&fullscreen, nullptr));
				if (fullscreen)
				{
					chr(swapChain->SetFullscreenState(false, nullptr));
				}
				PostQuitMessage(0);
			}
		}

		const float spd = 5.f;
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
		dr->camera_position(cam.position());
		dr->view(cam.view());

		gameobjects[0]->position().x = sinf(-t) * 2;
		gameobjects[0]->position().z = cosf(t) * 2;
		
		for (auto& g : gameobjects)
			g->update(t, dt);
	}

	void draw_scene()
	{
		bind(context);

		for (auto& g : gameobjects)
			g->draw(context, *this);

		unbind(context);
	}

	void draw_scene_dr(ComPtr<ID3D11DeviceContext> ctx, render_shader& rs)
	{
		rs.bind(ctx);

		for (auto& g : gameobjects)
			g->draw(ctx, rs);

		rs.unbind(ctx);
	}

	void render(float t, float dt) override
	{
		dx_app::render(t, dt);

		dr->render(context, this);

		/*diffuse_buffer.push(this);
		current_shader = &diffuse_shader;
		draw_scene();
		pop_render_target();

		positions_buffer.push(this);
		current_shader = &positions_shader;
		draw_scene();
		pop_render_target();

		normals_buffer.push(this);
		current_shader = &normals_shader;
		draw_scene();
		pop_render_target();

		spec_buffer.push(this);
		current_shader = &spec_shader;
		draw_scene();
		pop_render_target();*/

		bls.om_bind(context);
		dr->current_shader() = &pntlight_shader;
		dr->bind(context);

		dr->update(context);

		light_cb.bind(context, shader_stage::Pixel);
		light_cb.data().pos = float4(0, 5, 0, .005f);
		light_cb.data().col = float4(1.f);
		light_cb.update(context);

		int i = 0;
		for (auto& rp : dr->render_passes())
		{
			rp.rt->bind(context, shader_stage::Pixel, i);
			i++;
		}

		quad2->draw(context);

		i = 0;
		for (auto& rp : dr->render_passes())
		{
			rp.rt->unbind(context, shader_stage::Pixel, i);
			i++;
		}

		static float2 light_vol = float2(randfn()*.25f, randfn()*.25f);
		static float2 light_pos = float2(0, 0);
		light_vol = light_vol + float2(randfn(), randfn())*dt;
		light_pos = light_pos + light_vol;
		if (light_pos.x < -32 || light_pos.x > 32) light_vol.x = -light_vol.x;
		if (light_pos.y < -32 || light_pos.y > 32) light_vol.y = -light_vol.y;

		light_cb.data().pos = float4(light_pos.x, 2, light_pos.y, .03f);
		light_cb.data().col = float4(.8f, .8f, .4f, 0.f);
		light_cb.update(context);

		i = 0;
		for (auto& rp : dr->render_passes())
		{
			rp.rt->bind(context, shader_stage::Pixel, i);
			i++;
		}

		quad->draw(context);

		i = 0;
		for (auto& rp : dr->render_passes())
		{
			rp.rt->unbind(context, shader_stage::Pixel, i);
			i++;
		}

		pntlight_shader.unbind(context);
		light_cb.unbind(context, shader_stage::Pixel);
		bls.om_unbind(context);
		dr->unbind(context);
	}
};

#pragma once
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
#include <render_texture.h>
#include "bo_file.h"
using namespace aldx;

#include "basic_shader.h"
#include "game_object.h"
#include "deferred_renderer.h"
#include "mesh_load_from_obj.h"

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




class df_lijjio_app : public dx_app
{
	vector<game_object*> gameobjects;
	camera cam;

	shader pntlight_shader;
	constant_buffer<point_light> light_cb;
	constant_buffer<float4> stuff_cb;
	
	struct pnt_light
	{
		point_light pl;
		float radius;
		pnt_light(float3 pos, float att, float4 col)
			: pl(float4(pos, att), col)
		{
			radius = sizeof_light_sphere(pl.pos.w);
		}
	};
	vector<pnt_light> lights;

	mesh* light_sphere;

	deferred_renderer* dr;

	blend_state bls;
	rasterizer_state rsl;
	ComPtr<ID3D11DepthStencilState> stencil_read_rps;

	static float sizeof_light_sphere(float x)
	{
		//unfortunately, i couldn't find a equation to do this,
		// so good ol' approximation / a loop go through all values of d
		float d = 0.01f;
		float a = FLT_MAX;
		do
		{
			d += 0.01f;
			a = (1 / (x*(d*d)));
		} while (fabsf(a) > 0.03f);
		return d;
	}
public:
	df_lijjio_app()
		: dx_app(4, true),
		cam(float3(0, 0, 4.1f), float3(0, 0.1f, 0), 0.1f, 1000.f, to_radians(45.f))
	{
		this->clear_color = float4(0, 0, 0, 1);
		
	}

	void load() override
	{
		gameobjects.push_back(new game_object(new model(mesh::create_sphere(device, 1.f, 32, 32)), 
			new texture2d(device, read_data_from_package(L"stone.dds")), 
			float3(4, 2.f, 8), float3(), float3(1), 
			basic_material(float4(.8f, .8f, .8f, 1), float3(.8f, .8f, .8f), 16, 
				float3(.2f, .2f, .2f), false)));
		//gameobjects.push_back(new game_object(new model(mesh::create_grid(device, 64, 64, 4, 4)), 
		//	new texture2d(device, read_data_from_package(L"floor.dds")), float3(), float3(), float3(1)));
		//
		//auto crate_model = new model(mesh::create_box(device, 1.f, 1.f, 1.f));
		auto crate_texture = new texture2d(device, read_data_from_package(L"crate.dds"));

		//for (float y = 0; y < 5; ++y)
		//{
		//	float vx = (5.f - y)*.5f;
		//	for (float x = -vx; x < vx; ++x)
		//	{
		//		gameobjects.push_back(new game_object(crate_model, crate_texture, float3(x, y + .5f, -4)));
		//	}
		//}

		//for (float y = 0; y < 5; ++y)
		//{
		//	float vx = (5.f - y)*.5f;
		//	for (float x = -vx; x < vx; ++x)
		//	{
		//		gameobjects.push_back(new game_object(crate_model, crate_texture, float3(x, y + .5f, 4)));
		//	}
		//}

		//for (float y = 0; y < 5; ++y)
		//{
		//	float vx = (5.f - y)*.5f;
		//	for (float x = -vx; x < vx; ++x)
		//	{
		//		gameobjects.push_back(new game_object(crate_model, crate_texture, float3(x+16, y + .5f, -20)));
		//	}
		//}
		//for (float y = 0; y < 5; ++y)
		//{
		//	float vx = (5.f - y)*.5f;
		//	for (float x = -vx; x < vx; ++x)
		//	{
		//		gameobjects.push_back(new game_object(crate_model, crate_texture, float3(x - 16, y + .5f, 20)));
		//	}
		//}

		gameobjects.push_back(new game_object(new model(device, load_bo(read_data_from_package(L"base.bo"))), crate_texture, float3(7, 1, 7)));
		gameobjects.push_back(new game_object(new model(device, load_bo(read_data_from_package(L"knot.bo"))), 
			crate_texture, float3(4, 2.0f, -16)));

		auto basic_vs_data = read_data_from_package(L"basic_vs.cso");
		
		set_up_defered_renderer();

		pntlight_shader = shader(device, basic_vs_data /*read_data_from_package(L"ndc_vs.cso")*/, read_data_from_package(L"dr_pointlight_ps.cso"), posnormtex_layout, _countof(posnormtex_layout));
		light_cb = constant_buffer<point_light>(device, 2, point_light());

		stuff_cb = constant_buffer<float4>(device, 3, float4(windowBounds.width, windowBounds.height,0,0));
		stuff_cb.update(context);

		lights.push_back(pnt_light(float3(8, 4, 8), .02f, float4(1.f)));
		lights.push_back(pnt_light(float3(8, 4, -16), .02f, float4(1.f, 1.f, .9f, 1.f)));
		/*
		lights.push_back(point_light(float4(0, 2, 0, .03f),  float4(0.8f, .8f, .4f, 1.f)));
		for (int i = 0; i < 5; ++i)
			lights.push_back(point_light(float4(randfn()*30, 5, randfn()*30, .08f), float4(0.7f)));*/
			

		bls = blend_state(device, true, D3D11_BLEND_ONE, D3D11_BLEND_ONE, D3D11_BLEND_OP_ADD);
		rsl = rasterizer_state(device, D3D11_FILL_SOLID, D3D11_CULL_FRONT);

		CD3D11_DEPTH_STENCIL_DESC rdsdec(false, D3D11_DEPTH_WRITE_MASK_ALL, D3D11_COMPARISON_LESS,
			false, D3D11_DEFAULT_STENCIL_READ_MASK, D3D11_DEFAULT_STENCIL_WRITE_MASK,
			D3D11_STENCIL_OP_KEEP, D3D11_STENCIL_OP_KEEP, D3D11_STENCIL_OP_KEEP, D3D11_COMPARISON_ALWAYS, //front
			D3D11_STENCIL_OP_KEEP, D3D11_STENCIL_OP_KEEP, D3D11_STENCIL_OP_KEEP, D3D11_COMPARISON_ALWAYS  //back
			);

		device->CreateDepthStencilState(&rdsdec, stencil_read_rps.GetAddressOf());

		light_sphere = mesh::create_sphere(device, 1.2f, 6, 6);//create_ndc_quad(device);
	}

	void set_up_defered_renderer()
	{
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
	}

	void update(float t, float dt) override
	{
		if (windowSizeChanged)
		{
			cam.update_proj(windowBounds.width / windowBounds.height);
			delete dr;
			set_up_defered_renderer();
			dr->proj(cam.proj());
			stuff_cb.data() = float4(windowBounds.width, windowBounds.height,0,0);
			stuff_cb.update(context);
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

		const float spd = 15.f;
		if (keyboard::key_down('A'))
			cam.strafe(-spd*dt);
		else if (keyboard::key_down('D'))
			cam.strafe(spd*dt);
		if (keyboard::key_down('W'))
			cam.forward(spd*dt);
		else if (keyboard::key_down('S'))
			cam.forward(-spd*dt);
		if (keyboard::key_down('Q'))
			cam.position().y += spd*2*dt;
		else if (keyboard::key_down('E'))
			cam.position().y -= spd*2*dt;

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

		gameobjects[0]->position().x = 8 + sinf(-t) * 6;
		gameobjects[0]->position().z = 8 + cosf(t) * 6;

		//static float2 light_vol = float2(randfn()*.25f, randfn()*.25f);
		//light_vol = light_vol + float2(randfn(), randfn())*dt;
		//lights[1].pos.x = lights[1].pos.x + light_vol.x;
		//lights[1].pos.z = lights[1].pos.z + light_vol.y;
		//if (lights[1].pos.x < -32 || lights[1].pos.x > 32) light_vol.x = -light_vol.x;
		//if (lights[1].pos.z < -32 || lights[1].pos.z > 32) light_vol.y = -light_vol.y;

		//lights[2].pos = float4(cam.position().x, cam.position().y, cam.position().z, 0.003f);
		
		for (auto& g : gameobjects)
			g->update(t, dt);
	}

	void draw_scene_dr(ComPtr<ID3D11DeviceContext> ctx, render_shader& rs)
	{
		rs.bind(ctx);

		for (auto& g : gameobjects)
			g->draw(ctx, rs);

		rs.unbind(ctx);
	}

	void render_point_light(pnt_light pl)
	{
		dr->world(XMMatrixScaling(pl.radius, pl.radius, pl.radius) * XMMatrixTranslationFromVector(pl.pl.pos));
		dr->update(context);		
		int i = 0;
		for (auto& rp : dr->render_passes())
		{
			rp.rt->bind(context, shader_stage::Pixel, i);
			i++;
		}

		light_cb.data() = pl.pl;
		light_cb.update(context);

		light_sphere->draw(context);		
		
		i = 0;
		for (auto& rp : dr->render_passes())
		{
			rp.rt->unbind(context, shader_stage::Pixel, i);
			i++;
		}
	}

	void render(float t, float dt) override
	{
		dx_app::render(t, dt);

		ComPtr<ID3DUserDefinedAnnotation> uda;
		context.As(&uda);

		uda->BeginEvent(L"Render to G Buffer");
		context->OMSetDepthStencilState(nullptr, 0);
		dr->render(context, this);
		uda->EndEvent();

		uda->BeginEvent(L"Render lights to framebuffer");
		dr->current_shader() = &pntlight_shader;
		dr->bind(context);
		bls.om_bind(context);
		rsl.bind(context);
		context->OMSetDepthStencilState(stencil_read_rps.Get(), 0);

		light_cb.bind(context, shader_stage::Pixel);
		stuff_cb.bind(context, shader_stage::Pixel);
		for (auto l : lights)
		{
			render_point_light(l);
		}
		uda->EndEvent();

		pntlight_shader.unbind(context);
		light_cb.unbind(context, shader_stage::Pixel);
		stuff_cb.unbind(context, shader_stage::Pixel);
		bls.om_unbind(context);
		rsl.unbind(context);
		dr->unbind(context);
	}
};
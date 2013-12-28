
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
//#include "bo_file.h"
using namespace aldx;

#include "basic_shader.h"
#include "game_object.h"

class st_lijjio_app : public dx_app
{
	vector<game_object*> gameobjects;
	camera cam;
	basic_shader shd;
public:
	st_lijjio_app()
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
			basic_material(float4(.8f, .8f, .8f, 1), float3(.8f, .8f, .8f), 32, float3(.2f, .2f, .2f), false)));

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
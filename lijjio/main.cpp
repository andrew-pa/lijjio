#define LINK_DIRECTX
#include <helper.h>
#include <dx_app.h>
#include <shader.h>
#include <mesh.h>
#include <camera.h>
#include "bo_file.h"
using namespace aldx;

class lijjio_app : public dx_app
{
	mesh knot;
	camera cam;
public:
	lijjio_app()
		: dx_app(4, true),
		cam(float3(0, 0, -5), float3(0, 0, 0), 0.1f, 1000.f, to_radians(45.f))
	{}

	void load() override
	{
		bo_file knot_mesh(read_data_from_package(L"knot.bo"));
		auto tkd = knot_mesh["Torus_Knot"];
		knot = mesh(device, (void*)(tkd.data + 2),
			(void*)(tkd.data + 2 + (*(uint32*)tkd.data)), *(uint32*)tkd.data + 1, *(uint32*)tkd.data, sizeof(dvertex), "knot");
	}

	void update(float t, float dt) override
	{
		if (windowSizeChanged)
		{
			cam.update_proj(windowBounds.width / windowBounds.height);
		}
	}

	void render(float t, float dt) override
	{
		dx_app::render(t, dt);
		knot.draw(context);
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
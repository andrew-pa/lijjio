#define LINK_DIRECTX
#include <helper.h>
#include <dx_app.h>
#include <shader.h>
#include <mesh.h>
using namespace aldx;

class lijjio_app : public dx_app
{
public:
	lijjio_app()
		: dx_app(4, true)
	{}

	void load() override
	{
	}

	void update(float t, float dt) override
	{
	}

	void render(float t, float dt) override
	{
		dx_app::render(t, dt);
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
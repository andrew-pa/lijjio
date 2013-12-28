#define LINK_DIRECTX
#include <helper.h>

#include "st_lijjio_app.h"



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
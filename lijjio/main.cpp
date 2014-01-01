#define LINK_DIRECTX
#include <helper.h>

#define DF

#include "st_lijjio_app.h"
#include "df_lijjio_app.h"

int CALLBACK WinMain(
	_In_  HINSTANCE inst,
	_In_  HINSTANCE pinst,
	_In_  LPSTR cmdLine,
	_In_  int cmds
	)
{

#ifdef ST
	st_lijjio_app app;
#elif defined(DF)
	df_lijjio_app app;
#endif
	app.run(inst, cmds, L"lijjio", 1280, 960);
}
#pragma once
#include <helper.h>
#include <mesh.h>
#include <model.h>
using namespace aldx;

model* model_load_from_obj(ComPtr<ID3D11Device> device, const string& fn);

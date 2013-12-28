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

class game_object
{
	model* _m;
	float4x4 w;
	basic_material mm;
	texture2d* _tex;
	float3 pos, rot, scl;
public:
	proprw(model*, mmodel, { return _m; })
		propr(float4x4, world, { return w; })
		proprw(float3, position, { return pos; })
		proprw(float3, rotation, { return rot; })
		proprw(float3, scale, { return scl; })
		proprw(basic_material, mmaterial, { return mm; })
		proprw(texture2d*, texture, { return _tex; })

		game_object(model* model_, texture2d* tex, float3 pos_ = float3(), float3 rot_ = float3(), float3 scl_ = float3(1),
		basic_material mm_ = basic_material(float4(.8f, .8f, .8f, 1.f), float3(.8f, .8f, .8f), 0, float3(.3f, .3f, .3f), false))
		: _m(model_), _tex(tex), pos(pos_), rot(rot_), scl(scl_), mm(mm_)
	{
		}

	virtual void update(float t, float dt)
	{
		w = XMMatrixScalingFromVector(scl) * XMMatrixRotationRollPitchYawFromVector(rot) * XMMatrixTranslationFromVector(pos);
	}

	virtual void draw(ComPtr<ID3D11DeviceContext> context, basic_shader& shd)
	{
		shd.set_material(mm);
		shd.set_texture(_tex);
		for (int i = 0; i < _m->meshes().size(); ++i)
		{
			shd.world(w * _m->worlds()[i]);
			shd.update(context);
			_m->meshes()[i]->draw(context);
		}
	}
};
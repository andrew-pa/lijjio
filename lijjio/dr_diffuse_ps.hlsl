
cbuffer camera : register (b0)
{
	float4x4 view;
	float4x4 world;
	float3 cam_pos;
};

struct mat
{
	float4 dif;
	float3 spec;
	float spec_exp;
	float3 amb; bool alpha_clip;
};

cbuffer object : register (b1)
{
	mat m;
}

Texture2D diffuse_texture : register(t0);
SamplerState smp : register(s0);

struct ps_in
{
	float4 pos : SV_POSITION;
	float3 posW : POSITION;
	float2 texc : TEXCOORD0;
	float3 normW : NORMAL;
};

float4 main(ps_in i) : SV_TARGET
{
	float4 texcol = diffuse_texture.Sample(smp, i.texc);
	return texcol * m.dif;
}
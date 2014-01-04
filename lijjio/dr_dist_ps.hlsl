cbuffer camera : register (b0)
{
	float4x4 view;
	float4x4 proj;
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

struct point_light
{
	float4 pos;
	float4 col;
};

cbuffer lighting : register (b2)
{
	point_light pl;
}

struct ps_in
{
	float4 pos : SV_POSITION;
	float3 posW : POSITION;
	float2 texc : TEXCOORD0;
	float3 normW : NORMAL;
};

float new_length(float3 l)
{
	return length(l);
}


float4 main(ps_in i) : SV_TARGET
{
	float d = length(-(pl.pos.xyz - i.posW))+0.001f;
	return float4(d*.1f, d*.1f, d*.1f, 1);
}
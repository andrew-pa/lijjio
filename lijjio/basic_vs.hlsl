#pragma pack_matrix(row_major)

cbuffer model_dependant : register(b0)
{
	float4x4 world;
	float4x4 inv_world_transpose;
};

cbuffer camera_dependant : register(b1)
{
	float4x4 view;
	float4x4 proj;
	float3 cam_pos;
};

struct vs_in
{
	float3 pos : POSITION;
	float3 norm : NORMAL;
	float2 texc : TEXCOORD;
};

struct ps_in
{
	float4 pos : SV_POSITION;
	float3 posW : POSITION;
	float2 texc : TEXCOORD0;
	float3 normW : NORMAL;
};

ps_in main( vs_in v )
{
	ps_in o;
	float4 p = float4(v.pos, 1.f);
	p = mul(p, world);
	o.posW = p.xyz;
	p = mul(p, view);
	p = mul(p, proj);
	o.pos = p;
	o.texc = v.texc;
	o.normW = mul(float4(v.norm,0), inv_world_transpose).xyz;
	return o;
}
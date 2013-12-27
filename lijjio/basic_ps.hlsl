
struct point_light
{
	float4 pos;
	float4 col;
};

struct dir_light
{
	float4 dir;
	float4 col;
};

cbuffer lights : register (b0)
{
	point_light point_lights[8];
	dir_light dir_lights[8];
	int num_point_lights;
	int num_dir_lights;
	float2 extra;
};

cbuffer camera : register (b1)
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

cbuffer object : register (b2)
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
	return float4(0, 1, 0, 1);
	float3 n = normalize(i.normW);
	float3 to_cam = normalize(cam_pos - i.posW);

	float3 color = m.amb;

	float4 texcol = diffuse_texture.Sample(smp, i.texc);
	if (m.alpha_clip)
	{
		clip(texcol.w - 0.2f);
	}

	for (int pli = 0; pli < num_point_lights; ++pli)
	{
		float3 l = (point_lights[pli].pos - i.posW);
		float a = 1 / dot(l, l); //inverse squared light factor
		if (a < 0.1f) continue;
		l = normalize(l);
		float df = max(dot(l, n), 0);
		float3 tlc = df * texcol.xyz * m.dif.xyz * 
			point_lights[pli].col.xyz;
		if (df > 0 && m.spec_exp > 0)
		{
			float3 v = reflect(-l, n);
			float sf = pow(max(dot(v, to_cam), 0), m.spec_exp);
			tlc += sf * m.spec * point_lights[pli].col.xyz;
		}
		color += tlc*a;
	}

	for (int dli = 0; dli < num_dir_lights; ++dli)
	{
		float3 l = dir_lights[dli].dir;
		float df = max(dot(l, n), 0);
		float3 tlc = df * texcol.xyz * m.dif.xyz * dir_lights[dli].col.xyz;
		if (df > 0 && m.spec_exp > 0)
		{
			float3 v = reflect(-l, n);
				float sf = pow(max(dot(v, to_cam), 0), m.spec_exp);
			tlc += sf * m.spec * dir_lights[dli].col.xyz;
		}
		color += tlc;
	}


	return float4(color, texcol.w * m.dif.w);
}
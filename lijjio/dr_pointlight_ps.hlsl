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

struct point_light
{
	float4 pos;
	float4 col;
};

cbuffer lighting : register (b2)
{
	point_light pl;
}

Texture2D diffuse_buffer : register(t0);
Texture2D positions_buffer : register(t1);
Texture2D normals_buffer : register(t2);
SamplerState smp : register(s0);

struct ps_in
{
	float4 pos : SV_POSITION;
	float2 texc : TEXCOORD0;
	float3 normW : NORMAL;
};

float4 main(ps_in i) : SV_TARGET
{
	//return float4(m.spec_exp, m.spec_exp, m.spec_exp, m.spec_exp);
	float3 pw = positions_buffer.Sample(smp, i.texc).xyz;
	float3 to_cam = normalize(cam_pos - pw);
	float3 n = normalize(normals_buffer.Sample(smp, i.texc).xyz);
	float3 l = pl.pos - pw;
	float a = clamp(pl.pos.w / dot(l, l), 0, 1);
	l = normalize(l);
	float df = max(dot(l, n), 0);
	float3 color = df*diffuse_buffer.Sample(smp, i.texc)*pl.col.xyz;
	//if (df > 0 && m.spec_exp > 0)
	//{
	//	float3 v = reflect(-l, n);
	////		float sf = pow(max(dot(v, to_cam), 0), m.spec_exp);
	//	color += sf * m.spec * pl.col;
	//}
	return float4(color, 1);
/*	if (i.texc.y < 0.5f)
	{
		if (i.texc.x < 0.5f)
			return diffuse_buffer.Sample(smp, i.texc*2.f);
		else
			return normals_buffer.Sample(smp, i.texc*2.f);
	}
	else
	{
		if(i.texc.x < 0.5f)
			return positions_buffer.Sample(smp, i.texc*float2(2.f, 2.f));
		else
		{
			float3 pw = positions_buffer.Sample(smp, i.texc).xyz;
				float3 to_cam = normalize(cam_pos - pw);
			float3 n = normalize(normals_buffer.Sample(smp, i.texc).xyz);
			float3 l = pl.pos - pw;
			float a = clamp(pl.pos.w / dot(l, l), 0, 1);
			l = normalize(l);
			float df = max(dot(l, n), 0);
			float3 color = df*diffuse_buffer.Sample(smp, i.texc)*pl.col.xyz;
			if (df > 0 && m.spec_exp > 0)
			{
				float3 v = reflect(-l, n);
				float sf = pow(max(dot(v, to_cam), 0), m.spec_exp);
				color += sf * m.spec * pl.col;
			}
			return float4(color, 1);
		}
	}
		*/
}
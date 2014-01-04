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

cbuffer stuff : register (b3)
{
	float2 screen_size;
	float2 extra;
}

Texture2D diffuse_buffer : register(t0);
Texture2D positions_buffer : register(t1);
Texture2D normals_buffer : register(t2);
Texture2D spec_buffer : register(t3);
SamplerState smp : register(s0);

struct ps_in
{
	float4 pos : SV_POSITION;
	float3 posW : POSITION;
	float2 texc : TEXCOORD0;
	float3 normW : NORMAL;
};
float4 shade(float2 tc)
{
	float3 pw = positions_buffer.Sample(smp, tc).xyz;
	if (pw.x == 0 && pw.y == 0 && pw.z == 0)
	{
		return float4(0, 0, 0, .5f);
	}
	float3 to_cam = normalize(cam_pos - pw);
	float3 n = normalize(normals_buffer.Sample(smp, tc).xyz);
	float3 l = pl.pos - pw;
	float a = clamp(1 / (pl.pos.w * dot(l,l)), 0, 1);
	l = normalize(l);
	float df = max(dot(l, n), 0);
	float3 color = df*diffuse_buffer.Sample(smp, tc)*pl.col.xyz;
	if (df > 0)
	{
		float4 sp = spec_buffer.Sample(smp, tc);
		if (sp.x > 0)
		{
			float3 v = reflect(-l, n);
				float sf = pow(max(dot(v, to_cam), 0), sp.x);
			color += sf * sp.yzw * pl.col;
		}
	}
	return float4(a*color, 0.5f);
}

float4 main(ps_in i) : SV_TARGET
{
	return shade(i.pos.xy / screen_size)/* + float4(0.01f, .01f, .01f, 0)*/;
}
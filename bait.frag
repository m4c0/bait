#version 450

layout(push_constant) uniform upc { float aspect; } pc;
layout(set = 0, binding = 0) uniform sampler2D icon_left;
layout(set = 0, binding = 1) uniform sampler2D icon_right;

layout(location = 0) in vec2 frag_coord;

layout(location = 0) out vec4 frag_colour;

// from a to b with THickness
vec2 dvf_rect(vec2 p, vec2 a, vec2 b, float th) {
  float l = length(b - a);
  vec2 d = (b - a) / l;
  vec2 q = p - (a + b) * 0.5;
  return mat2(d.x, -d.y, d.y, d.x) * q;
}

// b = half size
float sd_box(vec2 p, vec2 b) {
  vec2 d = abs(p) - b;
  return length(max(d, 0)) + min(max(d.x, d.y), 0);
}

float sd_rect(vec2 p, vec2 a, vec2 b, float th) {
  vec2 q = dvf_rect(p, a, b, th);
  return sd_box(q, vec2(length(b - a) * 0.5, th));
}

vec4 sq(vec2 a, vec2 b, float s, sampler2D smp) {
  b = a + s * normalize(b);

  float th = s;

  vec2 dv = dvf_rect(frag_coord, a, b, th);
  float d = sd_box(dv, vec2(th));

  vec2 uv = clamp(dv * (2.0 - 0.2) + 0.5, 0, 1);

  vec4 smp_c = texture(smp, uv);

  d = 0.005 / (d - th * 0.1);
  vec3 m = smp_c.xyz + step(0, d);
  return vec4(m, d);
}

void main() {
  vec4 sl = sq(vec2(-0.9, -0.1), vec2(0.6, 0.1), 0.25, icon_left);
  vec4 sr = sq(vec2(0.5, 0.2), vec2(0.6, -0.1), 0.3, icon_right);

  vec3 m = sr.xyz + sl.xyz;

  frag_colour = vec4(m, 1);
}

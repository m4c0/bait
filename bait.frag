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

float sd_arc(vec2 p, float ap, float r, float th) {
  float s = sin(ap);
  float c = cos(ap);
  p.x = abs(p.x);
  return (p.x * c > p.y * s)
    ? length(p - vec2(s, c) * r) - th
    : abs(length(p) - r) - th;
}
float sd_rect(vec2 p, vec2 a, vec2 b, float th) {
  vec2 q = dvf_rect(p, a, b, th);
  return sd_box(q, vec2(length(b - a) * 0.5, th));
}
float sd_x(vec2 p, float w, float r) {
  p = abs(p);
  return length(p - min(p.x + p.y, w) * 0.5) - r;
}
float sd_isotri(vec2 p, float w, float h) {
  vec2 q = vec2(w, h);
  p.x = abs(p.x);

  vec2 a = p - q * clamp(dot(p, q) / dot(q, q), 0.0, 1.0);
  vec2 b = p - q * vec2(clamp(p.x / q.x, 0.0, 1.0), 1.0);

  float s = -sign(q.y);
  vec2 d = min(vec2(dot(a, a), s * (p.x * q.y - p.y * q.x)),
               vec2(dot(b, b), s * (p.y - q.y)));
  return -sqrt(d.x) * sign(d.y);
}

vec4 sq(vec2 a, vec2 b, float s, sampler2D smp) {
  b = a + s * normalize(b);

  float th = s;

  vec2 dv = dvf_rect(frag_coord, a, b, th);
  float d = sd_box(dv, vec2(th));

  vec2 uv = clamp(dv * (2.0 / (1.0+ s)) + 0.5, 0, 1);

  vec4 smp_c = texture(smp, uv);

  d = 0.005 / (d - th * 0.1);
  vec3 m = mix(smp_c.xyz, vec3(d) * smp_c.xyz, step(0, d));
  return vec4(m, mix(1, d, step(0, d)));
}

mat2 rot(float a) {
  return mat2(cos(a), -sin(a), sin(a), cos(a));
}
vec2 op_rot(vec2 p, float a) {
  return mat2(cos(a), -sin(a), sin(a), cos(a)) * p;
}

float arrow() {
  vec2 p = frag_coord + vec2(0.8, 0.0);
  vec2 end_pos = rot(-1.7) * p;
  vec2 pos = mix(p, end_pos, length(p));
  float arc = sd_isotri(pos, 0.1, 0.5);

  float d = arc;
  d = 0.002 / clamp(d, 0, 1);
  return d;
}

void main() {
  vec4 sl = sq(vec2(-0.9, -0.2), vec2(0.6, 0.1), 0.25, icon_left);
  vec4 sr = sq(vec2(0.3, 0.1), vec2(0.6, -0.1), 0.4, icon_right);

  float x = sd_x(frag_coord + vec2(0.78, 0.18), 0.6, 0.002);
  x = 0.002 / abs(x);

  float arr = arrow();

  vec3 m = pow(sr.xyz, vec3(0.4)) + vec3(arr + x, 0, 0) + sl.xyz * 0.2;

  frag_colour = vec4(m, 1);
}

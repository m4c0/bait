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

vec4 sq(vec2 p, vec2 a, vec2 b, float s, sampler2D smp) {
  b = a + s * normalize(b);

  float th = s;

  vec2 dv = dvf_rect(p, a, b, th);
  float d = sd_box(dv, vec2(th));

  vec2 uv = clamp(dv * (2.0 / (1.0+ s)) + 0.5, 0, 1);

  vec4 smp_c = texture(smp, uv);

  vec2 pp = p - (a + b) / 2.0;
  float r = sqrt(pp.x * pp.x + pp.y * pp.y);
  float phi = sign(pp.y) * acos(pp.x / r);

  float sp = sin(phi * 20.0);

  float dd = 0.005 / (d - th * 0.1);
  float c = dd * (1.0 + smoothstep(sp, 0, 1));
  vec3 m = mix(smp_c.xyz, vec3(c) * smp_c.xyz, step(0, dd));
  return vec4(m, mix(1, d, step(0, dd)));
}

mat2 rot(float a) {
  return mat2(cos(a), -sin(a), sin(a), cos(a));
}
vec2 op_rot(vec2 p, float a) {
  return mat2(cos(a), -sin(a), sin(a), cos(a)) * p;
}

float arrow_body(vec2 p) {
  vec2 end_pos = rot(-1.5) * p;
  vec2 pos = mix(p, end_pos, length(p));
  return sd_isotri(pos, 0.1, 0.45);
}
float arrow_head(vec2 p) {
  p = rot(1.3) * p;
  return sd_isotri(p, 0.15, 0.4);
}
float arrow(vec2 p) {
  float arc = arrow_body(p + vec2(0.8, 0.0));
  float head = arrow_head(p + vec2(0.03, -0.25));

  float d = min(arc, head);
  d = 0.002 / clamp(d, 0, 1);
  return d;
}

float map(vec3 p) {
  float d_sp = length(p) - 0.25;

  float d_fl = p.y + 0.25;

  return min(d_sp, d_fl);
}

vec3 norm(vec3 p) {
  vec2 e = vec2(0.0001, 0.0);

  return normalize(vec3(
        map(p + e.xyy) - map(p - e.xyy),
        map(p + e.yxy) - map(p - e.yxy),
        map(p + e.yyx) - map(p - e.yyx)
        ));
}

float cast_ray(vec3 ro, vec3 rd) {
  float t = 0.0;
  for (int i = 0; i < 100; i++) {
    vec3 ray = ro + rd * t;

    float dt = map(ray);
    if (dt < 0.001) break;
    
    t += dt;
    if (t > 20.0) return -1.0;
  }
  return t;
}

void main() {
  vec2 p = frag_coord;

  float an = 0.0;

  vec3 ro = vec3(1.0 * sin(an), 0.0, 1.0 * cos(an));
  vec3 ta = vec3(0.0, 0.0, 0.0);

  vec3 ww = normalize(ta - ro);
  vec3 uu = normalize(cross(ww, vec3(0.0, 1.0, 0.0)));
  vec3 vv = normalize(cross(uu, ww));

  vec3 rd = normalize(p.x * uu + p.y * vv + 1.5 * ww);

  float t = cast_ray(ro, rd);

  vec3 col = vec3(0.4, 0.75, 1.0) - 0.7 * p.y;
  col = mix(col, vec3(0.7, 0.75, 0.8), exp(-10.0 * rd.y));

  if (t > 0.0) {
    vec3 pos = ro + rd * t;
    vec3 nor = norm(pos);

    vec3 mate = vec3(0.18);

    vec3 sun_dir = normalize(vec3(0.8, 0.4, 0.2));
    float sun_dif = clamp(dot(nor, sun_dir), 0.0, 1.0);
    float sun_shd = step(cast_ray(pos + nor * 0.001, sun_dir), 0.0);

    float sky_dif = clamp(0.5 + 0.5 * dot(nor, vec3(0.0, 1.0, 0.0)), 0.0, 1.0);

    float bou_dif = clamp(0.5 + 0.5 * dot(nor, vec3(0.0, -1.0, 0.0)), 0.0, 1.0);

    col  = mate * vec3(7.0, 4.5, 3.0) * sun_dif * sun_shd;
    col += mate * vec3(0.5, 0.8, 0.9) * sky_dif;
    col += mate * vec3(0.7, 0.3, 0.2) * bou_dif;
  }

  // It seems Bait applies this gamma somehow already
  // col = pow(col, vec3(0.4545));

  frag_colour = vec4(col, 1);
}

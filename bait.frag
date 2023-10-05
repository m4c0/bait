#version 450

layout(push_constant) uniform upc { float aspect; float time; } pc;
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
float sd_rect(vec2 p, vec2 b) {
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
  return sd_rect(q, vec2(length(b - a) * 0.5, th));
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
  float d = sd_rect(dv, vec2(th));

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

float sd_box(vec3 p, vec3 b) {
  vec3 q = abs(p) - b;
  return length(max(q, 0.0)) + min(max(q.x, max(q.y, q.z)), 0.0);
}
float sd_stick(vec3 p, vec3 a, vec3 b, float ra, float rb) {
  vec3 ba = b - a;
  vec3 pa = p - a;
  float h = clamp(dot(pa, ba) / dot(ba, ba), 0.0, 1.0);
  float r = mix(ra, rb, h);
  return length(pa - h * ba) - r;
}
float sd_elipsoid(vec3 p, vec3 rad) {
  float k0 = length(p / rad);
  float k1 = length(p / rad / rad);
  return k0 * (k0 - 1.0) / k1;
}
float sd_sphere(vec3 p, float r) {
  return length(p) - r;
}

float smin(float a, float b, float k) {
  float h = max(k - abs(a - b), 0.0);
  return min(a, b) - h * h / (k * 4.0);
}

float smax(float a, float b, float k) {
  float h = max(k - abs(a - b), 0.0);
  return max(a, b) + h * h / (k * 4.0);
}

vec2 map(vec3 p) {
  // monolith
  float d = sd_box(p - vec3(0.0, 1.0, 0.0), vec3(0.5, 1.0, 0.2)) - 0.05;
  vec2 d1 = vec2(d, 2.0);

  // ground
  float d2 = p.y;

  return (d2 < d1.x) ? vec2(d2, 1.0) : d1;
}

vec3 norm(vec3 p) {
  vec2 e = vec2(0.0001, 0.0);

  return normalize(vec3(
        map(p + e.xyy).x - map(p - e.xyy).x,
        map(p + e.yxy).x - map(p - e.yxy).x,
        map(p + e.yyx).x - map(p - e.yyx).x
        ));
}

vec2 cast_ray(vec3 ro, vec3 rd) {
  float m = -1.0;
  float t = 0.0;
  for (int i = 0; i < 100; i++) {
    vec3 ray = ro + rd * t;

    vec2 h = map(ray);
    m = h.y;
    if (h.x < 0.001) break;
    
    t += h.x;
    if (t > 20.0) return vec2(-1.0);
  }
  return vec2(t, m);
}

float cast_shadow(vec3 ro, vec3 rd) {
  float res = 1.0;

  float t = 0.001;
  for (int i = 0; i < 100; i++) {
    vec3 pos = ro + rd * t;
    float h = map(pos).x;
    res = min(res, 16.0 * h.x / t);
    if (res < 0.001) break;
    t += h;
    if (t > 20.0) break;
  }

  return clamp(res, 0.0, 1.0);
}

float calc_ao(vec3 p, vec3 n) {
  float eps = 0.1;
  float res = 0.0;
  float w = 0.5;
  for (int i = 1; i < 5; i++) {
    float d = eps * float(i);
    res += w * (1.0 - (d - map(p + d * n).x));
    w *= 0.5;
  }
  return res;
}

void main() {
  vec2 p = frag_coord;

  float ro_an = 0.5;
  float ro_d = 2.0;
  float ro_y = 0.3;

  vec3 ta = vec3(0.0, 1.0, 0.0);
  vec3 ro = vec3(sin(ro_an), 0.0, cos(ro_an)) * ro_d + vec3(0.0, ro_y, 0.0);

  vec3 ww = normalize(ta - ro);
  vec3 uu = normalize(cross(ww, vec3(0.0, 1.0, 0.0)));
  vec3 vv = normalize(cross(uu, ww));

  vec3 rd = normalize(p.x * uu + p.y * vv + 1.6 * ww);

  vec3 col = vec3(0.4, 0.75, 1.0) * 0.3 - 0.2 * p.y;
  col = mix(col, vec3(0.9, 0.6, 0.4) * 0.5, exp(-10.0 * rd.y));

  vec2 tm = cast_ray(ro, rd);
  if (tm.x > 0.0) {
    float t = tm.x;
    vec3 pos = ro + rd * t;
    vec3 nor = norm(pos);

    float sun_an = -1.0;

    float ao = calc_ao(pos, nor);

    vec3 sun_dir = normalize(vec3(sin(sun_an), 0.9, cos(sun_an)));
    float sun_dif = clamp(dot(nor, sun_dir), 0.0, 1.0);
    float sun_shd = cast_shadow(pos + nor * 0.001, sun_dir);
    float sun_spc = 0.0;

    float sky_dif = clamp(0.5 + 0.5 * dot(nor, vec3(0.0, 1.0, 0.0)), 0.0, 1.0);

    float bou_dif = clamp(0.5 + 0.5 * dot(nor, vec3(0.0, -1.0, 0.0)), 0.0, 1.0);

    vec3 mate = vec3(0.18);

    if (tm.y < 1.5) {
      mate = vec3(0.01, 0.03, 0.01);
    } else if (tm.y < 2.5) {
      mate = vec3(0.01, 0.01, 0.02);

      vec3 r = 2 * dot(sun_dir, nor) * nor - sun_dir;
      sun_spc = max(0.0, pow(dot(r, -rd), 20.0));
    }

    // https://en.wikipedia.org/wiki/Phong_reflection_model
    //     -----------------------imd   ----L.N   -----kd
    col  = mate * vec3(7.0, 4.5, 3.0) * sun_dif * sun_shd;
    col +=        vec3(7.0, 4.5, 3.0) * sun_spc;
    col += mate * vec3(0.5, 0.8, 0.9) * sky_dif * 0.5;
    col += mate * vec3(0.7, 0.3, 0.2) * bou_dif * 0.9;
    col *= ao;
  }

  // It seems Bait applies this gamma somehow already
  // col = pow(col, vec3(0.4545));

  frag_colour = vec4(col, 1);
}

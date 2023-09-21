#version 450

layout(push_constant) uniform upc {
  float aspect;
} pc;

layout(location = 0) in vec2 position;
layout(location = 1) in vec4 rect;

layout(location = 0) out vec2 frag_coord;

void main() {
  vec2 p = rect.xy + position * rect.zw;
  gl_Position = vec4(p, 0, 1);
  frag_coord = vec2(p.x * pc.aspect, p.y);
}

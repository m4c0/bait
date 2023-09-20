#version 450

layout(push_constant) uniform upc {
  float aspect;
} pc;

layout(location = 0) in vec2 position;
layout(location = 1) in vec4 colour_from;
layout(location = 2) in vec4 colour_to;
layout(location = 3) in vec4 rect;

layout(location = 0) out vec2 frag_coord;
layout(location = 1) out vec4 gradient;

void main() {
  vec2 p = rect.xy + position * rect.zw;
  gl_Position = vec4(p, 0, 1);
  gradient = mix(colour_from, colour_to, position.y);
  frag_coord = p * 2.0 - 1.0f;
}

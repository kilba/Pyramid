#version 450

layout(location = 0) in vec3 bs_Position;
layout(location = 1) in vec4 bs_Color;

layout(location = 0) out vec3 fragColor;

void main() {
    gl_Position = vec4(bs_Position, 1.0);
    fragColor = bs_Color.rgb;
}
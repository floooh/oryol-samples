@vs wireframeVS
uniform vsParams {
    mat4 viewProj;
};
in vec4 position;
in vec4 color0;
out vec4 color;
void main() {
    gl_Position = viewProj * position;
    color = color0;
}
@end

@fs wireframeFS
in vec4 color;
out vec4 fragColor;

void main() {
    fragColor = color;
}
@end

@program WireframeShader wireframeVS wireframeFS
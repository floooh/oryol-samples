//------------------------------------------------------------------------------
@block Util
vec4 gamma(vec4 c) {
    float p = 1.0/2.2;
    return vec4(pow(c.xyz, vec3(p, p, p)), c.w);
}
@end

@vs lambertVS
uniform vsParams {
    mat4 mvp;
    mat4 model;
};
in vec4 position;
in vec3 normal;
out vec3 N;
void main() {
    gl_Position = mvp * position;
    N = (model * vec4(normal, 0.0)).xyz;
}
@end

@fs lambertFS
/*
@include Util
uniform lightParams {
    vec3 lightDir;
    vec4 lightColor;
};
uniform matParams {
    vec4 diffuseColor;
};
*/
in vec3 N;
out vec4 fragColor;

void main() {
    vec3 n = normalize(N);
    fragColor = vec4(n*0.5+0.5, 1.0);
}
@end

@program LambertShader lambertVS lambertFS

//------------------------------------------------------------------------------
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



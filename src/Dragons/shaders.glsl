@block SkinUtil
void skinned_pos(in vec4 pos, in vec4 skin_weights, in vec4 skin_indices, in vec4 skin_info, out vec4 skin_pos) {
    vec4 weights = skin_weights / dot(skin_weights, vec4(1.0));
    vec2 step = vec2(1.0 / skin_info.z, 0.0);
    vec2 uv;
    vec4 xxxx, yyyy, zzzz;
    if (weights.x > 0.0) {
        uv = vec2(skin_info.x + (3.0*skin_indices.x)/skin_info.z, skin_info.y);
        xxxx = textureLod(boneTex, uv, 0.0);
        yyyy = textureLod(boneTex, uv + step, 0.0);
        zzzz = textureLod(boneTex, uv + 2.0 * step, 0.0);
        skin_pos.xyz = vec3(dot(pos,xxxx), dot(pos,yyyy), dot(pos,zzzz)) * weights.x;
    }
    else {
        skin_pos.xyz = vec3(0.0);
    }
    if (weights.y > 0.0) {
        uv = vec2(skin_info.x + (3.0*skin_indices.y)/skin_info.z, skin_info.y);
        xxxx = textureLod(boneTex, uv, 0.0);
        yyyy = textureLod(boneTex, uv + step, 0.0);
        zzzz = textureLod(boneTex, uv + 2.0 * step, 0.0);
        skin_pos.xyz += vec3(dot(pos,xxxx), dot(pos,yyyy), dot(pos,zzzz)) * weights.y;
    }
    if (weights.z > 0.0) {
        uv = vec2(skin_info.x + (3.0*skin_indices.z)/skin_info.z, skin_info.y);
        xxxx = textureLod(boneTex, uv, 0.0);
        yyyy = textureLod(boneTex, uv + step, 0.0);
        zzzz = textureLod(boneTex, uv + 2.0 * step, 0.0);
        skin_pos.xyz += vec3(dot(pos,xxxx), dot(pos,yyyy), dot(pos,zzzz)) * weights.z;
    }
    if (weights.w > 0.0) {
        uv = vec2(skin_info.x + (3.0*skin_indices.w)/skin_info.z, skin_info.y);
        xxxx = textureLod(boneTex, uv, 0.0);
        yyyy = textureLod(boneTex, uv + step, 0.0);
        zzzz = textureLod(boneTex, uv + 2.0 * step, 0.0);
        skin_pos.xyz += vec3(dot(pos,xxxx), dot(pos,yyyy), dot(pos,zzzz)) * weights.w;
    }
    skin_pos.w = 1.0;
}
@end

//------------------------------------------------------------------------------
@vs vs
uniform vsParams {
    mat4 view_proj;
};
uniform sampler2D boneTex;

@include SkinUtil

in vec4 position;
in vec3 normal;
in vec4 weights;
in vec4 indices;
in vec4 instance0;  // per-instance data, transposed model matrix, xxxx
in vec4 instance1;  // yyyy
in vec4 instance2;  // zzzz
in vec4 instance3;  // bone-texture lookup info
out vec3 N;
void main() {
    vec4 xxxx = instance0;
    vec4 yyyy = instance1;
    vec4 zzzz = instance2;
    vec4 skin_info = instance3;
    vec4 p0;
    skinned_pos(position, weights, indices, skin_info, p0);
    vec4 p1 = vec4(dot(p0, xxxx), dot(p0, yyyy), dot(p0, zzzz), 1.0);
    gl_Position = view_proj * p1;
    N = vec3(dot(normal, xxxx.xyz), dot(normal, yyyy.xyz), dot(normal, zzzz.xyz));
}
@end

@fs fs
in vec3 N;
out vec4 fragColor;

void main() {
    vec3 n = normalize(N);
    fragColor = vec4(n*0.5+0.5, 1.0);
}
@end

@program Shader vs fs




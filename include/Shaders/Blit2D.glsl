@vs vs

// Constant Buffer
layout(binding=0) uniform ShaderConstants_2D {
    mat4 orthographic;
};

//Inputs
in vec2 pos;
in vec4 col;
in vec2 uv;

//Outputs
out vec4 pixel_col;
out vec2 pixel_uv;
//Vec4 position is implied with gl_Positon

void main()
{
    gl_Position = vec4(pos, 0.0, 1.0) * orthographic;
    pixel_col = col;
    pixel_uv = uv;
}
@end

@fs fs

layout(binding=0) uniform texture2D in_texture;
layout(binding=0) uniform sampler in_sampler;

//Inputs
in vec4 pixel_col;
in vec2 pixel_uv;

//Outputs
out vec4 out_col;

void main()
{
    out_col = pixel_col * texture(sampler2D(in_texture, in_sampler), pixel_uv);
}
@end

@program Blit2D vs fs

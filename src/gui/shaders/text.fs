#version 300 es
precision mediump float;
in vec2 TexCoords;
out vec4 color;
uniform sampler2D text;
uniform vec3 textColor;
uniform float alpha;
uniform float bg_alpha;
void main()
{
    vec4 sampled = vec4(texture(text, TexCoords).r, texture(text, TexCoords).r, texture(text, TexCoords).r, 1.0);
    if(texture(text, TexCoords).r < 1.0)
    {
        sampled = vec4(texture(text, TexCoords).r, texture(text, TexCoords).r, texture(text, TexCoords).r, bg_alpha + texture(text, TexCoords).r * (2.0 - bg_alpha));
    }
    color = vec4(textColor, alpha) * sampled;
}

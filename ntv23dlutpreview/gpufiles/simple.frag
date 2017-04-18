#version 410

uniform sampler2D rgbPreviewTexture;

in vec2 texcoord;
out vec4 FragColor;

void main()
{
    FragColor = texture(rgbPreviewTexture, texcoord).bgra;
    //FragColor = vec4(1.0f, 0.0f, 0.0f, 1.0f); // This works!!
}

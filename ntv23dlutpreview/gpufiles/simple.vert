#version 410
uniform float angle;
in vec4 position;
out vec2 texcoord;

void main()
{
        gl_Position = vec4(position.x,-position.y,0.0,1.0);
        texcoord = position.xy * vec2(0.5) + vec2(0.5);

}

#version 330 core
out vec4 FragColor;

in vec3 outColor;
in vec2 TexCoord;

// texture sampler
uniform sampler2D texture1;
uniform sampler2D texture2;

void main()
{
	FragColor = mix(texture(texture1,TexCoord), texture(texture2,TexCoord), 0.8) * vec4(outColor,1.0);
}
#version 430 core

layout (local_size_x = 1, local_size_y = 1) in;

layout (shared) uniform var_uniform
{
    int a;
    int b;
};

layout (std430, binding = 0) buffer var_buffer
{
    int res[];
} buf_var1;

layout (binding = 1, rgba8i) uniform readonly iimage1D image1;

void main()
{
    // for(int i = 0; i < 10; ++i)
    // {
        buf_var1.res[0] = imageSize(image1);
        buf_var1.res[1] = imageLoad(image1, 0).r;
        buf_var1.res[2] = imageLoad(image1, 0).g;
        buf_var1.res[3] = imageLoad(image1, 0).b;
        buf_var1.res[4] = imageLoad(image1, 0).a;
        
    // }
}
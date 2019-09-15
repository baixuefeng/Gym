#include "stdafx.h"
#include "TestUnit/OpenGLTest.h"

BEGIN_SHARELIBTEST_NAMESPACE

OpenGLWindow::OpenGLWindow() {}

OpenGLWindow::~OpenGLWindow() {}

int OpenGLWindow::OnCreate(LPCREATESTRUCT /*lpCreateStruct*/)
{
    m_glContext.InitOpenglWindow(*this);
    m_glContext.InitThreadRC();

    m_shader.CompileFile(shr::ShaderType::COMPUTE_SHADER, L"..\\TestUnit\\test_gpu.glsl");
    m_glslProgram.AttachShader(m_shader);
    m_glslProgram.LinkProgram();
    m_glslProgram.UseProgram();

    const char *UniformNames[] = {"a", "b"};
    GLint offsets[2] = {0};
    m_glslProgram.GetUniformBlockOffsets(2, UniformNames, offsets);
    auto uniformBlockIndex = m_glslProgram.GetUniformBlockIndex("var_uniform");

    shr::glhBufferMgr uniformBuffer;
    uniformBuffer.Create(shr::BufferTarget::UNIFORM_BUFFER, false);
    uniformBuffer.BindBufferBase(uniformBlockIndex);
    GLint uniformValue[2] = {11, 22};
    uniformBuffer.BufferData(sizeof(uniformValue), &uniformValue, shr::BufferUsage::DYNAMIC_DRAW);

    shr::glhBufferMgr bfoBuffer;
    bfoBuffer.Create(shr::BufferTarget::SHADER_STORAGE_BUFFER, true);
    bfoBuffer.BindBufferBase(0);
    GLint bufferValue[10] = {0};
    bfoBuffer.BufferData(sizeof(bufferValue), &bufferValue, shr::BufferUsage::DYNAMIC_DRAW);

    for (int i = 0; i < sizeof(bufferValue) / sizeof(bufferValue[0]); ++i)
    {
        bufferValue[i] = 0x0FFFFFFF;
    }
    shr::glhTextureMgr texture;
    texture.Create(shr::TextureTarget::TEXTURE_1D);
    texture.TexStorage1D(1, shr::TextureFormat::RGBA8I, 10);
    shr::glhBufferMgr textureBuffer;
    textureBuffer.Create(shr::BufferTarget::TEXTURE_BUFFER, true);
    texture.TexBuffer(shr::TextureFormat::RGBA8I, textureBuffer);
    textureBuffer.BufferData(sizeof(bufferValue), bufferValue, shr::BufferUsage::DYNAMIC_READ);
    texture.BindImageTexture(
        1, 0, TRUE, 0, shr::TextureAccess::READ_WRITE, shr::TextureFormat::RGBA8I);

    glDispatchCompute(1, 1, 1);
    glFinish();

    auto p = bfoBuffer.MapBufferRange(0, sizeof(bufferValue), shr::BufferAccessBit::MAP_READ_BIT);
    if (p)
    {
        std::memcpy(bufferValue, p, sizeof(bufferValue));
        bfoBuffer.UnmapBuffer();
    }

    return 0;
}

void OpenGLWindow::OnPaint(CDCHandle dc) {}

void OpenGLWindow::OnDestroy()
{
    m_glContext.ClearThreadRC();
}

void OpenGLWindow::OnFinalMessage(_In_ HWND /*hWnd*/)
{
    PostQuitMessage(0);
}

void TestOpenGL()
{
    OpenGLWindow win;
    win.Create(NULL);
    win.ShowWindow(SW_SHOW);
    WTL::CMessageLoop{}.Run();
}

END_SHARELIBTEST_NAMESPACE

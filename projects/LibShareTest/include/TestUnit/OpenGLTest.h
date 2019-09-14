#pragma once
#include "UI/Utility/WindowUtility.h"
#include "OpenGL/glHelper.h"

BEGIN_SHARELIBTEST_NAMESPACE

class OpenGLWindow :
    public ATL::CWindowImpl<OpenGLWindow, ATL::CWindow, ATL::CWinTraits<WS_OVERLAPPEDWINDOW | WS_CLIPCHILDREN | WS_CLIPSIBLINGS, WS_EX_OVERLAPPEDWINDOW> >,
    public shr::MinRestoreHandler
{
    BEGIN_MSG_MAP_EX(OpenGLWindow)
        CHAIN_MSG_MAP(shr::MinRestoreHandler)
        MSG_WM_CREATE(OnCreate)
        MSG_WM_PAINT(OnPaint)
        MSG_WM_DESTROY(OnDestroy)
    END_MSG_MAP()

public:
    OpenGLWindow();
    ~OpenGLWindow();

protected:
    int OnCreate(LPCREATESTRUCT /*lpCreateStruct*/);
    void OnPaint(CDCHandle dc);
    void OnDestroy();
    virtual void OnFinalMessage(_In_ HWND /*hWnd*/) override;

private:
    shr::glhWindowContext m_glContext;
    shr::glhGlslShader m_shader;
    shr::glhGlslProgram m_glslProgram;
};

void TestOpenGL();

END_SHARELIBTEST_NAMESPACE

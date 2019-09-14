#include "targetver.h"
#include "OpenGL/glHelper.h"
#include <cassert>
#include <stdexcept>
#include <cstring>
#include <mutex>
#include <cstdlib>
#include <locale>
#include <codecvt>
#include <boost/interprocess/file_mapping.hpp>
#include <boost/interprocess/mapped_region.hpp>

#pragma comment(lib, "glu32.lib")    /* link OpenGL Utility lib     */
#pragma comment(lib, "opengl32.lib") /* link Microsoft OpenGL lib   */

static const int ERR_MSG_BUFFER_MAX_LENGTH = 1024;

/*  glIsBuffer returns GL_TRUE if buffer is currently the name of a buffer object.
If buffer is zero, or is a non-zero value that is not currently the name of a
buffer object, or if an error occurs, glIsBuffer returns GL_FALSE.
    A name returned by glGenBuffers, but not yet associated with a buffer object
by calling glBindBuffer, is not the name of a buffer object.
*/
static const GLuint INVALID_BUFFER_INDEX = 0;

/*  glIsTexture returns GL_TRUE if texture is currently the name of a texture. If 
texture is zero, or is a non-zero value that is not currently the name of a texture, 
or if an error occurs, glIsTexture returns GL_FALSE.
    A name returned by glGenTextures, but not yet associated with a texture by calling
glBindTexture, is not the name of a texture.
*/
static const GLuint INVALID_TEXTURE_INDEX = 0;

SHARELIB_BEGIN_NAMESPACE

glhWindowContext::glhWindowContext()
{
	m_hWnd = NULL;
	m_hDc = NULL;
}

glhWindowContext::~glhWindowContext()
{
	Destroy();
}

bool glhWindowContext::InitOpenglWindow(HWND hWnd)
{
	if (m_hDc && m_hWnd)
	{
		return true;
	}
	HDC hDc = ::GetDC(hWnd);
	if (hDc)
	{
		PIXELFORMATDESCRIPTOR pfd{ sizeof(pfd) };
		pfd.nVersion = 1;
        pfd.dwFlags = PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER;
		pfd.iPixelType = PFD_TYPE_RGBA;
		pfd.cDepthBits = 24;
		pfd.cStencilBits = 8;
		int n = ::ChoosePixelFormat(hDc, &pfd);
		if (n > 0)
		{
			if (::SetPixelFormat(hDc, n, &pfd))
			{
				m_hWnd = hWnd;
				m_hDc = hDc;
				return true;
			}
		}

		::ReleaseDC(hWnd, hDc);
		return false;
	}
	return false;
}

uint8_t glhWindowContext::GetStencilBufferBit()
{
    if (m_hDc)
    {
        PIXELFORMATDESCRIPTOR pfd{ sizeof(pfd) };
        ::DescribePixelFormat(m_hDc, ::GetPixelFormat(m_hDc), sizeof(pfd), &pfd);
        return pfd.cStencilBits;
    }
    return 0;
}

uint8_t glhWindowContext::GetDepthBufferBit()
{
    if (m_hDc)
    {
        PIXELFORMATDESCRIPTOR pfd{ sizeof(pfd) };
        ::DescribePixelFormat(m_hDc, ::GetPixelFormat(m_hDc), sizeof(pfd), &pfd);
        return pfd.cDepthBits;
    }
    return 0;
}

bool glhWindowContext::InitThreadRC()
{
	if (m_hDc)
	{
		HGLRC hRc = ::wglGetCurrentContext();
		if (hRc)
		{
			return true;
		}
		hRc = ::wglCreateContext(m_hDc);
		if (hRc)
		{
			if (::wglMakeCurrent(m_hDc, hRc))
			{
				//初始化glew库
                static std::once_flag s_initFlag;
                std::call_once(s_initFlag,
					[]()
				{
					GLenum status = glewInit();
                    (void)status;
					assert(status == GLEW_OK);
				});

                ConfigRC();
				return true;
			}
			::wglDeleteContext(hRc);
		}
	}
	return false;
}

bool glhWindowContext::UseDebugRC()
{
	if (m_hDc)
	{
		HGLRC hRc = ::wglGetCurrentContext();
		if (hRc && wglCreateContextAttribsARB)
		{
			const char * pVersion = (const char*)glGetString(GL_VERSION);
			char * pEnd = nullptr;
			int nMajor = std::strtol(pVersion, &pEnd, 0);
			int nMinor = 0;
			if (pEnd && *pEnd == '.')
			{
				nMinor = std::strtol(++pEnd, nullptr, 0);
			}

			const int nAttribList[] = 
			{
				WGL_CONTEXT_MAJOR_VERSION_ARB, nMajor,
				WGL_CONTEXT_MINOR_VERSION_ARB, nMinor,
				WGL_CONTEXT_PROFILE_MASK_ARB, WGL_CONTEXT_CORE_PROFILE_BIT_ARB,
				WGL_CONTEXT_FLAGS_ARB, WGL_CONTEXT_DEBUG_BIT_ARB,
				0
			};
			HGLRC hDebugRc = wglCreateContextAttribsARB(m_hDc, hRc, nAttribList);
			if (hDebugRc)
			{
				::wglMakeCurrent(m_hDc, nullptr);
				::wglDeleteContext(hRc);
				::wglMakeCurrent(m_hDc, hDebugRc);
                ConfigRC();
                return true;
			}
		}
	}
	return false;
}

bool glhWindowContext::IsValid() const
{
	return (m_hDc && m_hWnd && ::wglGetCurrentContext());
}

void glhWindowContext::ClearThreadRC()
{
	if (m_hDc)
	{
		HGLRC hRc = ::wglGetCurrentContext();
		if (hRc)
		{
			::wglMakeCurrent(NULL, NULL);
			::wglDeleteContext(hRc);
		}
	}
}

bool glhWindowContext::SwapBuffers()
{
	if (m_hDc)
	{
        ::glFlush();
		return !!::SwapBuffers(m_hDc);
	}
	return false;
}

void glhWindowContext::Destroy()
{
	if (m_hDc)
	{
		::ReleaseDC(m_hWnd, m_hDc);
		m_hWnd = NULL;
		m_hDc = NULL;
	}
}

bool glhWindowContext::ConfigRC()
{
    //剪切
    ::glEnable(GL_SCISSOR_TEST);

    //多重采样
    if (EnableMultiSample(true))
    {
        ::glEnable(GL_SAMPLE_ALPHA_TO_COVERAGE);
    }

    ::glEnable(GL_BLEND);
    ::glBlendFunc(EnumToGL(BlendFuncType::SRC_ALPHA), EnumToGL(BlendFuncType::ONE_MINUS_SRC_ALPHA));
    //::glEnable(GL_COLOR_LOGIC_OP); 与融混有冲突
    //::glLogicOp(LogicOpType::BIT_COPY);

    ::glEnable(GL_STENCIL_TEST);
    //::glEnable(GL_DEPTH_TEST);

    //::glEnable(SmoothType::LINE_SMOOTH);
    //::glEnable(SmoothType::POLYGON_SMOOTH);
    //::glHint(HintTargetType::LINE_SMOOTH_HINT, HintModeType::NICEST_HINT);
    //::glHint(HintTargetType::POLYGON_SMOOTH_HINT, HintModeType::NICEST_HINT);
    //::glHint(HintTargetType::FRAGMENT_SHADER_DERIVATIVE_HINT, HintModeType::NICEST_HINT);
    //::glHint(HintTargetType::TEXTURE_COMPRESSION_HINT, HintModeType::NICEST_HINT);
    return true;
}

//-------------------------------------------------------------------------------

glhVertextArrayMgr::glhVertextArrayMgr()
{
}

glhVertextArrayMgr::~glhVertextArrayMgr()
{
    Delete();
}

bool glhVertextArrayMgr::Generate(GLsizei n)
{
    assert(GLEW_ARB_vertex_array_object);
    if (!GLEW_ARB_vertex_array_object || n <= 0)
    {
        return false;
    }
    
    m_array.assign(n, 0);
    ::glGenVertexArrays(n, &m_array[0]);
    if (::glGetError() != GL_NO_ERROR)
    {
        Delete();
        return false;
    }
    return true;
}

bool glhVertextArrayMgr::Bind(size_t nBufferIndex)
{
    assert(GLEW_ARB_vertex_array_object);
    if (!GLEW_ARB_vertex_array_object || nBufferIndex >= m_array.size())
    {
        return false;
    }
    ::glBindVertexArray(m_array[nBufferIndex]);
    return !!::glIsVertexArray(m_array[nBufferIndex]);
}

void glhVertextArrayMgr::Delete()
{
    assert(GLEW_ARB_vertex_array_object);
    if (GLEW_ARB_vertex_array_object)
    {
        if (!m_array.empty())
        {
            ::glDeleteVertexArrays((GLsizei)m_array.size(), &m_array[0]);
            m_array.clear();
        }
    }
}

//-------------------------------------------------------------------------------

glhBufferMgr::glhBufferMgr()
    : m_bufferIndex(INVALID_BUFFER_INDEX)
    , m_target(0)
{
}

glhBufferMgr::~glhBufferMgr()
{
    Delete();
}

bool glhBufferMgr::Create(BufferTarget target, bool bind)
{
    assert(GLEW_VERSION_1_5);
    if (!GLEW_VERSION_1_5)
    {
        return false;
    }
    Delete();
    ::glGenBuffers(1, &m_bufferIndex);
    if ((::glGetError() == GL_NO_ERROR) && 
        (INVALID_BUFFER_INDEX != m_bufferIndex))
    {
        if (bind)
        {
            ::glBindBuffer(EnumToGL(target), m_bufferIndex);
            assert(::glGetError() == GL_NO_ERROR);
            if (::glGetError() == GL_NO_ERROR)
            {
                m_target = EnumToGL(target);
                return true;
            }
        }
        else
        {
            m_target = EnumToGL(target);
            return true;
        }
    }
    Delete();
    return false;
}

void glhBufferMgr::Delete()
{
    assert(GLEW_VERSION_1_5);
    if (!GLEW_VERSION_1_5)
    {
        return;
    }
    if (INVALID_BUFFER_INDEX == m_bufferIndex)
    {
        ::glDeleteBuffers(1, &m_bufferIndex);
        m_bufferIndex = INVALID_BUFFER_INDEX;
        m_target = 0;
    }
}

glhBufferMgr::operator GLuint() const
{
    return m_bufferIndex;
}

BufferTarget glhBufferMgr::BufferType() const
{
    return (BufferTarget)m_target;
}

bool glhBufferMgr::IsValid() const
{
    return INVALID_BUFFER_INDEX != m_bufferIndex;
}

bool glhBufferMgr::BindBufferRange(GLuint ntargetIndex, GLintptr offset, GLsizeiptr size)
{
    assert(GLEW_ARB_uniform_buffer_object);
    if (!GLEW_ARB_uniform_buffer_object || (INVALID_BUFFER_INDEX == m_bufferIndex))
    {
        return false;
    }
    ::glBindBufferRange(m_target, ntargetIndex, m_bufferIndex, offset, size);
    assert(::glGetError() == GL_NO_ERROR);
    return ::glGetError() == GL_NO_ERROR;
}

bool glhBufferMgr::BindBufferBase(GLuint ntargetIndex)
{
    assert(GLEW_ARB_uniform_buffer_object);
    if (!GLEW_ARB_uniform_buffer_object || (INVALID_BUFFER_INDEX == m_bufferIndex))
    {
        return false;
    }
    ::glBindBufferBase(m_target, ntargetIndex, m_bufferIndex);
    assert(::glGetError() == GL_NO_ERROR);
    return ::glGetError() == GL_NO_ERROR;
}

bool glhBufferMgr::BufferData(GLsizeiptr size, const void *pData, BufferUsage usage)
{
    assert(GLEW_VERSION_1_5);
    if (!GLEW_VERSION_1_5)
    {
        return false;
    }
    ::glBufferData(m_target, size, pData, EnumToGL(usage));
    assert(::glGetError() == GL_NO_ERROR);
    return ::glGetError() == GL_NO_ERROR;
}

bool glhBufferMgr::BufferSubData(GLintptr offset, GLsizeiptr size, const void *pData)
{
    assert(GLEW_VERSION_1_5);
    if (!GLEW_VERSION_1_5)
    {
        return false;
    }
    ::glBufferSubData(m_target, offset, size, pData);
    assert(::glGetError() == GL_NO_ERROR);
    return ::glGetError() == GL_NO_ERROR;
}

bool glhBufferMgr::CopyBufferSubData(BufferTarget readTarget, BufferTarget writeTarget, GLintptr readOffset, GLintptr writeOffset, GLsizeiptr size)
{
    assert(GLEW_ARB_copy_buffer);
    if (!GLEW_ARB_copy_buffer)
    {
        return false;
    }
    ::glCopyBufferSubData(EnumToGL(readTarget), EnumToGL(writeTarget), readOffset, writeOffset, size);
    assert(::glGetError() == GL_NO_ERROR);
    return ::glGetError() == GL_NO_ERROR;
}

void * glhBufferMgr::MapBufferRange(GLintptr offset, GLsizeiptr length, BufferAccessBit access)
{
    assert(GLEW_ARB_map_buffer_range);
    if (!GLEW_ARB_map_buffer_range)
    {
        return nullptr;
    }
    return ::glMapBufferRange(m_target, offset, length, EnumToGL(access));
}

void * glhBufferMgr::MapBuffer(BufferAccessBit access)
{
    assert(GLEW_VERSION_1_5);
    if (!GLEW_VERSION_1_5)
    {
        return nullptr;
    }
    return ::glMapBuffer(m_target, EnumToGL(access));
}

bool glhBufferMgr::FlushMappedBufferRange(GLintptr offset, GLsizeiptr length)
{
    assert(GLEW_ARB_map_buffer_range);
    if (!GLEW_ARB_map_buffer_range)
    {
        return false;
    }
    ::glFlushMappedBufferRange(m_target, offset, length);
    assert(::glGetError() == GL_NO_ERROR);
    return ::glGetError() == GL_NO_ERROR;
}

bool glhBufferMgr::UnmapBuffer()
{
    assert(GLEW_VERSION_1_5);
    if (!GLEW_VERSION_1_5)
    {
        return false;
    }
    return !!::glUnmapBuffer(m_target);
}

GLint glhBufferMgr::GetBufferInfo(BufferTarget target, BufferParameter param)
{
    assert(GLEW_VERSION_1_5);
    if (!GLEW_VERSION_1_5)
    {
        return -1;
    }
    GLint res = -1;
    ::glGetBufferParameteriv(EnumToGL(target), EnumToGL(param), &res);
    assert(::glGetError() == GL_NO_ERROR);
    return res;
}

//----------------------------------------------------------------------------

glhTextureMgr::glhTextureMgr()
    : m_textureIndex(INVALID_TEXTURE_INDEX)
    , m_target(0)
{
}

glhTextureMgr::~glhTextureMgr()
{
}

bool glhTextureMgr::Create(TextureTarget target)
{
    glGenTextures(1, &m_textureIndex);
    assert(::glGetError() == GL_NO_ERROR);
    if ((::glGetError() == GL_NO_ERROR) &&
        (INVALID_TEXTURE_INDEX != m_textureIndex))
    {
        glBindTexture(EnumToGL(target), m_textureIndex);
        assert(::glGetError() == GL_NO_ERROR);
        if (::glGetError() == GL_NO_ERROR)
        {
            m_target = EnumToGL(target);
            return true;
        }
    }
    Delete();
    return false;
}

glhTextureMgr::operator GLuint() const
{
    return m_textureIndex;
}

TextureTarget glhTextureMgr::TextureType() const
{
    return (TextureTarget)m_target;
}

bool glhTextureMgr::IsValid() const
{
    return INVALID_TEXTURE_INDEX != m_textureIndex;
}

void glhTextureMgr::Delete()
{
    if (INVALID_TEXTURE_INDEX != m_textureIndex)
    {
        glDeleteTextures(1, &m_textureIndex);
        m_textureIndex = INVALID_TEXTURE_INDEX;
        m_target = 0;
    }
}

bool glhTextureMgr::TexStorage1D(GLsizei levels, TextureFormat format, GLsizei width)
{
    if (!GLEW_ARB_texture_storage || !IsValid())
    {
        return false;
    }
    if (EnumToGL(TextureTarget::TEXTURE_1D) != m_target)
    {
        assert(!"error type");
        return false;
    }
    glTexStorage1D(m_target, levels, EnumToGL(format), width);
    assert(::glGetError() == GL_NO_ERROR);
    return ::glGetError() == GL_NO_ERROR;
}

bool glhTextureMgr::TexStorage2D(GLsizei levels, TextureFormat format, GLsizei width, GLsizei height)
{
    if (!GLEW_ARB_texture_storage || !IsValid())
    {
        return false;
    }
    if ((EnumToGL(TextureTarget::TEXTURE_2D) != m_target) &&
        (EnumToGL(TextureTarget::TEXTURE_1D_ARRAY) != m_target) &&
        (EnumToGL(TextureTarget::TEXTURE_RECTANGLE) != m_target) &&
        (EnumToGL(TextureTarget::TEXTURE_CUBE_MAP) != m_target))
    {
        assert(!"error type");
        return false;
    }
    glTexStorage2D(m_target, levels, EnumToGL(format), width, height);
    assert(::glGetError() == GL_NO_ERROR);
    return ::glGetError() == GL_NO_ERROR;
}

bool glhTextureMgr::TexStorage3D(GLsizei levels, TextureFormat format, GLsizei width, GLsizei height, GLsizei depth)
{
    if (!GLEW_ARB_texture_storage || !IsValid())
    {
        return false;
    }
    if ((EnumToGL(TextureTarget::TEXTURE_3D) != m_target) &&
        (EnumToGL(TextureTarget::TEXTURE_2D_ARRAY) != m_target) &&
        (EnumToGL(TextureTarget::TEXTURE_CUBE_MAP_ARRAY) != m_target))
    {
        assert(!"error type");
        return false;
    }
    glTexStorage3D(m_target, levels, EnumToGL(format), width, height, depth);
    assert(::glGetError() == GL_NO_ERROR);
    return ::glGetError() == GL_NO_ERROR;
}

bool glhTextureMgr::TexStorage2DMultisample(GLsizei levels, TextureFormat format, GLsizei width, GLsizei height, GLboolean fixedsamplelocations)
{
    if (!GLEW_ARB_texture_storage_multisample || !IsValid())
    {
        return false;
    }
    if (EnumToGL(TextureTarget::TEXTURE_2D_MULTISAMPLE) != m_target)
    {
        assert(!"error type");
        return false;
    }
    glTexStorage2DMultisample(m_target, levels, EnumToGL(format), width, height, fixedsamplelocations);
    assert(::glGetError() == GL_NO_ERROR);
    return ::glGetError() == GL_NO_ERROR;
}

bool glhTextureMgr::TexStorage3DMultisample(GLsizei levels, TextureFormat format, GLsizei width, GLsizei height, GLsizei depth, GLboolean fixedsamplelocations)
{
    if (!GLEW_ARB_texture_storage_multisample || !IsValid())
    {
        return false;
    }
    if (EnumToGL(TextureTarget::TEXTURE_2D_MULTISAMPLE_ARRAY) != m_target)
    {
        assert(!"error type");
        return false;
    }
    glTexStorage3DMultisample(m_target, levels, EnumToGL(format), width, height, depth, fixedsamplelocations);
    assert(::glGetError() == GL_NO_ERROR);
    return ::glGetError() == GL_NO_ERROR;
}

bool glhTextureMgr::TexBuffer(TextureFormat format, const glhBufferMgr & buffer)
{
    if (!GLEW_VERSION_3_1 || !IsValid())
    {
        return false;
    }
    if (!buffer.IsValid() || buffer.BufferType() != BufferTarget::TEXTURE_BUFFER)
    {
        return false;
    }
    glTexBuffer(GL_TEXTURE_BUFFER, EnumToGL(format), (GLuint)buffer);
    assert(::glGetError() == GL_NO_ERROR);
    return ::glGetError() == GL_NO_ERROR;
}

bool glhTextureMgr::TexBufferRange(TextureFormat format, const glhBufferMgr & buffer, GLintptr offset, GLsizeiptr size)
{
    if (!GLEW_ARB_texture_buffer_range || !IsValid())
    {
        return false;
    }
    if (!buffer.IsValid() || buffer.BufferType() != BufferTarget::TEXTURE_BUFFER)
    {
        return false;
    }
    glTexBufferRange(GL_TEXTURE_BUFFER, EnumToGL(format), buffer, offset, size);
    assert(::glGetError() == GL_NO_ERROR);
    return ::glGetError() == GL_NO_ERROR;
}

bool glhTextureMgr::BindImageTexture(GLuint unit, GLint level, GLboolean layered, GLint layer, TextureAccess access, TextureFormat format)
{
    if (!GLEW_ARB_shader_image_load_store || !IsValid())
    {
        return false;
    }
    glBindImageTexture(unit, m_textureIndex, level, layered, layer, EnumToGL(access), EnumToGL(format));
    assert(::glGetError() == GL_NO_ERROR);
    return ::glGetError() == GL_NO_ERROR;
}

//------------------------------------------------------------------------------

glhGlslShader::glhGlslShader()
    : m_shader(0)
{
}

glhGlslShader::~glhGlslShader()
{
    Destroy();
}

bool glhGlslShader::Create(ShaderType type)
{
    if (!GLEW_VERSION_2_0)
    {
        return false;
    }
	m_shader = glCreateShader(EnumToGL(type));
    return !!glIsShader(m_shader);
}

bool glhGlslShader::CompileFile(ShaderType type, const wchar_t *pFileName)
{
    if (!GLEW_VERSION_2_0)
    {
        return false;
    }
    try
    {
        auto & cvtFacet = std::use_facet<std::codecvt_utf16<wchar_t> >(std::locale());
        std::wstring_convert<std::codecvt_utf16<wchar_t> > cvt{ &cvtFacet };
        boost::interprocess::file_mapping fileMap{ cvt.to_bytes(pFileName).c_str(), boost::interprocess::mode_t::read_only };
        boost::interprocess::mapped_region region{ fileMap, boost::interprocess::mode_t::read_only };
        return CompileString(type, (const GLchar*)region.get_address(), (GLint)region.get_size());
    }
    catch (const std::exception&)
    {
        assert(!"CompileFile Failed!");
        return false;
    }
}

bool glhGlslShader::CompileString(ShaderType type, const GLchar *pString, GLint nLength)
{
    if (!GLEW_VERSION_2_0)
    {
        return false;
    }
    if (nLength == 0)
    {
        nLength = (GLint)std::strlen(pString);
    }
    if (!pString || nLength == 0)
    {
        return false;
    }
    if (glIsShader(m_shader))
    {
        Destroy();
    }
    if (!Create(type))
    {
        return false;
    }
    
    //std::vector<GLint> linesLenth;
    //std::vector<const GLchar *> lines;
    //const GLchar *pCur = pString, *pLineHead = pString;
    //for (GLint i = 1; i <= nLength; ++i)
    //{
    //    if (*pCur++ == '\n'
    //        || i == nLength)
    //    {
    //        lines.push_back(pLineHead);
    //        linesLenth.push_back(pCur - pLineHead);
    //        pLineHead = pCur;
    //    }
    //}
    //assert(lines.size() == linesLenth.size());

    //glShaderSource(m_shader, (GLsizei)linesLenth.size(), &lines[0], &linesLenth[0]);
	glShaderSource(m_shader, 1, &pString, &nLength);

    glCompileShader(m_shader);
    GLint result = GL_FALSE;
    glGetShaderiv(m_shader, GL_COMPILE_STATUS, &result);
    if (result != GL_TRUE)
    {
        GLchar szBuffer[ERR_MSG_BUFFER_MAX_LENGTH] = { 0 };
        GLsizei msgLength = 0;
        glGetShaderInfoLog(m_shader, ERR_MSG_BUFFER_MAX_LENGTH, &msgLength, szBuffer);
        m_errmsg.assign(szBuffer, msgLength);
        assert(!"compile failed!");
        return false;
    }
    return true;
}

std::string glhGlslShader::GetCompileErrmsg() const
{
    return m_errmsg;
}

glhGlslShader::operator GLuint() const
{
    return m_shader;
}

void glhGlslShader::Destroy()
{
	if (GLEW_VERSION_2_0 && glIsShader(m_shader))
    {
        glDeleteShader(m_shader);
        m_shader = 0;
    }
}

//----------------------------------------------------------------------------

glhGlslProgram::glhGlslProgram()
    : m_program(0)
{
}

glhGlslProgram::~glhGlslProgram()
{
    Destroy();
}

bool glhGlslProgram::AttachShader(const glhGlslShader & shader)
{
	if (!GLEW_VERSION_2_0)
	{
		return false;
	}
    if (!glIsShader(shader))
    {
        return false;
    }
    if (!glIsProgram(m_program) && !Create())
    {
        return false;
    }
    glAttachShader(m_program, shader);
    return true;
}

bool glhGlslProgram::DetachShader(const glhGlslShader & shader)
{
    if (!GLEW_VERSION_2_0)
    {
        return false;
    }
    if (!glIsShader(shader))
    {
        return false;
    }
    if (!glIsProgram(m_program) && !Create())
    {
        return false;
    }
    glDetachShader(m_program, shader);
    return true;
}

bool glhGlslProgram::LinkProgram()
{
	if (!GLEW_VERSION_2_0)
	{
		return false;
	}
    if (!glIsProgram(m_program))
    {
        return false;
    }
    glLinkProgram(m_program);

    GLint result = GL_FALSE;
    glGetProgramiv(m_program, GL_LINK_STATUS, &result);
    if (result != GL_TRUE)
    {
        GLchar szBuffer[ERR_MSG_BUFFER_MAX_LENGTH] = { 0 };
        GLsizei msgLength = 0;
        glGetProgramInfoLog(m_program, ERR_MSG_BUFFER_MAX_LENGTH, &msgLength, szBuffer);
        m_errmsg.assign(szBuffer, msgLength);
        assert(!"link failed!");
        return false;
    }
    return true;
}

bool glhGlslProgram::UseProgram()
{
	if (!GLEW_VERSION_2_0)
	{
		return false;
	}
    if (!glIsProgram(m_program))
    {
        return false;
    }
    glUseProgram(m_program);
    return true;
}

GLuint glhGlslProgram::GetUniformBlockIndex(const char* pName)
{
    if (!GLEW_ARB_uniform_buffer_object || !glIsProgram(m_program))
    {
        return GL_INVALID_INDEX;
    }
    return glGetUniformBlockIndex(m_program, pName);
}

bool glhGlslProgram::GetUniformBlockOffsets(GLsizei uniformCount, const GLchar* const * pUniformNames, GLint* pOffsets)
{
    if (!GLEW_ARB_uniform_buffer_object || !glIsProgram(m_program) || !pUniformNames || !pOffsets)
    {
        return false;
    }
    if (0 == uniformCount)
    {
        return true;
    }
    std::vector<GLuint> indices;
    indices.assign(uniformCount, 0);
    glGetUniformIndices(m_program, uniformCount, pUniformNames, &indices[0]);
    assert(::glGetError() == GL_NO_ERROR);
    if (::glGetError() != GL_NO_ERROR)
    {
        return false;
    }
    glGetActiveUniformsiv(m_program, uniformCount, &indices[0], GL_UNIFORM_OFFSET, pOffsets);
    assert(::glGetError() == GL_NO_ERROR);
    if (::glGetError() != GL_NO_ERROR)
    {
        return false;
    }
    return true;
}

GLuint glhGlslProgram::GetAttribLocation(const GLchar * pName)
{
    if (!GLEW_VERSION_2_0)
    {
        return GL_INVALID_INDEX;
    }
    return glGetAttribLocation(m_program, pName);
}

GLuint glhGlslProgram::GetUniformLocation(const GLchar * pName)
{
    if (!GLEW_VERSION_2_0)
    {
        return GL_INVALID_INDEX;
    }
    return glGetUniformLocation(m_program, pName);
}

void glhGlslProgram::UnuseProgram()
{
    if (GLEW_VERSION_2_0)
    {
        glUseProgram(NULL);
    }
}

glhGlslProgram::operator GLuint() const
{
    return m_program;
}

std::string glhGlslProgram::GetLinkErrmsg() const
{
    return m_errmsg;
}

bool glhGlslProgram::Create()
{
    m_program = glCreateProgram();
    return !!glIsProgram(m_program);
}

void glhGlslProgram::Destroy()
{
	if (GLEW_VERSION_2_0 && glIsProgram(m_program))
    {
        glDeleteProgram(m_program);
        m_program = 0;
    }
}

bool glhGlslProgram::VertexAttribPointer(GLuint nLocation, GLint nCount, VertexAttribType type, bool bNormalize, GLsizei nStride, GLuint nOffset)
{
    if (GLEW_VERSION_2_0)
    {
        ::glVertexAttribPointer(nLocation, nCount, (GLenum)type, bNormalize, nStride, (const void*)(ptrdiff_t)nOffset);
        ::glEnableVertexAttribArray(nLocation);
        return true;
    }
    return false;
}

size_t GetGlslTypeSize(GLenum type)
{
    size_t size = 0;
#define CASE(Enum, Count, Type) \
	case Enum: size = Count * sizeof(Type); break

    switch (type) {
        CASE(GL_FLOAT, 1, GLfloat);
        CASE(GL_FLOAT_VEC2, 2, GLfloat);
        CASE(GL_FLOAT_VEC3, 3, GLfloat);
        CASE(GL_FLOAT_VEC4, 4, GLfloat);
        CASE(GL_INT, 1, GLint);
        CASE(GL_INT_VEC2, 2, GLint);
        CASE(GL_INT_VEC3, 3, GLint);
        CASE(GL_INT_VEC4, 4, GLint);
        CASE(GL_UNSIGNED_INT, 1, GLuint);
        CASE(GL_UNSIGNED_INT_VEC2, 2, GLuint);
        CASE(GL_UNSIGNED_INT_VEC3, 3, GLuint);
        CASE(GL_UNSIGNED_INT_VEC4, 4, GLuint);
        CASE(GL_BOOL, 1, GLboolean);
        CASE(GL_BOOL_VEC2, 2, GLboolean);
        CASE(GL_BOOL_VEC3, 3, GLboolean);
        CASE(GL_BOOL_VEC4, 4, GLboolean);
        CASE(GL_FLOAT_MAT2, 4, GLfloat);
        CASE(GL_FLOAT_MAT2x3, 6, GLfloat);
        CASE(GL_FLOAT_MAT2x4, 8, GLfloat);
        CASE(GL_FLOAT_MAT3, 9, GLfloat);
        CASE(GL_FLOAT_MAT3x2, 6, GLfloat);
        CASE(GL_FLOAT_MAT3x4, 12, GLfloat);
        CASE(GL_FLOAT_MAT4, 16, GLfloat);
        CASE(GL_FLOAT_MAT4x2, 8, GLfloat);
        CASE(GL_FLOAT_MAT4x3, 12, GLfloat);
#undef CASE

    default:
        break;
    }
    return size;
}

//-------------------------------------------------------------------------

bool EnableMultiSample(bool isEnable)
{
    if (isEnable)
    {
        if (::glIsEnabled(GL_MULTISAMPLE))
        {
            return true;
        }
        GLint data = 0;
        ::glGetIntegerv(GL_SAMPLE_BUFFERS, &data);
        if (data)
        {
            ::glEnable(GL_MULTISAMPLE);
            return true;
        }
        return false;
    }
    else
    {
        ::glDisable(GL_MULTISAMPLE);
        return true;
    }
}

void GetMultiSamplePos(std::vector<std::pair<GLfloat, GLfloat> >  & samplePos)
{
	samplePos.clear();
	if (!GLEW_ARB_texture_multisample)
	{
		return;
	}
	GLint count = 0;
	::glGetIntegerv(GL_SAMPLES, &count);
	if (count > 0)
	{
		samplePos.reserve(count);
		std::pair<GLfloat, GLfloat> onePos;
		for (GLint i = 0; i < count; ++i)
		{
			::glGetMultisamplefv(GL_SAMPLE_POSITION, i, (GLfloat*)&onePos);
			assert(::glGetError() == GL_NO_ERROR);
			if (::glGetError() == GL_NO_ERROR)
			{
				samplePos.push_back(onePos);
			}
		}
	}
}

SHARELIB_END_NAMESPACE

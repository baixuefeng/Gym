#pragma once
#include "GL/glew.h"
#include "GL/wglew.h"
#include "MacroDefBase.h"
#include "glEnumerations.h"
#include <vector>
#include <string>
#include <cstdint>

//----Opengl绘制过程----------------------
/* 1.每个窗口一个glWindowContext，InitOpenglWindow初始化，只做一次；
   2.每个绘制线程InitThreadRC，绘制。线程退出之前ClearThreadRC；
   3.所有线程绘制完成后，SwapBuffers(双缓冲)
   注：InitThreadRC之后 glGetString(GL_VERSION)获取opengl的版本，glewGetString(GLEW_VERSION)
      获取glew的版本，通过 GLEW_VERSION_3_3 之类的宏可以判断当前的opengl是否支持该版本，如：GLEW_VERSION_3_3
      非空就表示当前的opengl版本支持3.3版本。
      不同的窗口pixelformat设置也会影响支持的opengl版本。
*/

SHARELIB_BEGIN_NAMESPACE

struct glhGuardAttrib
{
	explicit glhGuardAttrib(GLbitfield mask = ~0){ ::glPushAttrib(mask); }
	~glhGuardAttrib(){ ::glPopAttrib(); }
};

struct glhGuardMatrix
{
    glhGuardMatrix(){ ::glPushMatrix(); }
    ~glhGuardMatrix(){ ::glPopMatrix(); }
};

//-------------------------------------------------------------------
//rendering DC 管理类
class glhWindowContext 
{
    SHARELIB_DISABLE_COPY_CLASS(glhWindowContext);
public:
	glhWindowContext();
	~glhWindowContext();
	bool InitOpenglWindow(HWND hWnd);

    //获取模板缓存位宽
    uint8_t GetStencilBufferBit();
    uint8_t GetDepthBufferBit();

	//这两个是线程安全的。必须配对使用
	bool InitThreadRC();
	bool UseDebugRC();
	bool IsValid() const;
	void ClearThreadRC();

	//等所有线程都绘制完成后，在InitOpenglWindow的线程里调用
	bool SwapBuffers();

private:
	void Destroy();
    bool ConfigRC();
	
	HWND m_hWnd;
	HDC m_hDc;
};

//-------------------------------------------------------------------
//VertextArray管理类
class glhVertextArrayMgr
{
    SHARELIB_DISABLE_COPY_CLASS(glhVertextArrayMgr);
public:
    glhVertextArrayMgr();
    ~glhVertextArrayMgr();
    bool Generate(GLsizei n);
    bool Bind(size_t nBufferIndex);

private:
    void Delete();

    std::vector<GLuint> m_array;
};

//-------------------------------------------------------------------
//Buffers管理类
class glhBufferMgr
{
    SHARELIB_DISABLE_COPY_CLASS(glhBufferMgr);
public:
    glhBufferMgr();
    ~glhBufferMgr();

    /** 生成特定类型用途的buffer
    @param[in] target 绑定类型
    @param[in] bind 是否绑定，如uniform类型的不能绑定，buffer类型的需要绑定
    */
    bool Create(BufferTarget target, bool bind);

    /** buffer索引
    */
    operator GLuint() const;

    /** 获取buffer类型
    */
    BufferTarget BufferType() const;

    /** buffer是否有效
    */
    bool IsValid() const;

    /** 删除buffer
    */
    void Delete();

    //----与着色器关联，先关联，再操作，否则可能出错，比如uniform类型，不关联的情况下用BufferData写入会出错
    //---------------------------------------------------------------

    /** 绑定buffer到glsl目标缓冲区
    @param[in] ntargetIndex 缓冲区索引
    @param[in] offset 偏移
    @param[in] size 大小
    */
    bool BindBufferRange(GLuint ntargetIndex, GLintptr offset, GLsizeiptr size);

    /** 与BindBufferRange类似，只不过是把整个buffer绑定上去
    */
    bool BindBufferBase(GLuint ntargetIndex);

    //----buffer操作--------------------------------------------------

    /** 往buffer中写入数据
    @param[in] size 大小
    @param[in] pData 源指针，如果为nullptr表示不写入数据，只开辟空间
    @param[in] usage 用途
    */
    bool BufferData(GLsizeiptr size, const void *pData, BufferUsage usage);

    /** 往buffer中写入数据
    @param[in] offset 偏移
    @param[in] size 大小
    @param[in] pData 源指针
    */
    bool BufferSubData(GLintptr offset, GLsizeiptr size, const void *pData);

    /** 在buffer之间copy
    @param[in] readTarget 读取目标
    @param[in] writeTarget 写入目标
    @param[in] readOffset 读取偏移
    @param[in] writeOffset 写入偏移
    @param[in] size 大小
    */
    bool CopyBufferSubData(BufferTarget readTarget, BufferTarget writeTarget, GLintptr readOffset, GLintptr writeOffset, GLsizeiptr size);

    /** 映射buffer到内存
    @param[in] offset 偏移
    @param[in] length 长度
    @param[in] access 权限
    */
    void * MapBufferRange(GLintptr offset, GLsizeiptr length, BufferAccessBit access);

    /** 与MapBufferRange类似，映射整个buffer。SHADER_STORAGE_BUFFER类型的buffer不适用。
    */
    void * MapBuffer(BufferAccessBit access);

    /** 刷新映射的buffer区域
    @param[in] target 目标
    @param[in] offset 偏移
    @param[in] length 长度
    */
    bool FlushMappedBufferRange(GLintptr offset, GLsizeiptr length);

    /** 取消映射
    @param[in] target 目标
    */
    bool UnmapBuffer();

    /**
    @param[in] target
    @param[in] param
    @return
    */
    GLint GetBufferInfo(BufferTarget target, BufferParameter param);

private:
    //buffer索引
    GLuint m_bufferIndex;

    //绑定类型
    GLenum m_target;
};

//-------------------------------------------------------------------------

class glhTextureMgr
{
    SHARELIB_DISABLE_COPY_CLASS(glhTextureMgr);
public:
    glhTextureMgr();
    ~glhTextureMgr();

    /** 生成特定类型的texture
    @param[in] target 绑定类型
    */
    bool Create(TextureTarget target);

    /** texture索引
    */
    operator GLuint() const;

    /** 获取texture类型
    */
    TextureTarget TextureType() const;

    /** texture是否有效
    */
    bool IsValid() const;

    /** 删除buffer
    */
    void Delete();

    /** 设置texture的格式及大小
    @param[in] levels >=0
    @param[in] format 
    @param[in] width
    @param[in] height
    @param[in] depth
    */
    bool TexStorage1D(GLsizei levels, TextureFormat format, GLsizei width);
    bool TexStorage2D(GLsizei levels, TextureFormat format, GLsizei width, GLsizei height);
    bool TexStorage3D(GLsizei levels, TextureFormat format, GLsizei width, GLsizei height, GLsizei depth);
    bool TexStorage2DMultisample(GLsizei levels, TextureFormat format, GLsizei width, GLsizei height, GLboolean fixedsamplelocations);
    bool TexStorage3DMultisample(GLsizei levels, TextureFormat format, GLsizei width, GLsizei height, GLsizei depth, GLboolean fixedsamplelocations);

    /** 关联buffer，将其中的数据存入texture
    */
    bool TexBuffer(TextureFormat format, const glhBufferMgr & buffer);
    bool TexBufferRange(TextureFormat format, const glhBufferMgr & buffer, GLintptr offset, GLsizeiptr size);

    bool BindImageTexture(GLuint unit, GLint level, GLboolean layered, GLint layer, TextureAccess access, TextureFormat format);
private:
    //texture索引
    GLuint m_textureIndex;

    //绑定类型
    GLenum m_target;
};

//-------------------------------------------------------------------------
//GLSL 编译辅助类
class glhGlslShader
{
    SHARELIB_DISABLE_COPY_CLASS(glhGlslShader);
public:
    glhGlslShader();
    ~glhGlslShader();

	/** 编译glsl shader文件
	@param[in] type shader类型
	@param[in] pFileName 文件名
	@return 编译是否成功
	*/
	bool CompileFile(ShaderType type, const wchar_t *pFileName);

	/** 编译glsl shader字符串
	@param[in] type shader类型
	@param[in] pString shader字符串
	@param[in] nLength shader字符串长度，0表示不明确指定，以\0为结尾
	@return 编译是否成功
	*/
	bool CompileString(ShaderType type, const char *pString, GLint nLength);
    
    /** 获取编译错误提示
    */
    std::string GetCompileErrmsg() const;
    
    /** shader类型
    */
    operator GLuint() const;

    /** 销毁shader
    */
    void Destroy();

private:
	bool Create(ShaderType type);

    std::string m_errmsg;
    GLuint m_shader;
};

//-------------------------------------------------------------------------
//glsl shader程序
class glhGlslProgram
{
    SHARELIB_DISABLE_COPY_CLASS(glhGlslProgram);
public:
    glhGlslProgram();
    ~glhGlslProgram();

    /** 关联编译好的shader
    @param[in] shader 成功编译的shader
    */
    bool AttachShader(const glhGlslShader & shader);

    /** 解除与shader的关联
    @param[in] shader shader
    */
    bool DetachShader(const glhGlslShader & shader);

    /** 链接程序
    */
    bool LinkProgram();

    /** 在当前线程环境中使用shader程序
    */
    bool UseProgram();

    /** 获取glsl中uniform block的索引
    @param[in] pName uniform块的名字
    @return 索引，有错误则返回 GL_INVALID_INDEX
    */
    GLuint GetUniformBlockIndex(const char* pName);

    /** 获取uniform block中变量的内存偏移量
    @param[in] uniformCount 变量个数
    @param[in] uniformNames 变量名字数组
    @param[out] offsets 变量的内存偏移量
    @return 操作是否成功
    */
    bool GetUniformBlockOffsets(GLsizei uniformCount, const GLchar* const * pUniformNames, GLint* pOffsets);

    GLuint GetAttribLocation(const GLchar * pName);
    GLuint GetUniformLocation(const GLchar * pName);

    void UnuseProgram();
    std::string GetLinkErrmsg() const;
    operator GLuint() const;
    void Destroy();

    static bool VertexAttribPointer(GLuint nLocation, GLint nCount, VertexAttribType type, bool bNormalize, GLsizei nStride, GLuint nOffset);
private:
    bool Create();

    std::string m_errmsg;
    GLuint m_program;
};

//-------------------------------------------------------------------------

/** 获取glsl中的类型大小
@param[in] type glsl中的类型
@return 大小，失败返回0
*/
size_t GetGlslTypeSize(GLenum type);

bool EnableMultiSample(bool isEnable);

void GetMultiSamplePos(std::vector<std::pair<GLfloat, GLfloat> > & samplePos);

SHARELIB_END_NAMESPACE

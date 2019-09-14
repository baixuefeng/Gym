﻿#pragma once
#include <type_traits>
#include "MacroDefBase.h"
#include "GL/glew.h"


SHARELIB_BEGIN_NAMESPACE

//转换enum到GL值
template<typename EnumType>
std::underlying_type_t<EnumType> EnumToGL(EnumType value)
{
    return (std::underlying_type_t<EnumType>)value;
}

//glGetString
enum class StringType : GLenum
{
    VENDOR                   = GL_VENDOR,
    RENDERER                 = GL_RENDERER,
    VERSION                  = GL_VERSION,
    SHADING_LANGUAGE_VERSION = GL_SHADING_LANGUAGE_VERSION,
};

//glGetError
enum class ErrorType : GLenum
{
	INVALID_ENUM                  = GL_INVALID_ENUM,                  //0x0500
	INVALID_VALUE                 = GL_INVALID_VALUE,                 //0x0501
	INVALID_OPERATION             = GL_INVALID_OPERATION,             //0x0502
	INVALID_FRAMEBUFFER_OPERATION = GL_INVALID_FRAMEBUFFER_OPERATION, //0x0506
	OUT_OF_MEMORY                 = GL_OUT_OF_MEMORY,                 //0x0505
	STACK_UNDERFLOW               = GL_STACK_UNDERFLOW,               //0x0504
	STACK_OVERFLOW                = GL_STACK_OVERFLOW                 //0x0503
};

//glBindBuffer
enum class BufferTarget : GLenum
{
	ARRAY_BUFFER              = GL_ARRAY_BUFFER,
	ATOMIC_COUNTER_BUFFER     = GL_ATOMIC_COUNTER_BUFFER,
	COPY_READ_BUFFER          = GL_COPY_READ_BUFFER,
	COPY_WRITE_BUFFER         = GL_COPY_WRITE_BUFFER,
	DISPATCH_INDIRECT_BUFFER  = GL_DISPATCH_INDIRECT_BUFFER,
	DRAW_INDIRECT_BUFFER      = GL_DRAW_INDIRECT_BUFFER,
	ELEMENT_ARRAY_BUFFER      = GL_ELEMENT_ARRAY_BUFFER,
	PIXEL_PACK_BUFFER         = GL_PIXEL_PACK_BUFFER,
	PIXEL_UNPACK_BUFFER       = GL_PIXEL_UNPACK_BUFFER,
	QUERY_BUFFER              = GL_QUERY_BUFFER,
	SHADER_STORAGE_BUFFER     = GL_SHADER_STORAGE_BUFFER,
	TEXTURE_BUFFER            = GL_TEXTURE_BUFFER,
	TRANSFORM_FEEDBACK_BUFFER = GL_TRANSFORM_FEEDBACK_BUFFER,
	UNIFORM_BUFFER            = GL_UNIFORM_BUFFER
};

//glBufferData
enum class BufferUsage : GLenum
{
	STREAM_DRAW  = GL_STREAM_DRAW,
	STREAM_READ  = GL_STREAM_READ,
	STREAM_COPY  = GL_STREAM_COPY,
	STATIC_DRAW  = GL_STATIC_DRAW,
	STATIC_READ  = GL_STATIC_READ,
	STATIC_COPY  = GL_STATIC_COPY,
	DYNAMIC_DRAW = GL_DYNAMIC_DRAW,
	DYNAMIC_READ = GL_DYNAMIC_READ,
	DYNAMIC_COPY = GL_DYNAMIC_COPY
};

//glMapBuffer
enum class BufferAccess : GLenum
{
	READ_ONLY  = GL_READ_ONLY,
	WRITE_ONLY = GL_WRITE_ONLY,
	READ_WRITE = GL_READ_WRITE
};

//glMapBufferRange
enum class BufferAccessBit : GLbitfield
{
	MAP_READ_BIT              = GL_MAP_READ_BIT,
	MAP_WRITE_BIT             = GL_MAP_WRITE_BIT,
	MAP_PERSISTENT_BIT        = GL_MAP_PERSISTENT_BIT,
	MAP_COHERENT_BIT          = GL_MAP_COHERENT_BIT,
	MAP_INVALIDATE_RANGE_BIT  = GL_MAP_INVALIDATE_RANGE_BIT,
	MAP_INVALIDATE_BUFFER_BIT = GL_MAP_INVALIDATE_BUFFER_BIT,
	MAP_FLUSH_EXPLICIT_BIT    = GL_MAP_FLUSH_EXPLICIT_BIT,
	MAP_UNSYNCHRONIZED_BIT    = GL_MAP_UNSYNCHRONIZED_BIT
};

enum class BufferParameter : GLenum
{
    BUFFER_ACCESS            = GL_BUFFER_ACCESS,
    BUFFER_ACCESS_FLAGS      = GL_BUFFER_ACCESS_FLAGS,
    BUFFER_IMMUTABLE_STORAGE = GL_BUFFER_IMMUTABLE_STORAGE,
    BUFFER_MAPPED            = GL_BUFFER_MAPPED,
    BUFFER_MAP_LENGTH        = GL_BUFFER_MAP_LENGTH,
    BUFFER_MAP_OFFSET        = GL_BUFFER_MAP_OFFSET,
    BUFFER_SIZE              = GL_BUFFER_SIZE,
    BUFFER_STORAGE_FLAGS     = GL_BUFFER_STORAGE_FLAGS,
    BUFFER_USAGE             = GL_BUFFER_USAGE,
};

//glCreateShader
enum class ShaderType : GLenum
{
	COMPUTE_SHADER         = GL_COMPUTE_SHADER,
	VERTEX_SHADER          = GL_VERTEX_SHADER,
	TESS_CONTROL_SHADER    = GL_TESS_CONTROL_SHADER,
	TESS_EVALUATION_SHADER = GL_TESS_EVALUATION_SHADER,
	GEOMETRY_SHADER        = GL_GEOMETRY_SHADER,
	FRAGMENT_SHADER        = GL_FRAGMENT_SHADER
};

//glDrawArrays
enum class DrawMode : GLenum
{
	POINTS                   = GL_POINTS,
	LINE_STRIP               = GL_LINE_STRIP,
	LINE_LOOP                = GL_LINE_LOOP,
	LINES                    = GL_LINES,
	LINE_STRIP_ADJACENCY     = GL_LINE_STRIP_ADJACENCY,
	LINES_ADJACENCY          = GL_LINES_ADJACENCY,
	TRIANGLE_STRIP           = GL_TRIANGLE_STRIP,
	TRIANGLE_FAN             = GL_TRIANGLE_FAN,
	TRIANGLES                = GL_TRIANGLES,
	TRIANGLE_STRIP_ADJACENCY = GL_TRIANGLE_STRIP_ADJACENCY,
	TRIANGLES_ADJACENCY      = GL_TRIANGLES_ADJACENCY,
	PATCHES                  = GL_PATCHES
};

//glBindTexture
enum class TextureTarget : GLenum
{
    TEXTURE_1D                   = GL_TEXTURE_1D,
    TEXTURE_2D                   = GL_TEXTURE_2D,
    TEXTURE_3D                   = GL_TEXTURE_3D,
    TEXTURE_1D_ARRAY             = GL_TEXTURE_1D_ARRAY,
    TEXTURE_2D_ARRAY             = GL_TEXTURE_2D_ARRAY,
    TEXTURE_RECTANGLE            = GL_TEXTURE_RECTANGLE,
    TEXTURE_CUBE_MAP             = GL_TEXTURE_CUBE_MAP,
    TEXTURE_CUBE_MAP_ARRAY       = GL_TEXTURE_CUBE_MAP_ARRAY,
    TEXTURE_2D_MULTISAMPLE       = GL_TEXTURE_2D_MULTISAMPLE,
    TEXTURE_2D_MULTISAMPLE_ARRAY = GL_TEXTURE_2D_MULTISAMPLE_ARRAY
};

//glBindImageTexture
enum class TextureAccess : GLenum
{
    READ_ONLY  = GL_READ_ONLY,
    WRITE_ONLY = GL_WRITE_ONLY,
    READ_WRITE = GL_READ_WRITE
};

//glTexStorage  glTextureStorage  glBindImageTexture
enum class TextureFormat : GLenum
{
    R8             = GL_R8,
    R8_SNORM       = GL_R8_SNORM,
    R16            = GL_R16,
    R16_SNORM      = GL_R16_SNORM,
    RG8            = GL_RG8,
    RG8_SNORM      = GL_RG8_SNORM,
    RG16           = GL_RG16,
    RG16_SNORM     = GL_RG16_SNORM,
    R3_G3_B2       = GL_R3_G3_B2,
    RGB4           = GL_RGB4,
    RGB5           = GL_RGB5,
    RGB8           = GL_RGB8,
    RGB8_SNORM     = GL_RGB8_SNORM,
    RGB10          = GL_RGB10,
    RGB12          = GL_RGB12,
    RGB16_SNORM    = GL_RGB16_SNORM,
    RGBA2          = GL_RGBA2,
    RGBA4          = GL_RGBA4,
    RGB5_A1        = GL_RGB5_A1,
    RGBA8          = GL_RGBA8,
    RGBA8_SNORM    = GL_RGBA8_SNORM,
    RGB10_A2       = GL_RGB10_A2,
    RGB10_A2UI     = GL_RGB10_A2UI,
    RGBA12         = GL_RGBA12,
    RGBA16         = GL_RGBA16,
    SRGB8          = GL_SRGB8,
    SRGB8_ALPHA8   = GL_SRGB8_ALPHA8,
    R16F           = GL_R16F,
    RG16F          = GL_RG16F,
    RGB16F         = GL_RGB16F,
    RGBA16F        = GL_RGBA16F,
    R32F           = GL_R32F,
    RG32F          = GL_RG32F,
    RGB32F         = GL_RGB32F,
    RGBA32F        = GL_RGBA32F,
    R11F_G11F_B10F = GL_R11F_G11F_B10F,
    RGB9_E5        = GL_RGB9_E5,
    R8I            = GL_R8I,
    R8UI           = GL_R8UI,
    R16I           = GL_R16I,
    R16UI          = GL_R16UI,
    R32I           = GL_R32I,
    R32UI          = GL_R32UI,
    RG8I           = GL_RG8I,
    RG8UI          = GL_RG8UI,
    RG16I          = GL_RG16I,
    RG16UI         = GL_RG16UI,
    RG32I          = GL_RG32I,
    RG32UI         = GL_RG32UI,
    RGB8I          = GL_RGB8I,
    RGB8UI         = GL_RGB8UI,
    RGB16I         = GL_RGB16I,
    RGB16UI        = GL_RGB16UI,
    RGB32I         = GL_RGB32I,
    RGB32UI        = GL_RGB32UI,
    RGBA8I         = GL_RGBA8I,
    RGBA8UI        = GL_RGBA8UI,
    RGBA16I        = GL_RGBA16I,
    RGBA16UI       = GL_RGBA16UI,
    RGBA32I        = GL_RGBA32I,
    RGBA32UI       = GL_RGBA32UI
};

//glDrawElements
enum class DrawElementsType : GLenum
{
	UNSIGNED_BYTE  = GL_UNSIGNED_BYTE,
	UNSIGNED_SHORT = GL_UNSIGNED_SHORT,
	UNSIGNED_INT   = GL_UNSIGNED_INT
};

//glVertexAttribPointer
enum class VertexAttribType : GLenum
{
	BYTE                         = GL_BYTE,
	UNSIGNED_BYTE                = GL_UNSIGNED_BYTE,
	SHORT                        = GL_SHORT,
	UNSIGNED_SHORT               = GL_UNSIGNED_SHORT,
	INT                          = GL_INT,
	UNSIGNED_INT                 = GL_UNSIGNED_INT,

	//下面这些 glVertexAttribPointer 可以用，glVertexAttribIPointer不可以
	HALF_FLOAT                   = GL_HALF_FLOAT,
	FLOAT                        = GL_FLOAT,
	DOUBLE                       = GL_DOUBLE,
	FIXED                        = GL_FIXED,
	INT_2_10_10_10_REV           = GL_INT_2_10_10_10_REV,
	UNSIGNED_INT_2_10_10_10_REV  = GL_UNSIGNED_INT_2_10_10_10_REV,
	UNSIGNED_INT_10F_11F_11F_REV = GL_UNSIGNED_INT_10F_11F_11F_REV
};

//glClear glBlitFramebuffer
enum class BufferBitMask : GLenum
{
	COLOR_BUFFER_BIT   = GL_COLOR_BUFFER_BIT,
	DEPTH_BUFFER_BIT   = GL_DEPTH_BUFFER_BIT,
	STENCIL_BUFFER_BIT = GL_STENCIL_BUFFER_BIT
};

//glStencilMaskSeparate
enum class FaceType : GLenum
{
	FRONT          = GL_FRONT,
	BACK           = GL_BACK,
	FRONT_AND_BACK = GL_FRONT_AND_BACK
};

//glStencilFunc  glDepthFunc
enum class TestFuncType : GLenum
{
	NEVER    = GL_NEVER,
	LESS     = GL_LESS,
	LEQUAL   = GL_LEQUAL,
	GREATER  = GL_GREATER,
	GEQUAL   = GL_GEQUAL,
	EQUAL    = GL_EQUAL,
	NOTEQUAL = GL_NOTEQUAL,
	ALWAYS   = GL_ALWAYS
};

//glStencilOp
enum class StencilOpType : GLenum
{
	KEEP      = GL_KEEP,
	ZERO      = GL_ZERO,
	REPLACE   = GL_REPLACE,
	INCR      = GL_INCR,
	INCR_WRAP = GL_INCR_WRAP,
	DECR      = GL_DECR,
	DECR_WRAP = GL_DECR_WRAP,
	INVERT    = GL_INVERT
};

//glBlendFunc
enum class BlendFuncType : GLenum
{
	ZERO                     = GL_ZERO,
	ONE                      = GL_ONE,
	SRC_COLOR                = GL_SRC_COLOR,
	ONE_MINUS_SRC_COLOR      = GL_ONE_MINUS_SRC_COLOR,
	DST_COLOR                = GL_DST_COLOR,
	ONE_MINUS_DST_COLOR      = GL_ONE_MINUS_DST_COLOR,
	SRC_ALPHA                = GL_SRC_ALPHA,
	ONE_MINUS_SRC_ALPHA      = GL_ONE_MINUS_SRC_ALPHA,
	DST_ALPHA                = GL_DST_ALPHA,
	ONE_MINUS_DST_ALPHA      = GL_ONE_MINUS_DST_ALPHA,
	CONSTANT_COLOR           = GL_CONSTANT_COLOR,
	ONE_MINUS_CONSTANT_COLOR = GL_ONE_MINUS_CONSTANT_COLOR,
	CONSTANT_ALPHA           = GL_CONSTANT_ALPHA,
	ONE_MINUS_CONSTANT_ALPHA = GL_ONE_MINUS_CONSTANT_ALPHA,
	SRC_ALPHA_SATURATE       = GL_SRC_ALPHA_SATURATE,
	SRC1_COLOR               = GL_SRC1_COLOR,
	ONE_MINUS_SRC1_COLOR     = GL_ONE_MINUS_SRC1_COLOR,
	SRC1_ALPHA               = GL_SRC1_ALPHA,
	ONE_MINUS_SRC1_ALPHA     = GL_ONE_MINUS_SRC1_ALPHA
};

//glBlendEquation
enum class BlendEquationType : GLenum
{
	FUNC_ADD              = GL_FUNC_ADD,
	FUNC_SUBTRACT         = GL_FUNC_SUBTRACT,
	FUNC_REVERSE_SUBTRACT = GL_FUNC_REVERSE_SUBTRACT,
	MIN                   = GL_MIN,
	MAX                   = GL_MAX
};

//glLogicOp
enum class LogicOpType : GLenum
{
	CLEAR         = GL_CLEAR,        //0
	SET           = GL_SET,          //1
	COPY          = GL_COPY,         //s
	COPY_INVERTED = GL_COPY_INVERTED,//~s
	NOOP          = GL_NOOP,         //d
	INVERT        = GL_INVERT,       //~d
	AND           = GL_AND,          //s & d
	NAND          = GL_NAND,         //~(s & d)
	OR            = GL_OR,           //s | d
	NOR           = GL_NOR,          //~(s | d)
	XOR           = GL_XOR,          //s ^ d
	EQUIV         = GL_EQUIV,        //~(s ^ d)
	AND_REVERSE   = GL_AND_REVERSE,  //s & ~d
	AND_INVERTED  = GL_AND_INVERTED, //~s & d
	OR_REVERSE    = GL_OR_REVERSE,   //s | ~d
	OR_INVERTED   = GL_OR_INVERTED	 //~s | d
};

//glEnable
enum class SmoothType : GLenum
{
	LINE_SMOOTH    = GL_LINE_SMOOTH,
	POLYGON_SMOOTH = GL_POLYGON_SMOOTH
};

//glHint
enum class HintTargetType : GLenum
{
	LINE_SMOOTH_HINT                = GL_LINE_SMOOTH_HINT,
	POLYGON_SMOOTH_HINT             = GL_POLYGON_SMOOTH_HINT,
	TEXTURE_COMPRESSION_HINT        = GL_TEXTURE_COMPRESSION_HINT,
	FRAGMENT_SHADER_DERIVATIVE_HINT = GL_FRAGMENT_SHADER_DERIVATIVE_HINT
};

//glHint
enum class HintModeType : GLenum
{
	FASTEST   = GL_FASTEST,
	NICEST    = GL_NICEST,
	DONT_CARE = GL_DONT_CARE
};

//glBindFramebuffer, glFramebufferRenderbuffer
enum class FramebufferTarget : GLenum
{
	DRAW_FRAMEBUFFER = GL_DRAW_FRAMEBUFFER,
	READ_FRAMEBUFFER = GL_READ_FRAMEBUFFER,
	FRAMEBUFFER      = GL_FRAMEBUFFER
};

//glFramebufferRenderbuffer
enum class FramebufferAttachment : GLenum
{
	DEPTH_ATTACHMENT         = GL_DEPTH_ATTACHMENT,
	STENCIL_ATTACHMENT       = GL_STENCIL_ATTACHMENT,
	DEPTH_STENCIL_ATTACHMENT = GL_DEPTH_STENCIL_ATTACHMENT,
	COLOR_ATTACHMENT_BEGIN   = GL_COLOR_ATTACHMENT0  //(0 ~ GL_MAX_COLOR_ATTACHMENTS - 1)
};

//glBlitFramebuffer
enum class BlitFrameFilter : GLenum
{
	NEAREST = GL_NEAREST,
	LINEAR  = GL_LINEAR   // only a valid interpolation method for the color buffer
};

//glClearBuffer
enum class ClearBufferType : GLenum
{
	COLOR   = GL_COLOR,
	DEPTH   = GL_DEPTH,
	STENCIL = GL_STENCIL
};

SHARELIB_END_NAMESPACE
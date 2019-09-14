#pragma once
#include <cstdint>
#include "MacroDefBase.h"

SHARELIB_BEGIN_NAMESPACE

#ifndef COLOR32_ARGB

typedef uint32_t COLOR32;
#define COLOR32_ARGB(a,r,g,b) ((uint32_t)( \
                                           ( ((uint32_t)((uint8_t)(a))) << 24 ) \
                                         | ( ((uint32_t)((uint8_t)(r))) << 16 ) \
                                         | ( ((uint16_t)((uint8_t)(g))) << 8 ) \
                                         | ((uint8_t)(b)) \
                                         ))

#define COLOR32_RGB(r,g,b) COLOR32_ARGB(255,r,g,b)

#define COLOR32_GetA(argb) ((uint8_t)( (((uint32_t)(argb)) & (uint32_t)0xFF000000) >> 24 ))
#define COLOR32_GetR(argb) ((uint8_t)( (((uint32_t)(argb)) & (uint32_t)0x00FF0000) >> 16 ))
#define COLOR32_GetG(argb) ((uint8_t)( (((uint32_t)(argb)) & (uint32_t)0x0000FF00) >> 8 ))
#define COLOR32_GetB(argb) ((uint8_t)( (((uint32_t)(argb)) & (uint32_t)0x000000FF) ))

#define COLOR32_SetA(argb,a) ( ((argb) &= (uint32_t)0x00FFFFFF) |= ( ((uint32_t)((uint8_t)(a))) << 24 ) )
#define COLOR32_SetR(argb,r) ( ((argb) &= (uint32_t)0xFF00FFFF) |= ( ((uint32_t)((uint8_t)(r))) << 16 ) )
#define COLOR32_SetG(argb,g) ( ((argb) &= (uint32_t)0xFFFF00FF) |= ( ((uint32_t)((uint8_t)(g))) << 8 ) )
#define COLOR32_SetB(argb,b) ( ((argb) &= (uint32_t)0xFFFFFF00) |=  ((uint8_t)(b)) )

//如果alpha为0，且其它分量不全为0，则修正alpha为255
#define VERIFY_COLOR32(argb) ( !COLOR32_GetA(argb) && ((argb) & (uint32_t)0x00FFFFFF) ? \
                                ((argb) | (uint32_t)0xFF000000) : (argb) )

#endif // !COLOR32_ARGB

SHARELIB_END_NAMESPACE

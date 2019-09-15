#pragma once
#include <cstdint>
#pragma warning(disable : 4201)
#include "glm/glm.hpp"

namespace ShaderUtility {
using glm::uint;
using glm::uvec2;
using glm::ivec2;
using glm::uvec4;

/** 从xmrig里复制过来的 64位无符号乘法
@param[in] multiplier 乘数1
@param[in] multiplicand 乘数2
@param[in] product_hi 128位结果的高64位
@return 128位结果的低64位
*/
uint64_t u128_mul(uint64_t multiplier, uint64_t multiplicand, uint64_t *product_hi);

/** 64位无符号加法
@param[in] var1 加数，x为低32位，y为高32位
@param[in] var2 加数，x为低32位，y为高32位
@return 64位结果，x为低32位，y为高32位
*/
uvec2 u64_add(uvec2 var1, uvec2 var2);

/** 64位无符号减法
@param[in] var1 x为低32位，y为高32位
@param[in] var2 x为低32位，y为高32位
@return 64位结果，x为低32位，y为高32位
*/
uvec2 u64_sub(uvec2 var1, uvec2 var2);

/** 64位无符号乘法，返回128位结果
@param[in] var1 64位乘数，x为低32位，y为高32位
@param[in] var2 64位乘数，x为低32位，y为高32位
@return 128位结果，从低位到高位依次为 x,y,z,w
*/
uvec4 u128_mul(uvec2 var1, uvec2 var2);

/** 64数无符号除法，除数固定为32位
@param[in] var1 64位被除数，x为低32位，y为高32位
@param[in] i 32位除数
@return 64位结果，x为低32位，y为高32位
*/
uvec2 u64_divide(uvec2 var1, uint i);

/** 64位有符号数取负
@param[in] var 64位有符号数，x为低32位，y为高32位
@return 取负之后的结果，x为低32位，y为高32位
*/
ivec2 i64_negative(ivec2 var);

/** 64数有符号除法，除数固定为32位
@param[in] var1 64位被除数，x为低32位，y为高32位
@param[in] i 32位除数
@return 64位结果，x为低32位，y为高32位
*/
ivec2 i64_divide(ivec2 var1, int i);

} // namespace ShaderUtility

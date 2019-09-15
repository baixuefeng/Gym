#include "OpenGL/ShaderUtility.h"

namespace ShaderUtility {

#define UINT_BITS 32

using glm::umulExtended;
using glm::uaddCarry;
using glm::abs;
using glm::sign;
using glm::findMSB;
using glm::findLSB;

uint64_t u128_mul(uint64_t multiplier, uint64_t multiplicand, uint64_t *product_hi)
{
    // multiplier   = ab = a * 2^32 + b
    // multiplicand = cd = c * 2^32 + d
    // ab * cd = a * c * 2^64 + (a * d + b * c) * 2^32 + b * d
    uint64_t a = multiplier >> 32;
    uint64_t b = multiplier & 0xFFFFFFFF;
    uint64_t c = multiplicand >> 32;
    uint64_t d = multiplicand & 0xFFFFFFFF;

    //uint64_t ac = a * c;
    uint64_t ad = a * d;
    //uint64_t bc = b * c;
    uint64_t bd = b * d;

    uint64_t adbc = ad + (b * c);
    uint64_t adbc_carry = adbc < ad ? 1 : 0;

    // multiplier * multiplicand = product_hi * 2^64 + product_lo
    uint64_t product_lo = bd + (adbc << 32);
    uint64_t product_lo_carry = product_lo < bd ? 1 : 0;
    *product_hi = (a * c) + (adbc >> 32) + (adbc_carry << 32) + product_lo_carry;

    return product_lo;
}

uvec2 u64_add(uvec2 var1, uvec2 var2)
{
    uvec2 res = uvec2(0);
    uint carry = 0;
    res.x = uaddCarry(var1.x, var2.x, carry);
    res.y = var1.y + var2.y + carry;
    return res;
}

uvec2 u64_sub(uvec2 var1, uvec2 var2)
{
    uvec2 res = uvec2(0);
    res.x = var1.x - var2.x;
    res.y = var1.y - var2.y;
    if (var1.x < var2.x)
    {
        res.y -= 1;
    }
    return res;
}

uvec4 u128_mul(uvec2 var1, uvec2 var2)
{
    uvec2 ac = uvec2(0), bc = uvec2(0), ad = uvec2(0), bd = uvec2(0);
    umulExtended(var1.x, var2.x, bd.y, bd.x);
    umulExtended(var1.y, var2.x, ad.y, ad.x);
    umulExtended(var1.x, var2.y, bc.y, bc.x);
    umulExtended(var1.y, var2.y, ac.y, ac.x);

    uvec4 res = uvec4(0);

    res.x = bd.x;

    uint carry_all = 0;
    uint carry = 0;
    {
        res.y = uaddCarry(bd.y, ad.x, carry);
        carry_all += carry;
        res.y = uaddCarry(res.y, bc.x, carry);
        carry_all += carry;
    }

    {
        res.z = uaddCarry(ac.x, carry_all, carry_all);
        res.z = uaddCarry(res.z, ad.y, carry);
        carry_all += carry;
        res.z = uaddCarry(res.z, bc.y, carry);
        carry_all += carry;
    }

    res.w = ac.y + carry_all;
    return res;
}

uvec2 u64_divide(uvec2 var1, uint i)
{
    uvec2 res = uvec2(0);

    {
        uint lowzero = findLSB(i);
        if (lowzero > 0)
        {
            //除数中低位的0直接移位去掉
            i >>= lowzero;
            uint high = var1.y;
            var1 >>= lowzero;
            var1.x |= high << (UINT_BITS - lowzero);
        }
        if (1 == i)
        {
            return var1;
        }
        if (0 == var1.y)
        {
            res.x = var1.x / i;
            return res;
        }
    }

    res.y = var1.y / i;
    //余数
    uint remain = var1.y % i;
    if (0 == remain)
    {
        res.x = var1.x / i;
        return res;
    }

    //余数位数
    uint remainBits = findMSB(remain) + 1;
    //余数增加位数是否会溢出
    bool isOverflow = (UINT_BITS == remainBits);
    if (isOverflow)
    {
        --remainBits;
    }
    //增补位数
    uint addtionalBits = UINT_BITS - remainBits;
    //剩余未参与运算的位数
    uint leftBit = remainBits;
    //中间步骤的被除数
    uint midVar = 0;

    do
    {
        //余数放到高位，低位从后序位增补。先左移高位清0，再右移
        midVar = remain << addtionalBits;
        midVar |= (var1.x << (UINT_BITS - addtionalBits - leftBit)) >> (UINT_BITS - addtionalBits);
        if (isOverflow)
        {
            //商1，余数直接减
            res.x |= (1 << leftBit);
            remain = midVar - i;
        }
        else
        {
            res.x |= (midVar / i) << leftBit;
            remain = midVar % i;
        }

        if (0 == remain)
        {
            //余数为0，剩下的部分直接计算出来
            res.x |= ((var1.x << (UINT_BITS - leftBit)) >> (UINT_BITS - leftBit)) / i;
            return res;
        }
        else
        {
            remainBits = findMSB(remain) + 1;
            isOverflow = (UINT_BITS == remainBits);
            if (isOverflow)
            {
                --remainBits;
            }
            addtionalBits = (UINT_BITS - remainBits < leftBit ? UINT_BITS - remainBits : leftBit);
            leftBit -= addtionalBits;
        }
    } while (leftBit > 0);

    if (isOverflow)
    {
        res.x |= 1;
    }
    else
    {
        midVar = remain << addtionalBits;
        midVar |= (var1.x << (UINT_BITS - addtionalBits)) >> (UINT_BITS - addtionalBits);
        res.x |= midVar / i;
    }
    return res;
}

ivec2 i64_negative(ivec2 var)
{
    uvec2 res = uvec2(~var);
    uint carry = 0;
    res.x = uaddCarry(res.x, 1u, carry);
    res.y += carry;
    return ivec2(res);
}

ivec2 i64_divide(ivec2 var1, int i)
{
    // 结果是否为负
    bool isNagetive = (var1.y ^ i) < 0;
    if (var1.y < 0)
    {
        var1 = i64_negative(var1);
    }
    if (i < 0)
    {
        i = -i;
    }
    // 转为正数进行无符号运算
    ivec2 res = ivec2(u64_divide(uvec2(var1), uint(i)));
    if (isNagetive)
    {
        res = i64_negative(res);
    }
    return res;
}

} // namespace ShaderUtility

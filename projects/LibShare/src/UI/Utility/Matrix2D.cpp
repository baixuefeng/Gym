/******************************************************************************
*  版权所有（C）2008-2016，上海二三四五网络科技有限公司                        *
*  保留所有权利。                                                            *
******************************************************************************
*  作者 : 白雪峰
*  版本 : 1.0
*****************************************************************************/
#include "UI/Utility/Matrix2D.h"
#include <cassert>
#include <cmath>
#include <cstring>
#include <float.h>
#include "targetver.h"

SHARELIB_BEGIN_NAMESPACE

/** 角度转化为弧度
*/
#ifndef TO_RADIAN
#    define TO_RADIAN(degree) ((degree)*0.01745329252)
#endif

Matrix2D::Matrix2D()
{
    Reset();
}

Matrix2D::Matrix2D(const XFORM &mx)
{
    m_data[0][0] = mx.eM11;
    m_data[0][1] = mx.eM12;
    m_data[1][0] = mx.eM21;
    m_data[1][1] = mx.eM22;
    m_data[2][0] = mx.eDx;
    m_data[2][1] = mx.eDy;
}

Matrix2D::Matrix2D(const D2D1::Matrix3x2F &mx)
{
    m_data[0][0] = mx._11;
    m_data[0][1] = mx._12;
    m_data[1][0] = mx._21;
    m_data[1][1] = mx._22;
    m_data[2][0] = mx._31;
    m_data[2][1] = mx._32;
}

Matrix2D::Matrix2D(const DWRITE_MATRIX &mx)
{
    m_data[0][0] = mx.m11;
    m_data[0][1] = mx.m12;
    m_data[1][0] = mx.m21;
    m_data[1][1] = mx.m22;
    m_data[2][0] = mx.dx;
    m_data[2][1] = mx.dy;
}

Matrix2D::operator XFORM() const
{
    XFORM mx;
    mx.eM11 = (float)m_data[0][0];
    mx.eM12 = (float)m_data[0][1];
    mx.eM21 = (float)m_data[1][0];
    mx.eM22 = (float)m_data[1][1];
    mx.eDx = (float)m_data[2][0];
    mx.eDy = (float)m_data[2][1];
    return mx;
}

Matrix2D::operator D2D1::Matrix3x2F() const
{
    D2D1::Matrix3x2F mx;
    mx._11 = (float)m_data[0][0];
    mx._12 = (float)m_data[0][1];
    mx._21 = (float)m_data[1][0];
    mx._22 = (float)m_data[1][1];
    mx._31 = (float)m_data[2][0];
    mx._32 = (float)m_data[2][1];
    return mx;
}

Matrix2D::operator DWRITE_MATRIX() const
{
    DWRITE_MATRIX mx;
    mx.m11 = (float)m_data[0][0];
    mx.m12 = (float)m_data[0][1];
    mx.m21 = (float)m_data[1][0];
    mx.m22 = (float)m_data[1][1];
    mx.dx = (float)m_data[2][0];
    mx.dy = (float)m_data[2][1];
    return mx;
}

void Matrix2D::Reset()
{
    std::memset(m_data, 0, sizeof(m_data));
    m_data[0][0] = 1.0f;
    m_data[1][1] = 1.0f;
}

float Matrix2D::Determinant() const
{
    return (m_data[0][0] * m_data[1][1] - m_data[0][1] * m_data[1][0]);
}

bool Matrix2D::Invert()
{
    float determinant = Determinant();
    if (std::abs(determinant) <= FLT_EPSILON) {
        return false;
    }
    float reciprocal = 1.0f / determinant;

    Matrix2D mx;
    mx.m_data[0][0] = +(m_data[1][1] * reciprocal);
    mx.m_data[1][0] = -(m_data[1][0] * reciprocal);
    mx.m_data[2][0] = +(m_data[1][0] * m_data[2][1] - m_data[2][0] * m_data[1][1]) * reciprocal;
    mx.m_data[0][1] = -(m_data[0][1] * reciprocal);
    mx.m_data[1][1] = +(m_data[0][0] * reciprocal);
    mx.m_data[2][1] = -(m_data[0][0] * m_data[2][1] - m_data[2][0] * m_data[0][1]) * reciprocal;
    *this = mx;
    return true;
}

bool Matrix2D::IsInvertible() const
{
    return std::abs(Determinant()) > FLT_EPSILON;
}

bool Matrix2D::IsIdentity() const
{
    for (int i = 0; i < 3; ++i) {
        for (int j = 0; j < 2; ++j) {
            if (i == j) {
                if (std::abs(m_data[i][j] - 1.0f) > FLT_EPSILON) {
                    return false;
                }
            } else {
                if (std::abs(m_data[i][j]) > FLT_EPSILON) {
                    return false;
                }
            }
        }
    }
    return true;
}

void Matrix2D::Multiply(const Matrix2D &mx, bool prepend /* = true*/)
{
    const float SrcA00 = m_data[0][0];
    const float SrcA01 = m_data[0][1];
    const float SrcA10 = m_data[1][0];
    const float SrcA11 = m_data[1][1];
    const float SrcA20 = m_data[2][0];
    const float SrcA21 = m_data[2][1];

    const float SrcB00 = mx.m_data[0][0];
    const float SrcB01 = mx.m_data[0][1];
    const float SrcB10 = mx.m_data[1][0];
    const float SrcB11 = mx.m_data[1][1];
    const float SrcB20 = mx.m_data[2][0];
    const float SrcB21 = mx.m_data[2][1];
    if (prepend) {
        m_data[0][0] = SrcA00 * SrcB00 + SrcA10 * SrcB01;
        m_data[0][1] = SrcA01 * SrcB00 + SrcA11 * SrcB01;
        m_data[1][0] = SrcA00 * SrcB10 + SrcA10 * SrcB11;
        m_data[1][1] = SrcA01 * SrcB10 + SrcA11 * SrcB11;
        m_data[2][0] = SrcA00 * SrcB20 + SrcA10 * SrcB21 + SrcA20;
        m_data[2][1] = SrcA01 * SrcB20 + SrcA11 * SrcB21 + SrcA21;
    } else {
        m_data[0][0] = SrcB00 * SrcA00 + SrcB10 * SrcA01;
        m_data[0][1] = SrcB01 * SrcA00 + SrcB11 * SrcA01;
        m_data[1][0] = SrcB00 * SrcA10 + SrcB10 * SrcA11;
        m_data[1][1] = SrcB01 * SrcA10 + SrcB11 * SrcA11;
        m_data[2][0] = SrcB00 * SrcA20 + SrcB10 * SrcA21 + SrcB20;
        m_data[2][1] = SrcB01 * SrcA20 + SrcB11 * SrcA21 + SrcB21;
    }
}

Matrix2D Matrix2D::operator*(const Matrix2D &mx) const
{
    Matrix2D res = *this;
    res *= mx;
    return res;
}

Matrix2D &Matrix2D::operator*=(const Matrix2D &mx)
{
    Multiply(mx, false);
    return *this;
}

Matrix2D Matrix2D::operator/(const Matrix2D &mx) const
{
    Matrix2D res = *this;
    res /= mx;
    return res;
}

Matrix2D &Matrix2D::operator/=(const Matrix2D &mx)
{
    Matrix2D mx1 = mx;
    bool bInvertible = mx1.Invert();
    assert(bInvertible);
    if (bInvertible) {
        Multiply(mx1, false);
    }
    return *this;
}

void Matrix2D::Translate(float fX, float fY, bool prepend /* = true*/)
{
    Matrix2D mx;
    mx.m_data[2][0] = fX;
    mx.m_data[2][1] = fY;
    Multiply(mx, prepend);
}

void Matrix2D::Scale(float fX, float fY, bool prepend /* = true*/)
{
    Matrix2D mx;
    mx.m_data[0][0] = fX;
    mx.m_data[1][1] = fY;
    Multiply(mx, prepend);
}

void Matrix2D::ScaleAt(float fX, float fY, float centerX, float centerY, bool prepend /* = true*/)
{
    Matrix2D mx;
    mx.m_data[0][0] = fX;
    mx.m_data[1][1] = fY;
    mx.m_data[2][0] = (1.0f - fX) * centerX;
    mx.m_data[2][1] = (1.0f - fY) * centerY;
    Multiply(mx, prepend);
}

void Matrix2D::Rotate(float fAngle, bool prepend /* = true*/)
{
    Matrix2D mx;
    float s = (float)std::sin(TO_RADIAN(fAngle));
    float c = (float)std::cos(TO_RADIAN(fAngle));
    mx.m_data[0][0] = c;
    mx.m_data[0][1] = s;
    mx.m_data[1][0] = -s;
    mx.m_data[1][1] = c;
    Multiply(mx, prepend);
}

void Matrix2D::RotateAt(float fAngle, float centerX, float centerY, bool prepend /* = true*/)
{
    Matrix2D mx;
    float s = (float)std::sin(TO_RADIAN(fAngle));
    float c = (float)std::cos(TO_RADIAN(fAngle));
    mx.m_data[0][0] = c;
    mx.m_data[0][1] = s;
    mx.m_data[1][0] = -s;
    mx.m_data[1][1] = c;
    mx.m_data[2][0] = (1.0f - c) * centerX + s * centerY;
    mx.m_data[2][1] = (1.0f - c) * centerY - s * centerX;
    Multiply(mx, prepend);
}

void Matrix2D::Shear(float shearX, float shearY, bool prepend /* = true*/)
{
    Matrix2D mx;
    mx.m_data[0][1] = shearY;
    mx.m_data[1][0] = shearX;
    Multiply(mx, prepend);
}

void Matrix2D::ShearAt(float shearX, float shearY, float fixX, float fixY, bool prepend /* = true*/)
{
    Matrix2D mx;
    mx.m_data[0][1] = shearY;
    mx.m_data[1][0] = shearX;
    mx.m_data[2][0] = -fixY * shearX;
    mx.m_data[2][1] = -fixX * shearY;
    Multiply(mx, prepend);
}

void Matrix2D::TransformPoints(POINT *pPt, unsigned nCount) const
{
    long x = 0, y = 0;
    for (unsigned i = 0; i < nCount; ++i) {
        x = pPt[i].x;
        y = pPt[i].y;
        pPt[i].x = (long)std::round(x * m_data[0][0] + y * m_data[1][0] + m_data[2][0]);
        pPt[i].y = (long)std::round(x * m_data[0][1] + y * m_data[1][1] + m_data[2][1]);
    }
}

SHARELIB_END_NAMESPACE

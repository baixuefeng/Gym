#pragma once
#include "MacroDefBase.h"
#include <windows.h>
#include <d2d1helper.h>
#include <dwrite.h>

SHARELIB_BEGIN_NAMESPACE

/** 2D变换矩阵, 针对行向量.内存布局与XFORM、D2D1::Matrix3x2F是一模一样的
*/
class Matrix2D
{
public:
    /** 构造函数, 初始化为单位矩阵
    */
    Matrix2D();

    /** 从XFORM初始化自己
    @param [in] mx XFORM格式的矩阵
    */
    Matrix2D(const XFORM& mx);

    /** 从D2D的矩阵初始化自己
    @param [in] mx D2D矩阵
    */
    Matrix2D(const D2D1::Matrix3x2F & mx);

    /** 从DWRITE的矩阵初始化自己
    @param[in] mx DWRITE的矩阵
    */
    Matrix2D(const DWRITE_MATRIX & mx);

    /** 转换为XFORM
    */
    operator XFORM() const;

    /** 转换为D2D的矩阵
    */
    operator D2D1::Matrix3x2F() const;

    /* 转换为DWRITE的矩阵
    */
    operator DWRITE_MATRIX() const;

    /** 重置矩阵为单位矩阵
    */
    void Reset();

    /** 矩阵的行列式的值
    */
    float Determinant() const;

    /** 矩阵求逆
    @return true表示正确，false表示矩阵不可逆
    */
    bool Invert();

    /** 矩阵是否可逆
    */
    bool IsInvertible() const;

    /** 矩阵是否为单位矩阵
    */
    bool IsIdentity() const;

    /** 乘另外一个矩阵
    @param [in] mx 要乘的矩阵
    @param [in] prepend true表示 mx 左乘当前矩阵, false表示右乘当前矩阵
    */
    void Multiply(const Matrix2D& mx, bool prepend = true);

    /** 矩阵乘法
    @param[in] mx 乘数
    @return 乘积
    */
    Matrix2D operator *(const Matrix2D & mx) const;

    /** 用另一个矩阵右乘当前矩阵
    @param[in] mx 要乘的矩阵
    @return 乘积
    */
    Matrix2D & operator *=(const Matrix2D & mx);

    /** 矩阵除法
    @param[in] mx 除数
    @return 商
    */
    Matrix2D operator /(const Matrix2D & mx) const;

    /** 除以另一个矩阵
    @param[in] mx 除数
    @return 商
    */
    Matrix2D & operator /=(const Matrix2D & mx);

    /** 平移变换
    @param [in] fX X方向平移量
    @param [in] fY Y方向平移量
    @param [in] prepend true表示该变换所代表的矩阵左乘当前矩阵, false表示右乘当前矩阵
    */
    void Translate(float fX, float fY, bool prepend = true);

    /** 缩放变换,缩放中心为(0,0)
    @param [in] fX X方向缩放倍率
    @param [in] fY Y方向缩放倍率
    @param [in] prepend true表示该变换所代表的矩阵左乘当前矩阵, false表示右乘当前矩阵
    */
    void Scale(float fX, float fY, bool prepend = true);

    /** 以某一点为中心进行缩放变换
    @param [in] fX X方向缩放倍率
    @param [in] fY Y方向缩放倍率
    @param [in] centerX 中心点的X坐标
    @param [in] centerY 中心点的Y坐标
    @param [in] prepend true表示该变换所代表的矩阵左乘当前矩阵, false表示右乘当前矩阵
    */
    void ScaleAt(float fX, float fY, float centerX, float centerY, bool prepend = true);

    /** 旋转变换,旋转中心为(0,0)
    @param [in] fAngle 旋转角度,正值表示从 X正向 向 Y正向 旋转
    @param [in] prepend true表示该变换所代表的矩阵左乘当前矩阵, false表示右乘当前矩阵
    */
    void Rotate(float fAngle, bool prepend = true);

    /** 以某一点为中心进行旋转变换
    @param [in] fAngle 旋转角度,正值表示从 X正向 向 Y正向 旋转
    @param [in] centerX 中心点的X坐标
    @param [in] centerY 中心点的Y坐标
    @param [in] prepend true表示该变换所代表的矩阵左乘当前矩阵, false表示右乘当前矩阵
    */
    void RotateAt(float fAngle, float centerX, float centerY, bool prepend = true);

    /** 错切变换,固定点为(0,0)
    @param [in] shearX X方向错切倍率
    @param [in] shearY Y方向错切倍率
    @param [in] prepend true表示该变换所代表的矩阵左乘当前矩阵, false表示右乘当前矩阵
    */
    void Shear(float shearX, float shearY, bool prepend = true);

    /** 以某一点为固定点进行错切变换
    @param [in] shearX X方向错切倍率
    @param [in] shearY Y方向错切倍率
    @param [in] fixX 固定点的X坐标
    @param [in] fixY 固定点的Y坐标
    @param [in] prepend true表示该变换所代表的矩阵左乘当前矩阵, false表示右乘当前矩阵
    */
    void ShearAt(float shearX, float shearY, float fixX, float fixY, bool prepend = true);

    /** 用当前变换矩阵变换坐标值
    @param [in,out] pPt 点数组的起始指针
    @param [in] nCount 点数组的个数
    */
    void TransformPoints(POINT * pPt, unsigned nCount) const;

    /** 2D变换矩阵的实际数据(3X2矩阵)
    */
    float m_data[3][2];
};

SHARELIB_END_NAMESPACE

#pragma once
#include <cfloat>
#include <cmath>
#include <cstring>
#include <d2d1helper.h>
#include "MacroDefBase.h"

SHARELIB_BEGIN_NAMESPACE

//该文件是从atlmisc.h移植过来的，针对D2D中最常用的 D2D_SIZE_F,D2D_POINT_2F,D2D_RECT_2F
//进行仿WTL的封装

class CD2DSizeF;
class CD2DPointF;
class CD2DRectF;
///////////////////////////////////////////////////////////////////////////////
// CD2DSizeF - Wrapper for D2D_SIZE_F/D2D_SIZE_U structure.

class CD2DSizeF : public D2D_SIZE_F
{
public:
    // Constructors
    CD2DSizeF()
    {
        width = 0;
        height = 0;
    }

    CD2DSizeF(FLOAT initCX, FLOAT initCY)
    {
        width = initCX;
        height = initCY;
    }

    CD2DSizeF(const D2D_SIZE_F &size) { std::memcpy(this, &size, sizeof(size)); }

    CD2DSizeF(const SIZE &initSize)
    {
        width = (FLOAT)initSize.cx;
        height = (FLOAT)initSize.cy;
    }

    operator SIZE() const
    {
        SIZE sz;
        sz.cx = (LONG)std::round(width);
        sz.cy = (LONG)std::round(height);
        return sz;
    }

    // Operations
    bool operator==(CD2DSizeF size) const
    {
        return (std::abs(width - size.width) < FLT_EPSILON) &&
               (std::abs(height - size.height) < FLT_EPSILON);
    }

    bool operator!=(CD2DSizeF size) const { return !(*this == size); }

    void operator+=(CD2DSizeF size)
    {
        width += size.width;
        height += size.height;
    }

    void operator-=(CD2DSizeF size)
    {
        width -= size.width;
        height -= size.height;
    }

    // Operators returning CD2DSizeF values
    CD2DSizeF operator+(CD2DSizeF size) const
    {
        return CD2DSizeF(width + size.width, height + size.height);
    }

    CD2DSizeF operator-(CD2DSizeF size) const
    {
        return CD2DSizeF(width - size.width, height - size.height);
    }

    CD2DSizeF operator-() const { return CD2DSizeF(-width, -height); }

    // Operators returning CD2DPointF values
    CD2DPointF operator+(CD2DPointF point) const;
    CD2DPointF operator-(CD2DPointF point) const;

    // Operators returning CRect values
    CD2DRectF operator+(const CD2DRectF &rect) const;
    CD2DRectF operator-(const CD2DRectF &rect) const;
};

///////////////////////////////////////////////////////////////////////////////
// CD2DPointF - Wrapper for D2D_POINT_2F structure.

class CD2DPointF : public D2D_POINT_2F
{
public:
    // Constructors
    CD2DPointF()
    {
        x = 0;
        y = 0;
    }

    CD2DPointF(FLOAT initX, FLOAT initY)
    {
        x = initX;
        y = initY;
    }

    CD2DPointF(const D2D_POINT_2F &point) { std::memcpy(this, &point, sizeof(point)); }

    CD2DPointF(const POINT &initPt)
    {
        x = (FLOAT)initPt.x;
        y = (FLOAT)initPt.y;
    }

    operator POINT() const
    {
        POINT pt;
        pt.x = (LONG)std::round(x);
        pt.y = (LONG)std::round(y);
        return pt;
    }

    // Operations
    void Offset(FLOAT xOffset, FLOAT yOffset)
    {
        x += xOffset;
        y += yOffset;
    }

    void Offset(CD2DPointF point)
    {
        x += point.x;
        y += point.y;
    }

    void Offset(CD2DSizeF size)
    {
        x += size.width;
        y += size.height;
    }

    bool operator==(CD2DPointF point) const
    {
        return (std::abs(x - point.x) < FLT_EPSILON) && (std::abs(y - point.y) < FLT_EPSILON);
    }

    bool operator!=(CD2DPointF point) const { return !(*this == point); }

    void operator+=(CD2DSizeF size)
    {
        x += size.width;
        y += size.height;
    }

    void operator-=(CD2DSizeF size)
    {
        x -= size.width;
        y -= size.height;
    }

    void operator+=(CD2DPointF point)
    {
        x += point.x;
        y += point.y;
    }

    void operator-=(CD2DPointF point)
    {
        x -= point.x;
        y -= point.y;
    }

    // Operators returning CD2DPointF values
    CD2DPointF operator+(CD2DSizeF size) const
    {
        return CD2DPointF(x + size.width, y + size.height);
    }

    CD2DPointF operator-(CD2DSizeF size) const
    {
        return CD2DPointF(x - size.width, y - size.height);
    }

    CD2DPointF operator-() const { return CD2DPointF(-x, -y); }

    CD2DPointF operator+(CD2DPointF point) const { return CD2DPointF(x + point.x, y + point.y); }

    // Operators returning CD2DSizeF values
    CD2DSizeF operator-(CD2DPointF point) const { return CD2DSizeF(x - point.x, y - point.y); }

    // Operators returning CD2DRectF values
    CD2DRectF operator+(const CD2DRectF &rect) const;
    CD2DRectF operator-(const CD2DRectF &rect) const;
};

///////////////////////////////////////////////////////////////////////////////
// CD2DRectF - Wrapper for D2D_RECT_F structure.

class CD2DRectF : public D2D_RECT_F
{
public:
    // Constructors
    CD2DRectF()
    {
        left = 0;
        top = 0;
        right = 0;
        bottom = 0;
    }

    CD2DRectF(FLOAT l, FLOAT t, FLOAT r, FLOAT b)
    {
        left = l;
        top = t;
        right = r;
        bottom = b;
    }

    CD2DRectF(const D2D_RECT_F &rect) { std::memcpy(this, &rect, sizeof(rect)); }

    CD2DRectF(const RECT &srcRect)
    {
        left = (FLOAT)srcRect.left;
        top = (FLOAT)srcRect.top;
        right = (FLOAT)srcRect.right;
        bottom = (FLOAT)srcRect.bottom;
    }

    operator RECT() const
    {
        RECT rc;
        rc.left = (LONG)std::round(left);
        rc.top = (LONG)std::round(top);
        rc.right = (LONG)std::round(right);
        rc.bottom = (LONG)std::round(bottom);
        return rc;
    }

    CD2DRectF(CD2DPointF point, CD2DSizeF size)
    {
        right = (left = point.x) + size.width;
        bottom = (top = point.y) + size.height;
    }

    CD2DRectF(CD2DPointF topLeft, CD2DPointF bottomRight)
    {
        left = topLeft.x;
        top = topLeft.y;
        right = bottomRight.x;
        bottom = bottomRight.y;
    }

    // Attributes (in addition to CD2DRectF members)
    FLOAT Width() const { return right - left; }

    FLOAT Height() const { return bottom - top; }

    CD2DSizeF Size() const { return CD2DSizeF(right - left, bottom - top); }

    CD2DPointF &TopLeft() { return *((CD2DPointF *)this); }

    CD2DPointF &BottomRight() { return *((CD2DPointF *)this + 1); }

    const CD2DPointF &TopLeft() const { return *((CD2DPointF *)this); }

    const CD2DPointF &BottomRight() const { return *((CD2DPointF *)this + 1); }

    CD2DPointF CenterPoint() const { return CD2DPointF((left + right) / 2, (top + bottom) / 2); }

    bool IsRectEmpty() const
    {
        return (left > right) || (top > bottom) || (std::abs(left - right) < FLT_EPSILON) ||
               (std::abs(top - bottom) < FLT_EPSILON);
    }

    bool IsRectNull() const
    {
        return (std::abs(left) < FLT_EPSILON) && (std::abs(top) < FLT_EPSILON) &&
               (std::abs(right) < FLT_EPSILON) && (std::abs(bottom) < FLT_EPSILON);
    }

    bool PtInRect(CD2DPointF point) const
    {
        if (IsRectEmpty() || IsRectNull()) {
            return false;
        }
        if (point.x >= left && point.x < right && point.y >= top && point.y < bottom) {
            return true;
        }
        return false;
    }

    // Operations

    void SetRectEmpty() { *this = CD2DRectF(); }

    void SetRectInfinite() { *this = D2D1::InfiniteRect(); }

    void InflateRect(FLOAT x, FLOAT y)
    {
        left -= x;
        top -= y;
        right += x;
        bottom += y;
    }

    void InflateRect(CD2DSizeF size) { InflateRect(size.width, size.height); }

    void InflateRect(const CD2DRectF &rect)
    {
        left -= rect.left;
        top -= rect.top;
        right += rect.right;
        bottom += rect.bottom;
    }

    void InflateRect(FLOAT l, FLOAT t, FLOAT r, FLOAT b)
    {
        left -= l;
        top -= t;
        right += r;
        bottom += b;
    }

    void DeflateRect(FLOAT x, FLOAT y) { InflateRect(-x, -y); }

    void DeflateRect(CD2DSizeF size) { InflateRect(-size.width, -size.height); }

    void DeflateRect(const CD2DRectF &rect)
    {
        left += rect.left;
        top += rect.top;
        right -= rect.right;
        bottom -= rect.bottom;
    }

    void DeflateRect(FLOAT l, FLOAT t, FLOAT r, FLOAT b)
    {
        left += l;
        top += t;
        right -= r;
        bottom -= b;
    }

    void OffsetRect(FLOAT x, FLOAT y)
    {
        left += x;
        right += x;
        top += y;
        bottom += y;
    }
    void OffsetRect(CD2DSizeF size) { OffsetRect(size.width, size.height); }

    void OffsetRect(CD2DPointF point) { OffsetRect(point.x, point.y); }

    void NormalizeRect()
    {
        FLOAT nTemp;
        if (left > right) {
            nTemp = left;
            left = right;
            right = nTemp;
        }
        if (top > bottom) {
            nTemp = top;
            top = bottom;
            bottom = nTemp;
        }
    }

    // absolute position of rectangle
    void MoveToY(FLOAT y)
    {
        bottom = Height() + y;
        top = y;
    }

    void MoveToX(FLOAT x)
    {
        right = Width() + x;
        left = x;
    }

    void MoveToXY(FLOAT x, FLOAT y)
    {
        MoveToX(x);
        MoveToY(y);
    }

    void MoveToXY(CD2DPointF pt)
    {
        MoveToX(pt.x);
        MoveToY(pt.y);
    }

    // operations that fill '*this' with result
    bool IntersectRect(const CD2DRectF &rect1, const CD2DRectF &rect2)
    {
        left = (rect1.left > rect2.left ? rect1.left : rect2.left);
        right = (rect1.right < rect2.right ? rect1.right : rect2.right);
        top = (rect1.top > rect2.top ? rect1.top : rect2.top);
        bottom = (rect1.bottom < rect2.bottom ? rect1.bottom : rect2.bottom);
        return IsRectEmpty();
    }

    bool UnionRect(const CD2DRectF &rect1, const CD2DRectF &rect2)
    {
        left = (rect1.left < rect2.left ? rect1.left : rect2.left);
        right = (rect1.right > rect2.right ? rect1.right : rect2.right);
        top = (rect1.top < rect2.top ? rect1.top : rect2.top);
        bottom = (rect1.bottom > rect2.bottom ? rect1.bottom : rect2.bottom);
        return IsRectEmpty();
    }

    // Additional Operations

    bool operator==(const CD2DRectF &rect) const
    {
        return (std::abs(left - rect.left) < FLT_EPSILON) &&
               (std::abs(right - rect.right) < FLT_EPSILON) &&
               (std::abs(top - rect.top) < FLT_EPSILON) &&
               (std::abs(bottom - rect.bottom) < FLT_EPSILON);
    }

    bool operator!=(const CD2DRectF &rect) const { return !(*this == rect); }

    void operator+=(CD2DPointF point) { OffsetRect(point.x, point.y); }

    void operator+=(CD2DSizeF size) { OffsetRect(size.width, size.height); }

    void operator+=(const CD2DRectF &rect) { InflateRect(rect); }

    void operator-=(CD2DPointF point) { OffsetRect(-point.x, -point.y); }

    void operator-=(CD2DSizeF size) { OffsetRect(-size.width, -size.height); }

    void operator-=(const CD2DRectF &rect) { DeflateRect(rect); }

    void operator&=(const CD2DRectF &rect) { IntersectRect(*this, rect); }

    void operator|=(const CD2DRectF &rect) { UnionRect(*this, rect); }

    // Operators returning CD2DRectF values
    CD2DRectF operator+(CD2DPointF pt) const
    {
        CD2DRectF rect(*this);
        rect.OffsetRect(pt.x, pt.y);
        return rect;
    }

    CD2DRectF operator-(CD2DPointF pt) const
    {
        CD2DRectF rect(*this);
        rect.OffsetRect(-pt.x, -pt.y);
        return rect;
    }

    CD2DRectF operator+(const CD2DRectF &rect) const
    {
        CD2DRectF rcReturn(*this);
        rcReturn.InflateRect(rect);
        return rcReturn;
    }

    CD2DRectF operator-(const CD2DRectF &rect) const
    {
        CD2DRectF rcReturn(*this);
        rcReturn.DeflateRect(rect);
        return rcReturn;
    }

    CD2DRectF operator+(CD2DSizeF size) const
    {
        CD2DRectF rect(*this);
        rect.OffsetRect(size.width, size.height);
        return rect;
    }

    CD2DRectF operator-(CD2DSizeF size) const
    {
        CD2DRectF rect(*this);
        rect.OffsetRect(-size.width, -size.height);
        return rect;
    }

    CD2DRectF operator&(const CD2DRectF &rect2) const
    {
        CD2DRectF rect;
        rect.IntersectRect(*this, rect2);
        return rect;
    }

    CD2DRectF operator|(const CD2DRectF &rect2) const
    {
        CD2DRectF rect;
        rect.UnionRect(*this, rect2);
        return rect;
    }
};

// CD2DSizeF implementation
inline CD2DPointF CD2DSizeF::operator+(CD2DPointF point) const
{
    return CD2DPointF(width + point.x, height + point.y);
}

inline CD2DPointF CD2DSizeF::operator-(CD2DPointF point) const
{
    return CD2DPointF(width - point.x, height - point.y);
}

inline CD2DRectF CD2DSizeF::operator+(const CD2DRectF &rect) const
{
    return CD2DRectF(rect) + *this;
}

inline CD2DRectF CD2DSizeF::operator-(const CD2DRectF &rect) const
{
    return CD2DRectF(rect) - *this;
}

// CD2DPointF implementation
inline CD2DRectF CD2DPointF::operator+(const CD2DRectF &rect) const
{
    return CD2DRectF(rect) + *this;
}

inline CD2DRectF CD2DPointF::operator-(const CD2DRectF &rect) const
{
    return CD2DRectF(rect) - *this;
}

SHARELIB_END_NAMESPACE

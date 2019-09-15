#include "stdafx.h"
#include <ObjIdl.h>
#pragma warning(disable : 4458)
#include <GdiPlus.h>
#pragma warning(default : 4458)
#include "Log/TempLog.h"
#include "TestUnit/MatrixTest.h"
#include "ui/Utility/GdiplusUtility.h"
#include "ui/Utility/Matrix2D.h"

BEGIN_SHARELIBTEST_NAMESPACE

void PrintMx(const shr::Matrix2D &mx, const Gdiplus::Matrix &gmx)
{
    float data[3][2] = {0};
    gmx.GetElements((float *)data);
    tcout.Printf(
        L"\nMyMatrix:%g, %g, %g, %g, %g, %g\n"
        L"gdiplus: %g, %g, %g, %g, %g, %g\n",
        mx.m_data[0][0],
        mx.m_data[0][1],
        mx.m_data[1][0],
        mx.m_data[1][1],
        mx.m_data[2][0],
        mx.m_data[2][1],
        data[0][0],
        data[0][1],
        data[1][0],
        data[1][1],
        data[2][0],
        data[2][1]);
}

void CompareMatrixCalculate()
{
    shr::InitializeGdiplus();
    shr::Matrix2D mx;
    Gdiplus::Matrix gmx;

    mx.Translate(0.1f, 0.2f, true);
    gmx.Translate(0.1f, 0.2f);
    PrintMx(mx, gmx);

    mx.Translate(0.1f, 0.2f, false);
    gmx.Translate(0.1f, 0.2f, Gdiplus::MatrixOrder::MatrixOrderAppend);
    PrintMx(mx, gmx);

    mx.Scale(2, 3, true);
    gmx.Scale(2, 3);
    PrintMx(mx, gmx);

    mx.Scale(4, 5, false);
    gmx.Scale(4, 5, Gdiplus::MatrixOrder::MatrixOrderAppend);
    PrintMx(mx, gmx);

    mx.ScaleAt(1.2f, 1.3f, 12, 13, true);
    gmx.Translate(12, 13);
    gmx.Scale(1.2f, 1.3f);
    gmx.Translate(-12, -13);
    PrintMx(mx, gmx);

    mx.ScaleAt(1.2f, 1.3f, 12, 13, false);
    gmx.Translate(-12, -13, Gdiplus::MatrixOrder::MatrixOrderAppend);
    gmx.Scale(1.2f, 1.3f, Gdiplus::MatrixOrder::MatrixOrderAppend);
    gmx.Translate(12, 13, Gdiplus::MatrixOrder::MatrixOrderAppend);
    PrintMx(mx, gmx);

    mx.Rotate(30, true);
    gmx.Rotate(30);
    PrintMx(mx, gmx);

    mx.Rotate(78, false);
    gmx.Rotate(78, Gdiplus::MatrixOrder::MatrixOrderAppend);
    PrintMx(mx, gmx);

    mx.RotateAt(32, 123, 456, true);
    gmx.RotateAt(32, Gdiplus::PointF(123, 456));
    PrintMx(mx, gmx);

    mx.RotateAt(32, 123, 456, false);
    gmx.RotateAt(32, Gdiplus::PointF(123, 456), Gdiplus::MatrixOrder::MatrixOrderAppend);
    PrintMx(mx, gmx);

    mx.Shear(3, 4, true);
    gmx.Shear(3, 4);
    PrintMx(mx, gmx);

    mx.Shear(3, 4, false);
    gmx.Shear(3, 4, Gdiplus::MatrixOrder::MatrixOrderAppend);
    PrintMx(mx, gmx);

    mx.ShearAt(3, 4, 30, 50, true);
    gmx.Translate(30, 50);
    gmx.Shear(3, 4);
    gmx.Translate(-30, -50);
    PrintMx(mx, gmx);

    mx.ShearAt(3, 4, 30, 50, false);
    gmx.Translate(-30, -50, Gdiplus::MatrixOrder::MatrixOrderAppend);
    gmx.Shear(3, 4, Gdiplus::MatrixOrder::MatrixOrderAppend);
    gmx.Translate(30, 50, Gdiplus::MatrixOrder::MatrixOrderAppend);
    PrintMx(mx, gmx);

    tcout << mx.IsInvertible() << " " << gmx.IsInvertible() << std::endl;
    mx.Invert();
    gmx.Invert();
    PrintMx(mx, gmx);

    POINT pt{100, 300};
    Gdiplus::Point gpt(100, 300);
    mx.TransformPoints(&pt, 1);
    gmx.TransformPoints(&gpt);
    tcout << pt.x << " " << pt.y << std::endl;
    tcout << gpt.X << " " << gpt.Y << std::endl;

    mx.Reset();
    gmx.Reset();
    PrintMx(mx, gmx);

    XFORM xMx = mx;
    D2D1::Matrix3x2F d2dMx = mx;
    tcout << "内存布局 " << std::memcmp(&xMx, &mx, sizeof(mx)) << " "
          << std::memcmp(&d2dMx, &mx, sizeof(mx)) << std::endl;
}

END_SHARELIBTEST_NAMESPACE

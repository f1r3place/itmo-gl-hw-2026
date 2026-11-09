#pragma once

#include <QMatrix4x4>
#include <QPointF>
#include <QSize>

#include <algorithm>

struct Camera2D
{
    QPointF center{0.0, 0.0};
    double unitsPerPixel = 1.0 / 240.0;

    QPointF screenToWorld(const QPointF &pixel, const QSize &viewport) const
    {
        return center + QPointF((pixel.x() - viewport.width() * 0.5) * unitsPerPixel,
                                (viewport.height() * 0.5 - pixel.y()) * unitsPerPixel);
    }

    void pan(const QPointF &delta)
    {
        center += QPointF(-delta.x() * unitsPerPixel, delta.y() * unitsPerPixel);
    }

    void zoomAt(const QPointF &pixel, const QSize &viewport, double factor)
    {
        const QPointF before = screenToWorld(pixel, viewport);
        unitsPerPixel = std::clamp(unitsPerPixel / factor, 0.0001, 0.1);
        center += before - screenToWorld(pixel, viewport);
    }

    QMatrix4x4 projection(const QSize &viewport) const
    {
        const float halfWidth = viewport.width() * unitsPerPixel * 0.5;
        const float halfHeight = viewport.height() * unitsPerPixel * 0.5;
        QMatrix4x4 matrix;
        matrix.ortho(-halfWidth, halfWidth, -halfHeight, halfHeight, 0.1f, 100.0f);
        return matrix;
    }
};

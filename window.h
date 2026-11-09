#pragma once

#include "openglwidget.h"
#include "camera2d.h"

#include <QOpenGLBuffer>
#include <QOpenGLShaderProgram>
#include <QOpenGLVertexArrayObject>
#include <QWindow>

class Window : public OpenGLWidget
{
    Q_OBJECT

public:
    Window();
    ~Window();

    void onInit() override;
    void onRender() override;
    void onResize(size_t width, size_t height) override;

protected:
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void wheelEvent(QWheelEvent *event) override;

private:
    void updateGeometry();

    Camera2D camera_;
    QPointF lastMousePosition_;
    int iterations_ = 8;
    bool shouldRebuild_ = true;
    GLsizei pointCount_ = 0;

    GLint mvpUniform_ = -1;

    QMatrix4x4 model_;
    QMatrix4x4 view_;
    QMatrix4x4 projection_;

    QOpenGLBuffer vbo_{QOpenGLBuffer::Type::VertexBuffer};
    QOpenGLVertexArrayObject vao_;

    std::unique_ptr<QOpenGLShaderProgram> program_;
};

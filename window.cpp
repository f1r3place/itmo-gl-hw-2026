#include "window.h"

#include <QLabel>
#include <QMouseEvent>
#include <QSlider>
#include <QVBoxLayout>
#include <QVector2D>
#include <QWheelEvent>

#include <cmath>
#include <vector>

namespace {
std::vector<QVector2D> generateDragon(int iterations)
{
    std::vector<QVector2D> points = {{0.0f, 0.0f}, {1.0f, 0.0f}};
    points.reserve((1 << iterations) + 1);

    for (int n = 0; n < iterations; ++n) {
        const QVector2D end = points.back();
        for (int i = static_cast<int>(points.size()) - 2; i >= 0; --i) {
            const QVector2D delta = points[i] - end;
            points.push_back(end + QVector2D(-delta.y(), delta.x()));
        }
    }

    return points;
}
}

Window::Window()
{
    // Add label for FPS
    auto* fpsLabel = new QLabel("FPS: 0", this);
    fpsLabel->setStyleSheet("QLabel { color: white; }");
    fpsLabel->setAttribute(Qt::WA_TransparentForMouseEvents);
    fpsLabel->adjustSize();
    fpsLabel->move(0, 0);
    fpsLabel->raise();

    connect(this, &Window::fpsUpdated, [fpsLabel](double fps) {
        fpsLabel->setText(QString("FPS: %1").arg(fps, 0, 'f', 0));
        fpsLabel->adjustSize();
    });

    auto* iterationsLabel = new QLabel(QString("Iterations: %1").arg(iterations_), this);
    iterationsLabel->setStyleSheet("QLabel { color: white; }");
    iterationsLabel->setAttribute(Qt::WA_TransparentForMouseEvents);

    auto* iterationsSlider = new QSlider(Qt::Horizontal, this);
    iterationsSlider->setRange(0, 20);
    iterationsSlider->setValue(iterations_);

    auto* layout = new QVBoxLayout(this);
    layout->addStretch();
    layout->addWidget(iterationsLabel);
    layout->addWidget(iterationsSlider);

    connect(iterationsSlider, &QSlider::valueChanged, this, [this, iterationsLabel](int value) {
        iterations_ = value;
        shouldRebuild_ = true;
        iterationsLabel->setText(QString("Iterations: %1").arg(iterations_));
        update();
    });
}

Window::~Window()
{
    program_.reset();
}

void Window::onInit()
{
    // Configure shaders
    program_ = std::make_unique<QOpenGLShaderProgram>(this);
    program_->addShaderFromSourceFile(QOpenGLShader::Vertex, ":/shaders/template.vs");
    program_->addShaderFromSourceFile(QOpenGLShader::Fragment, ":/shaders/template.fs");
    if (!program_->link()) {
        qDebug() << "Ошибка линковки:" << program_->log();
    }
    program_->bind();

    // Configure uniforms
    mvpUniform_ = program_->uniformLocation("mvp");

    // Create VAO object
    vao_.create();
    vao_.bind();

    // Create VBO
    vbo_.create();
    vbo_.bind();
    vbo_.setUsagePattern(QOpenGLBuffer::DynamicDraw);

    // Bind attributes
    program_->enableAttributeArray(0);
    program_->setAttributeBuffer(0, GL_FLOAT, 0, 2, static_cast<int>(sizeof(QVector2D)));

    // Release all
    program_->release();

    vao_.release();
    vbo_.release();

    // Еnable depth test and face culling
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);

    // Clear all FBO buffers
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}

void Window::updateGeometry()
{
    const auto points = generateDragon(iterations_);
    pointCount_ = static_cast<GLsizei>(points.size());
    vbo_.bind();
    vbo_.allocate(points.data(), static_cast<int>(points.size() * sizeof(QVector2D)));
    vbo_.release();
    shouldRebuild_ = false;
}

void Window::onRender()
{
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    if (shouldRebuild_) {
        updateGeometry();
    }

    // Calculate MVP matrix
    model_.setToIdentity();
    model_.translate(-0.5f, 0.0f, -1.5f);
    model_.rotate(iterations_ * 45.0f, 0.0f, 0.0f, 1.0f);
    model_.scale(std::pow(2.0f, -0.5f * iterations_));
    view_.setToIdentity();
    view_.translate(-camera_.center.x(), -camera_.center.y(), 0.0f);
    projection_ = camera_.projection(size());
    const auto mvp = projection_ * view_ * model_;

    // Bind shader program
    program_->bind();

    // Update uniform values
    program_->setUniformValue(mvpUniform_, mvp);

    // Bind VAO
    vao_.bind();

    // Draw
    glDrawArrays(GL_LINE_STRIP, 0, pointCount_);

    // Release all
    vao_.release();
    program_->release();
}

void Window::onResize(const size_t width, const size_t height)
{
    // Configure viewport
    glViewport(0, 0, static_cast<GLint>(width), static_cast<GLint>(height));
}

void Window::mousePressEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton) {
        lastMousePosition_ = event->position();
    }
}

void Window::mouseMoveEvent(QMouseEvent *event)
{
    // check for dragging
    if (!(event->buttons() & Qt::LeftButton)) return;

    camera_.pan(event->position() - lastMousePosition_);
    lastMousePosition_ = event->position();
    update();
}

void Window::wheelEvent(QWheelEvent *event)
{
    const double steps = !event->pixelDelta().isNull()
        ? event->pixelDelta().y() / 40.0
        : event->angleDelta().y() / 120.0;

    camera_.zoomAt(event->position(), size(), std::pow(1.2, steps));
    update();
    event->accept();
}

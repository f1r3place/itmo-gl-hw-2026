#include "window.h"

#include <QLabel>
#include <QMouseEvent>
#include <QSlider>
#include <QVBoxLayout>
#include <QWheelEvent>

#include <array>
#include <cmath>

constexpr std::array<GLfloat, 6u> vertices = {
    0.0f, 0.5f,
    -0.5f, -0.5f,
    0.5f, -0.5f,
};

constexpr std::array<GLuint, 3u> indices = {0, 1, 2};

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

    auto* widthLabel = new QLabel("Width: 1.00", this);
    widthLabel->setStyleSheet("QLabel { color: white; }");
    fpsLabel->setAttribute(Qt::WA_TransparentForMouseEvents);

    auto* widthSlider = new QSlider(Qt::Horizontal, this);
    widthSlider->setRange(10, 300);
    widthSlider->setValue(100);

    auto* layout = new QVBoxLayout(this);
    layout->addStretch();
    layout->addWidget(widthLabel);
    layout->addWidget(widthSlider);

    connect(widthSlider, &QSlider::valueChanged, this, [this, widthLabel](int value) {
        triangleWidth_ = value / 100.0f;
        widthLabel->setText(QString("Width: %1").arg(triangleWidth_, 0, 'f', 2));
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
    vbo_.setUsagePattern(QOpenGLBuffer::StaticDraw);
    vbo_.allocate(vertices.data(), static_cast<int>(vertices.size() * sizeof(GLfloat)));

    // Create IBO
    ibo_.create();
    ibo_.bind();
    ibo_.setUsagePattern(QOpenGLBuffer::StaticDraw);
    ibo_.allocate(indices.data(), static_cast<int>(indices.size() * sizeof(GLuint)));

    // Bind attributes
    program_->enableAttributeArray(0);
    program_->setAttributeBuffer(0, GL_FLOAT, 0, 2, static_cast<int>(2 * sizeof(GLfloat)));

    // Release all
    program_->release();

    vao_.release();
    vbo_.release();
    ibo_.release();

    // Еnable depth test and face culling
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);

    // Clear all FBO buffers
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}

void Window::onRender()
{
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // Calculate MVP matrix
    model_.setToIdentity();
    model_.translate(0, 0, -1.5);
    model_.scale(triangleWidth_, 1.0f, 1.0f);
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
    glDrawElements(GL_TRIANGLES, 3, GL_UNSIGNED_INT, nullptr);

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

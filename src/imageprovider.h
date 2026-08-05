#pragma once

#include <QImage>
#include <QQuickImageProvider>

// Serves the backend's current image to QML. QML requests
// "image://omapic/<revision>"; the <revision> path segment busts Qt's cache.
class ImageProvider : public QQuickImageProvider {
public:
    ImageProvider() : QQuickImageProvider(QQuickImageProvider::Image) {}

    void setImage(const QImage &img) { m_image = img; }

    QImage requestImage(const QString &id, QSize *size,
                        const QSize &requestedSize) override;

private:
    QImage m_image;
};

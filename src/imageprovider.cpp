#include "imageprovider.h"

QImage ImageProvider::requestImage(const QString &id, QSize *size,
                                   const QSize &requestedSize) {
    Q_UNUSED(id);
    QImage img = m_image;
    if (size)
        *size = img.size();
    if (requestedSize.isValid() && !img.isNull())
        img = img.scaled(requestedSize, Qt::KeepAspectRatio, Qt::SmoothTransformation);
    return img;
}

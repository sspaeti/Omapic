#include "backend.h"
#include "filepicker.h"
#include "imageprovider.h"

#include <QBuffer>
#include <QDate>
#include <QDateTime>
#include <QDir>
#include <QProcess>

Backend::Backend(QObject *parent) : QObject(parent) {}
Backend::Backend(ImageProvider *provider, FilePicker *picker, QObject *parent)
    : QObject(parent), m_provider(provider), m_picker(picker) {
    if (m_picker) {
        connect(m_picker, &FilePicker::openSelected, this,
                [this](const QUrl &url) { load(url); });
        connect(m_picker, &FilePicker::failed, this,
                [this](const QString &m) { setStatus(m); });
    }
}

QImage Backend::cut(const QImage &src, Orientation o, int start, int end) {
    if (src.isNull())
        return src;
    if (o == Horizontal) {
        const int h = src.height();
        start = qBound(0, start, h);
        end = qBound(start, end, h);
        const int band = end - start;
        if (band <= 0)
            return src;
        QImage out(src.width(), h - band, src.format());
        // rows [0, start)
        for (int y = 0; y < start; ++y)
            memcpy(out.scanLine(y), src.scanLine(y), src.bytesPerLine());
        // rows [end, h) pulled up
        for (int y = end; y < h; ++y)
            memcpy(out.scanLine(y - band), src.scanLine(y), src.bytesPerLine());
        return out;
    } else {
        const int w = src.width();
        start = qBound(0, start, w);
        end = qBound(start, end, w);
        const int band = end - start;
        if (band <= 0)
            return src;
        QImage out(w - band, src.height(), src.format());
        for (int y = 0; y < src.height(); ++y) {
            const QRgb *in = reinterpret_cast<const QRgb *>(src.scanLine(y));
            QRgb *o = reinterpret_cast<QRgb *>(out.scanLine(y));
            for (int x = 0; x < start; ++x)
                o[x] = in[x];
            for (int x = end; x < w; ++x)
                o[x - band] = in[x];
        }
        return out;
    }
}

void Backend::setImage(const QImage &img) {
    m_image = img;
    ++m_revision;
    if (m_provider)
        m_provider->setImage(m_image);
    emit imageChanged();
}

void Backend::setStatus(const QString &s) {
    m_status = s;
    emit statusChanged();
}

bool Backend::load(const QUrl &url) {
    const QString path = url.isLocalFile() ? url.toLocalFile() : url.toString();
    QImage img(path);
    if (img.isNull()) {
        setStatus(QStringLiteral("Could not open: %1").arg(path));
        return false;
    }
    // Normalize to 32-bit so the cut engine's QRgb access is always valid.
    img = img.convertToFormat(QImage::Format_ARGB32);
    m_source = url;
    m_undo.clear();
    setImage(img);
    setStatus(QStringLiteral("Loaded %1×%2").arg(img.width()).arg(img.height()));
    return true;
}

bool Backend::loadClipboard() {
    QProcess proc;
    proc.start(QStringLiteral("wl-paste"),
               {QStringLiteral("--type"), QStringLiteral("image/png")});
    if (!proc.waitForStarted(2000) || !proc.waitForFinished(3000)) {
        setStatus(QStringLiteral("wl-paste not available"));
        return false;
    }
    const QByteArray data = proc.readAllStandardOutput();
    QImage img;
    if (!img.loadFromData(data, "PNG")) {
        setStatus(QStringLiteral("Clipboard has no image"));
        return false;
    }
    img = img.convertToFormat(QImage::Format_ARGB32);
    m_source = QUrl();
    m_undo.clear();
    setImage(img);
    setStatus(QStringLiteral("Loaded from clipboard"));
    return true;
}

void Backend::applyCut(int orientation, int start, int end) {
    if (m_image.isNull())
        return;
    const Orientation o = static_cast<Orientation>(orientation);
    QImage next = cut(m_image, o, start, end);
    if (next.size() == m_image.size())
        return;                            // nothing removed
    m_undo.append(m_image);
    setImage(next);
    setStatus(QStringLiteral("Cut applied"));
}

void Backend::undo() {
    if (m_undo.isEmpty())
        return;
    setImage(m_undo.takeLast());
    setStatus(QStringLiteral("Undone"));
}

void Backend::copyToClipboard() {
    if (m_image.isNull())
        return;
    QByteArray png;
    QBuffer buf(&png);
    buf.open(QIODevice::WriteOnly);
    m_image.save(&buf, "PNG");
    buf.close();

    QProcess proc;
    proc.start(QStringLiteral("wl-copy"),
               {QStringLiteral("--type"), QStringLiteral("image/png")});
    if (!proc.waitForStarted(2000)) {
        setStatus(QStringLiteral("wl-copy not available"));
        return;
    }
    proc.write(png);
    proc.closeWriteChannel();
    proc.waitForFinished(2000);
    setStatus(QStringLiteral("Copied to clipboard"));
}

QString Backend::save() {
    if (m_image.isNull())
        return {};

    QString pics = qEnvironmentVariable("XDG_PICTURES_DIR");
    if (pics.isEmpty())
        pics = QDir::homePath() + QStringLiteral("/Pictures");

    const QString month = QDate::currentDate().toString(QStringLiteral("yyyy-MM"));
    const QString dir = pics + QStringLiteral("/Printscreen/") + month;
    QDir().mkpath(dir);

    const QString stamp =
        QDateTime::currentDateTime().toString(QStringLiteral("yyyy-MM-dd_HH-mm-ss"));
    const QString path = dir + QStringLiteral("/screenshot-") + stamp + QStringLiteral(".png");

    if (!m_image.save(path, "PNG")) {
        setStatus(QStringLiteral("Save failed"));
        return {};
    }
    setStatus(QStringLiteral("Saved %1").arg(path));
    return path;
}

void Backend::openDialog() {
    if (m_picker)
        m_picker->openImage();
}

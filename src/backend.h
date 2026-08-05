#pragma once

#include <QImage>
#include <QList>
#include <QObject>
#include <QString>
#include <QUrl>

class ImageProvider;
class FilePicker;

// Owns the current image and every operation on it. The cut() functions are
// static and pure so they can be unit-tested without a running application.
class Backend : public QObject {
    Q_OBJECT
    Q_PROPERTY(int revision READ revision NOTIFY imageChanged)
    Q_PROPERTY(int imageWidth READ imageWidth NOTIFY imageChanged)
    Q_PROPERTY(int imageHeight READ imageHeight NOTIFY imageChanged)
    Q_PROPERTY(bool hasImage READ hasImage NOTIFY imageChanged)
    Q_PROPERTY(bool canUndo READ canUndo NOTIFY imageChanged)
    Q_PROPERTY(QString status READ status NOTIFY statusChanged)

public:
    enum Orientation { Horizontal, Vertical };
    Q_ENUM(Orientation)

    explicit Backend(QObject *parent = nullptr);
    Backend(ImageProvider *provider, FilePicker *picker, QObject *parent = nullptr);

    // --- pure cut engine (tested) ---
    // Removes the band [start, end) along the orientation axis and collapses.
    // Horizontal removes rows (reduces height); Vertical removes columns.
    static QImage cut(const QImage &src, Orientation o, int start, int end);

    int revision() const { return m_revision; }
    int imageWidth() const { return m_image.width(); }
    int imageHeight() const { return m_image.height(); }
    bool hasImage() const { return !m_image.isNull(); }
    bool canUndo() const { return !m_undo.isEmpty(); }
    QString status() const { return m_status; }
    const QImage &image() const { return m_image; }

    Q_INVOKABLE bool load(const QUrl &url);
    Q_INVOKABLE bool loadClipboard();
    Q_INVOKABLE void applyCut(int orientation, int start, int end);
    Q_INVOKABLE void undo();
    Q_INVOKABLE void copyToClipboard();
    Q_INVOKABLE QString save();
    Q_INVOKABLE void openDialog();

signals:
    void imageChanged();
    void statusChanged();

private:
    void setImage(const QImage &img);   // pushes provider + bumps revision
    void setStatus(const QString &s);

    ImageProvider *m_provider = nullptr;
    FilePicker *m_picker = nullptr;
    QImage m_image;
    QUrl m_source;
    QList<QImage> m_undo;
    int m_revision = 0;
    QString m_status;
};

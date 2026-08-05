#pragma once

#include <QObject>
#include <QUrl>

class FilePicker : public QObject {
    Q_OBJECT
public:
    explicit FilePicker(QObject *parent = nullptr) : QObject(parent) {}
    virtual ~FilePicker() = default;
    virtual void openImage() = 0;

signals:
    void openSelected(const QUrl &url);
    void failed(const QString &message);
};

#include <QtTest>
#include <QImage>
#include <QTemporaryFile>
#include <QTemporaryDir>
#include <QDir>
#include <QDate>
#include <QFile>
#include "backend.h"

// row y gets color rgb(y, 0, 0) so each row is uniquely identifiable.
static QImage stripes(int w, int h) {
    QImage img(w, h, QImage::Format_RGB32);
    for (int y = 0; y < h; ++y)
        for (int x = 0; x < w; ++x)
            img.setPixel(x, y, qRgb(y, 0, 0));
    return img;
}

class BackendTests : public QObject {
    Q_OBJECT
private slots:
    void horizontalCutRemovesBandAndCollapses() {
        QImage src = stripes(4, 6);
        QImage out = Backend::cut(src, Backend::Horizontal, 2, 4);

        QCOMPARE(out.width(), 4);
        QCOMPARE(out.height(), 4);
        // Rows 0,1 unchanged; rows 4,5 pulled up to y=2,3.
        const int expectedRowSource[4] = {0, 1, 4, 5};
        for (int y = 0; y < 4; ++y)
            QCOMPARE(qRed(out.pixel(0, y)), expectedRowSource[y]);
    }

    void verticalCutRemovesBandAndCollapses() {
        // column x -> rgb(x,0,0)
        QImage src(5, 3, QImage::Format_RGB32);
        for (int y = 0; y < 3; ++y)
            for (int x = 0; x < 5; ++x)
                src.setPixel(x, y, qRgb(x, 0, 0));

        QImage out = Backend::cut(src, Backend::Vertical, 1, 3);

        QCOMPARE(out.width(), 3);
        QCOMPARE(out.height(), 3);
        const int expectedColSource[3] = {0, 3, 4};
        for (int x = 0; x < 3; ++x)
            QCOMPARE(qRed(out.pixel(x, 0)), expectedColSource[x]);
    }

    void applyCutThenUndoRestores() {
        // Write a 4x6 striped PNG to a temp file, load it, cut, undo.
        QTemporaryFile f;
        f.setFileTemplate(QDir::tempPath() + "/omapicXXXXXX.png");
        QVERIFY(f.open());
        stripes(4, 6).save(f.fileName(), "PNG");

        Backend b;
        QVERIFY(b.load(QUrl::fromLocalFile(f.fileName())));
        QCOMPARE(b.imageHeight(), 6);
        QVERIFY(!b.canUndo());

        b.applyCut(Backend::Horizontal, 2, 4);
        QCOMPARE(b.imageHeight(), 4);
        QVERIFY(b.canUndo());

        b.undo();
        QCOMPARE(b.imageHeight(), 6);
        QVERIFY(!b.canUndo());
    }

    void edgeBandActsLikeCrop() {
        QImage out = Backend::cut(stripes(4, 6), Backend::Horizontal, 0, 2);
        QCOMPARE(out.height(), 4);
        QCOMPARE(qRed(out.pixel(0, 0)), 2); // top two rows removed
    }

    void saveWritesTimestampedPngInMonthlyFolder() {
        QTemporaryDir dir;
        QVERIFY(dir.isValid());
        qputenv("XDG_PICTURES_DIR", dir.path().toUtf8());

        QTemporaryFile f;
        f.setFileTemplate(QDir::tempPath() + "/omapicXXXXXX.png");
        QVERIFY(f.open());
        stripes(8, 8).save(f.fileName(), "PNG");

        Backend b;
        QVERIFY(b.load(QUrl::fromLocalFile(f.fileName())));
        const QString out = b.save();

        QVERIFY(!out.isEmpty());
        QVERIFY(QFile::exists(out));
        const QString month = QDate::currentDate().toString("yyyy-MM");
        QVERIFY(out.contains("/Printscreen/" + month + "/"));
        QVERIFY(out.endsWith(".png"));
        // Source file is untouched.
        QVERIFY(QFile::exists(f.fileName()));
    }
};

QTEST_MAIN(BackendTests)
#include "backend_tests.moc"

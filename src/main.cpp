// omapic — a dead-simple image cut-out tool. Qt Quick (QML) UI, QImage cuts.
#include <QGuiApplication>
#include <QIcon>
#include <QQmlApplicationEngine>
#include <QQuickStyle>
#include <QUrl>

int main(int argc, char *argv[]) {
    QGuiApplication app(argc, argv);
    app.setApplicationName("omapic");
    app.setDesktopFileName("omapic");
    app.setWindowIcon(QIcon::fromTheme("omapic"));
    QQuickStyle::setStyle("Material");

    QQmlApplicationEngine engine;
    engine.load(QUrl("qrc:/Main.qml"));
    if (engine.rootObjects().isEmpty())
        return -1;

    return app.exec();
}

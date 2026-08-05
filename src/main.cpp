// omapic — a dead-simple image cut-out tool. Qt Quick (QML) UI, QImage cuts.
#include <QGuiApplication>
#include <QIcon>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QQuickStyle>
#include <QUrl>

#include "backend.h"
#include "imageprovider.h"
#include "portalfilepicker.h"

int main(int argc, char *argv[]) {
    QGuiApplication app(argc, argv);
    app.setApplicationName("omapic");
    app.setDesktopFileName("omapic");
    app.setWindowIcon(QIcon::fromTheme("omapic"));
    QQuickStyle::setStyle("Material");

    auto *provider = new ImageProvider();
    auto *picker = new PortalFilePicker(&app);
    Backend backend(provider, picker, &app);

    QQmlApplicationEngine engine;
    engine.addImageProvider("omapic", provider);   // engine takes ownership
    engine.rootContext()->setContextProperty("backend", &backend);
    engine.load(QUrl("qrc:/Main.qml"));
    if (engine.rootObjects().isEmpty())
        return -1;

    // CLI: `omapic --clipboard` or `omapic <file>`
    const QStringList args = app.arguments();
    if (args.contains(QStringLiteral("--clipboard"))) {
        backend.loadClipboard();
    } else if (args.size() > 1 && !args.at(1).startsWith(QStringLiteral("--"))) {
        backend.load(QUrl::fromLocalFile(args.at(1)));
    }

    return app.exec();
}

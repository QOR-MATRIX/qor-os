#include <QtGui/QGuiApplication>
#include <QtQml/QQmlApplicationEngine>
#include <QtQuickControls2/QQuickStyle>

int main(int argc, char *argv[])
{
    QGuiApplication app(argc, argv);

    // On a QTOS device the platform already selects Lumen; setting it here
    // keeps the app consistent when run from an IDE on another desktop.
    QQuickStyle::setStyle(QStringLiteral("Lumen"));

    QQmlApplicationEngine engine;
    engine.loadFromModule("Hello.Lumen", "Main");
    return engine.rootObjects().isEmpty() ? 1 : app.exec();
}

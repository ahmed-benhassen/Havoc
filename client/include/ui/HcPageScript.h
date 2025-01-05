#ifndef HAVOCCLIENT_HCPAGESCRIPT_H
#define HAVOCCLIENT_HCPAGESCRIPT_H

#include <QtCore/QVariant>
#include <QtWidgets/QApplication>
#include <QtWidgets/QGridLayout>
#include <QtWidgets/QHeaderView>
#include <QtWidgets/QLabel>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QSpacerItem>
#include <QtWidgets/QSplitter>
#include <QtWidgets/QTabWidget>
#include <QtWidgets/QTableWidget>
#include <QtWidgets/QWidget>

#include <ui/HcConsole.h>
#include <ui/HcStoreWidget.h>

QT_BEGIN_NAMESPACE

class HcPyConsole : public QTextEdit {
    Q_OBJECT

public:
    explicit HcPyConsole(
        QWidget* parent = nullptr
    );
};

class HcPagePlugins : public QWidget
{
Q_OBJECT

public:
    QGridLayout*   gridLayout         = nullptr;
    QGridLayout*   gridLayout_2       = nullptr;
    QSplitter*     splitter           = nullptr;
    QSpacerItem*   horizontalSpacer   = nullptr;
    QTabWidget*    TabWidget          = nullptr;
    QWidget*       TabPluginManager   = nullptr;
    QPushButton*   ButtonLoad         = nullptr;
    QLabel*        LabelLoadedPlugins = nullptr;
    QTableWidget*  TablePluginsWidget = nullptr;
    HcPyConsole*   PyConsole          = nullptr;
    HcStoreWidget* TabPluginStore     = nullptr;

    std::optional<py11::object> LoadCallback = {};

    explicit HcPagePlugins();

    auto retranslateUi() -> void;

    auto LoadScript(
        const std::string& path
    ) -> void;

    auto AddScriptPath(
        const QString& path,
        const QString& error = ""
    ) -> void;

    auto processPlugins(
        void
    ) -> void;

    auto contextMenu(
        const QPoint &pos
    ) -> void;

signals:
    auto SignalConsoleWrite(
        const QString& text
    ) -> void;
};

QT_END_NAMESPACE

#endif

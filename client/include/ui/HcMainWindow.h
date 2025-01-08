#ifndef HAVOCCLIENT_HCMAINWINDOW_H
#define HAVOCCLIENT_HCMAINWINDOW_H

#include <Havoc.h>

#include <QtCore/QVariant>
#include <QtWidgets/QApplication>
#include <QtWidgets/QGridLayout>
#include <QtWidgets/QMainWindow>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QSpacerItem>
#include <QtWidgets/QStackedWidget>
#include <QtWidgets/QWidget>

QT_BEGIN_NAMESPACE

class HavocButton : public QPushButton
{
    bool    buttonUsed         = false;
    QString originalObjectName = {};
public:
    explicit HavocButton( QWidget* w );

    auto currentlyUsed() const -> bool;
    auto setUsed( bool state ) -> void;
};

class HcMainWindow final : public QMainWindow
{
    Q_OBJECT

    std::mutex agent_mutex;

public:
    QWidget*            MainWidget      = nullptr;
    QGridLayout*        gridLayout      = nullptr;
    QGridLayout*        gridLayout_2    = nullptr;
    QWidget*            SideBarWidget   = nullptr;
    HavocButton*        ButtonHavoc     = nullptr;
    HavocButton*        ButtonListeners = nullptr;
    HavocButton*        ButtonSettings  = nullptr;
    HavocButton*        ButtonScripts   = nullptr;
    HavocButton*        ButtonAgents    = nullptr;
    HavocButton*        ButtonServer    = nullptr;
    QSpacerItem*        Spacer          = nullptr;
    QStackedWidget*     StackedWidget   = nullptr;
    HcPageAgent*        PageAgent       = nullptr;
    HcPageListener*     PageListener    = nullptr;
    QWidget*            PageServer      = nullptr;
    HcPagePlugins*      PageScripts     = nullptr;
    QWidget*            PageSettings    = nullptr;

    explicit HcMainWindow( );

    auto retranslateUi() -> void;
    auto renderWindow() -> void;
    auto unusedSideButtons() -> void;
    auto useSideButton(
        HavocButton*   button,
        const QString& name
    ) -> void;

    auto switchPageAgent() -> void;
    auto switchPageListener() -> void;
    auto switchPageServer() -> void;
    auto switchPageScripts() -> void;
    auto switchPageSettings() -> void;

public Q_SLOT:
    auto MessageBox(
        QMessageBox::Icon  icon,
        const std::string& title,
        const std::string& text
    ) -> void;

    auto AddListener(
        const json& listener
    ) const -> void;

    auto AddAgent(
        const json& metadata
    ) -> std::optional<HcAgent*>;

    auto RemoveAgent(
        const std::string& uuid
    ) -> void;

    auto AgentConsole(
        const json& console
    ) -> void;

signals:
    auto signalBuildLog(
        const QString& text
    ) -> void;
};

QT_END_NAMESPACE

#endif //HAVOCCLIENT_HCMAINWINDOW_H

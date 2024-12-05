#ifndef HAVOCCLIENT_HCPAGEAGENT_H
#define HAVOCCLIENT_HCPAGEAGENT_H

#include <QStackedWidget>
#include <QtWidgets/QSplitter>
#include <QtWidgets/QCompleter>
#include <QtWidgets/QTableWidget>
#include <QtWidgets/QWidget>
#include <QtWidgets/QStyledItemDelegate>
#include <QStandardItemModel>

#include <ui/HcConsole.h>
#include <ui/HcSessionGraph.h>
#include <core/HcAgent.h>

#include <DockManager.h>
#include <DockWidget.h>

QT_BEGIN_NAMESPACE

extern std::map<
    std::string,
    std::vector<std::pair<std::string, std::string>>
>* HcAgentCompletionList;

class HcAgentLineEdit final : public QLineEdit
{
    QTimer* m_timer          = {};
    bool    m_completerShown = {};
    QLabel* m_label          = {};

public:
    explicit HcAgentLineEdit(
        QWidget* parent
    );

    auto setPrefixLabel(
        QLabel* label
    ) -> void;

    auto CursorRect() const -> QRect;

protected:
    auto resizeEvent(
        QResizeEvent* event
    ) -> void override;

private:
    auto updatePrefixPosition() const -> void;
    auto updateMargins() -> void;

private slots:
    auto onTextEdited(
        const QString& text
    ) -> void;

    auto showCompleter(
        void
    ) -> void;
};

class HcAgentCompleter final : public QCompleter
{
    QStandardItemModel*                              _model;
    std::vector<std::pair<std::string, std::string>> _items;

public:
    explicit HcAgentCompleter( QObject* parent = nullptr );
    ~HcAgentCompleter() override;

    auto addCommand(
        const std::string& command,
        const std::string& description
    ) -> void;

    auto getCurrentCompletions() -> QStringList;
    auto findLongestCommonPrefix( const QStringList& strings ) -> QString;

protected:
    auto eventFilter(
        QObject* parent,
        QEvent*  event
    ) -> bool override;
};

class HcDescriptionDelegate : public QStyledItemDelegate {

public:
    HcDescriptionDelegate( QCompleter* completer, QObject* parent = {} );

    auto paint(
        QPainter*                   painter,
        const QStyleOptionViewItem& option,
        const QModelIndex&          index
    ) const -> void override;

private:
    QCompleter *completer;
};

class HcAgentConsole final : public QWidget {
    HcAgent*          Meta        = {};
    HcAgentCompleter* Completer   = {};
    QGridLayout*      gridLayout  = {};
    QTextEdit*        Console     = {};
    HcAgentLineEdit*  Input       = {};

public:
    QLabel* LabelHeader = {};
    QLabel* LabelBottom = {};
    QLabel* LabelInput  = {};

    explicit HcAgentConsole(
        HcAgent* meta,
        QWidget* parent = nullptr
    );

    auto setHeaderLabel(
        const QString& text
    ) -> void;

    auto setBottomLabel(
        const QString& text
    ) -> void;

    auto setInputLabel(
        const QString& text
    ) -> void;

    auto inputEnter() -> void;

    auto appendConsole(
        const QString& text
    ) const -> void;

    auto addCompleteCommand(
        const std::string& command,
        const std::string& description
    ) const -> void;
};

class HcPageAgent final : public QWidget
{
    bool                  SplitterMoveToggle  = false;
    ads::CDockAreaWidget* ConsoleAreaWidget   = {};

public:
    ads::CDockManager* DockManager            = {};
    QGridLayout*       gridLayout             = {};
    QSplitter*         Splitter               = {};
    QStackedWidget*    StackedWidget          = {};
    QTableWidget*      AgentTable             = {};
    HcSessionGraph*    AgentGraph             = {};
    QLabel*            AgentDisplayerElevated = {};
    QLabel*            AgentDisplayerSessions = {};
    QLabel*            AgentDisplayerPivots   = {};
    QToolButton*       AgentActionButton      = {};
    QMenu*             AgentActionMenu        = {};
    QAction*           ActionPayload          = {};
    QAction*           ActionShowHidden       = {};
    QSpacerItem*       horizontalSpacer       = {};

    QTableWidgetItem*  TitleAgentID           = {};
    QTableWidgetItem*  TitleInternal          = {};
    QTableWidgetItem*  TitleUsername          = {};
    QTableWidgetItem*  TitleHostname          = {};
    QTableWidgetItem*  TitleSystem            = {};
    QTableWidgetItem*  TitleProcessID         = {};
    QTableWidgetItem*  TitleProcessName       = {};
    QTableWidgetItem*  TitleProcessArch       = {};
    QTableWidgetItem*  TitleThreadID          = {};
    QTableWidgetItem*  TitleNote              = {};
    QTableWidgetItem*  TitleLastCallback      = {};

    std::vector<HcAgent*> agents      = {};
    int                   pivots      = {};
    int                   elevated    = {};
    bool                  show_hidden = false;

    explicit HcPageAgent(QWidget* parent = nullptr );
    ~HcPageAgent();

    auto retranslateUi() -> void;

    auto addTab(
        const QString& name,
        QWidget*       widget
    ) -> void;

    auto addTab(
        const QString& name,
        const QIcon&   icon,
        QWidget*       widget
    ) -> void;

    auto addAgent(
        HcAgent* agent
    ) -> void;

    auto removeAgent(
        const std::string& uuid
    ) -> void;

    auto spawnAgentConsole(
        const std::string& uuid
    ) -> void;

    auto Agent(
        const std::string& uuid
    ) -> std::optional<HcAgent*>;

    auto AgentConsole(
        const std::string& uuid,
        const std::string& format,
        const std::string& output = ""
    ) -> void;

private Q_SLOTS:
    auto handleAgentMenu(
        const QPoint& pos
    ) -> void;

    auto handleAgentDoubleClick(
        const QModelIndex& index
    ) -> void;

    auto itemChanged(
        QTableWidgetItem *item
    ) -> void;

    auto actionShowHidden(
        bool checked
    ) -> void;

    auto actionPayloadBuilder(
        bool checked
    ) -> void;

    auto actionTriggered(
        QAction* triggered
    ) -> void;
};

QT_END_NAMESPACE

#endif //HAVOCCLIENT_HCPAGEAGENT_H

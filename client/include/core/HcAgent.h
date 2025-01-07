#ifndef HAVOCCLIENT_HCAGENT_H
#define HAVOCCLIENT_HCAGENT_H

class HcSessionGraphItem;
class HcAgentConsole;
class HcAgent;

#include <Common.h>
#include <IHcAgent.h>

#include <QTableWidgetItem>

class HcAgentSignals final : public QObject {
    Q_OBJECT

signals:
    auto ConsoleWrite(
        const QString& uuid,
        const QString& text
    ) -> void;

    auto HeartBeat(
        const QString& uuid,
        const QString& time
    ) -> void;

    auto RegisterIcon(
        const QString& uuid,
        const QImage&  icon
    ) -> void;

    auto RegisterIconName(
        const QString& uuid,
        const QString& name
    ) -> void;
};

class HcAgentTableItem;

class HcAgent : public IHcAgent {
    std::string _uuid;
    std::string _type;
    std::string _parent;
    std::string _status;
    QDateTime   _last;
    json        _data;
    bool        _elevated;

    std::optional<py11::object> _interface;
    HcAgentConsole*             _console;
    bool                        _hidden;
    QImage                      _image;
public:
    struct {
        HcSessionGraphItem* node;

        struct {
            HcAgentTableItem* Uuid;
            HcAgentTableItem* Internal;
            HcAgentTableItem* Username;
            HcAgentTableItem* Hostname;
            HcAgentTableItem* ProcessPath;
            HcAgentTableItem* ProcessName;
            HcAgentTableItem* ProcessId;
            HcAgentTableItem* ThreadId;
            HcAgentTableItem* Arch;
            HcAgentTableItem* System;
            HcAgentTableItem* Note;
            HcAgentTableItem* Last;
        } table;

        HcAgentSignals signal = {};
    } ui;

    explicit HcAgent(
        const json& metadata
    );

    ~HcAgent();

    auto uuid() -> std::string override;
    auto type() -> std::string override;

    auto parent() -> std::string override;
    auto setParent(
        const std::string& parent
    ) -> void override;

    auto status() -> std::string override;
    auto setStatus(
        HcAgentStatus status
    ) -> void override;

    auto setLast(
        const std::string& last
    ) -> QDateTime override;
    auto last() -> QDateTime override;
    auto data() -> nlohmann::json override;
    auto interface() -> std::optional<pybind11::object> override;

    auto hidden() -> bool override;
    auto setHidden(
        bool hidden
    ) -> void override;

    auto elevated() const -> bool;

    auto image() -> QImage override;
    auto setImage(
        const QImage& image
    ) -> void override;

    auto writeConsole(
        const std::string& text
    ) -> void override;

    auto remove() -> void override;

    //
    // custom HcAgent methods
    //
    auto initialize() -> bool;
    auto post() -> void;
    auto console() const -> HcAgentConsole*;

    auto hide() -> void;
    auto unhide() -> void;
};

class HcAgentTableItem final : public QTableWidgetItem {
public:
    HcAgent* agent  = {};
    bool     ignore = true;

    explicit HcAgentTableItem(
        const QString&          value,
        HcAgent*                agent,
        const Qt::ItemFlag      flags = Qt::ItemIsEditable,
        const Qt::AlignmentFlag align = Qt::AlignCenter
    );
};

#endif //HAVOCCLIENT_HCAGENT_H

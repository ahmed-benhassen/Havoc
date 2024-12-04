#ifndef IHCAGENT_H
#define IHCAGENT_H

#include <QImage>
#include <string>
#include <nlohmann/json.hpp>
#include <pybind11/pybind11.h>

enum HcAgentStatus {
    AgentStatusHealthy,
    AgentStatusUnresponsive,
    AgentStatusDisconnected,
};

class IHcAgent {
public:
    virtual ~IHcAgent() = default;

    virtual auto uuid() -> std::string = 0;
    virtual auto type() -> std::string = 0;

    virtual auto parent() -> std::string = 0;
    virtual auto setParent(
        const std::string& parent
    ) -> void = 0;

    virtual auto status() -> std::string = 0;
    virtual auto setStatus(
        HcAgentStatus status
    ) -> void = 0;

    virtual auto setLast(
        const std::string& last
    ) -> QDateTime = 0;
    virtual auto last() -> QDateTime = 0;
    virtual auto data() -> nlohmann::json = 0;
    virtual auto interface() -> std::optional<pybind11::object> = 0;

    virtual auto hidden() -> bool = 0;
    virtual auto setHidden(
        bool hidden
    ) -> void = 0;

    virtual auto image() -> QImage = 0;
    virtual auto setImage(
        const QImage& image
    ) -> void = 0;

    virtual auto writeConsole(
        const std::string& text
    ) -> void = 0;

    virtual auto remove() -> void = 0;
};

#endif //IHCAGENT_H

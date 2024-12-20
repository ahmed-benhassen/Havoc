#ifndef HCINTERFACE_IHCAPPLICATION_H
#define HCINTERFACE_IHCAPPLICATION_H

#include <QWidget>
#include <QMainWindow>
#include <string>

#include <IHcAgent.h>

template <typename T>
using HcFnCallbackCtx = auto ( * ) ( const T& context ) -> void;
using HcFnCallback    = auto ( * ) ( void ) -> void;

class IHcApplication {

public:
    virtual ~IHcApplication() = default;

    //
    // generic ui apis
    //

    virtual auto StyleSheet() -> QString = 0;
    virtual auto MainWindowWidget() -> QMainWindow* = 0;

    //
    // agent page apis
    //

    virtual auto PageAgentAddTab(
        const std::string& name,
        const QIcon&       icon,
        QWidget*           widget
    ) -> void = 0;

    //
    // register actions callbacks
    //

    /*!
     * @brief
     *  register agent action
     *
     * @param action_name
     *  action name
     *
     * @param action_icon
     *  action icon
     *
     * @param action_func
     *  agent function
     *
     * @param agent_type
     *  agent type to only
     *  specify the action to
     */
    virtual auto RegisterAgentAction(
        const std::string&           action_name,
        const QIcon&                 action_icon,
        HcFnCallbackCtx<std::string> action_func,
        const std::string&           agent_type
    ) -> void = 0;

    /*!
     * @brief
     *  register agent action for
     *  all available agents
     *
     * @param action_name
     *  action name
     *
     * @param action_icon
     *  action icon
     *
     * @param action_func
     *  agent function
     */
    virtual auto RegisterAgentAction(
        const std::string&           action_name,
        const QIcon&                 action_icon,
        HcFnCallbackCtx<std::string> action_func
    ) -> void = 0;

    /*!
     * @brief
     *  register agent action for
     *  all available agents
     *
     * @param action_name
     *  action name
     *
     * @param action_func
     *  agent function
     *
     * @param multi_select
     *  add the action to
     *  the multi select
     */
    virtual auto RegisterAgentAction(
        const std::string&           action_name,
        HcFnCallbackCtx<std::string> action_func,
        bool                         multi_select
    ) -> void = 0;

    /*!
     * @brief
     *  register an action under
     *  the action menu
     *
     * @param action_name
     *  action name
     *
     * @param action_icon
     *  action icon
     *
     * @param action_func
     *  action callback
     */
    virtual auto RegisterMenuAction(
        const std::string& action_name,
        const QIcon&       action_icon,
        HcFnCallback       action_func
    ) -> void = 0;

    //
    // agent api
    //

    virtual auto Agent(
        const std::string& uuid
    ) -> std::optional<IHcAgent*> = 0;

    //
    // python api
    //

    /*!
     * @brief
     *  run function under the context of the gil
     *
     * @param function
     *  function to run that requires the
     *  current thread context to hold the
     *  python global interpreter lock
     *
     * @param concurrent
     *  if the specified function needs to be
     *  fun under a concurrent task/thread
     *
     * @return
     *  the specified function raises any kind
     *  of python related or native exceptions
     *  it will be returned as a std::exception
     */
    virtual auto PythonContextRun(
        std::function<void()> function,
        bool                  concurrent
    ) -> void = 0;
};

#endif //HCINTERFACE_IHCAPPLICATION_H

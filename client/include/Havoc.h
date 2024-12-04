#ifndef HAVOCCLIENT_HAVOC_H
#define HAVOCCLIENT_HAVOC_H

#include <Common.h>
#include <Events.h>

class HcMainWindow;

/* Havoc include */
#include <core/HcHelper.h>
#include <core/HcEventWorker.h>
#include <core/HcHeartbeatWorker.h>
#include <core/HcMetaWorker.h>
#include <core/HcPluginManager.h>

#include <ui/HcPageAgent.h>
#include <ui/HcPageListener.h>
#include <ui/HcPageScript.h>

#include <ui/HcPayloadBuild.h>
#include <ui/HcListenerDialog.h>
#include <ui/HcMainWindow.h>
#include <ui/HcConnectDialog.h>
#include <ui/HcLineEdit.h>
#include <ui/HcTheme.h>
#include <ui/HcSessionGraph.h>

#include <api/Engine.h>

#define HAVOC_VERSION  "1.0"
#define HAVOC_CODENAME "King Of The Damned"

template <class HcTypeWorker>
class HcWorker {
public:
    QThread*      Thread;
    HcTypeWorker* Worker;

    inline explicit HcWorker() : Thread( new QThread ), Worker( new HcTypeWorker ) {
        static_cast<QThread*>( Worker )->moveToThread( Thread );
    }
};

class HcApplication final : public QWidget {

    using toml_t = toml::basic_value<toml::discard_comments, std::unordered_map, std::vector>;

    struct NamedObject {
        std::string  name;
        py11::object object;
    };

public:
    struct ActionObject {
        ActionObject();

        enum ActionType {
            ActionHavoc,
            ActionListener,
            ActionAgent,
        } type;

        struct {
            std::string type;
        } agent;

        struct {
            std::string type;
        } listener;

        std::string name;
        std::string icon;

        HcFnCallbackCtx<void*> callback;
        py11::object           callback_py;
    };

private:
    //
    // current connection info
    //
    struct {
        std::string name;
        std::string host;
        std::string port;
        std::string user;
        std::string pass;
        std::string token;
        std::string ssl_hash;
    } session;

    HcWorker<HcEventWorker>     Events;
    HcWorker<HcHeartbeatWorker> Heartbeat;
    HcWorker<HcMetaWorker>      MetaWorker;

    std::vector<json>          listeners   = {};
    std::vector<NamedObject>   protocols   = {};
    std::vector<NamedObject>   builders    = {};
    std::vector<NamedObject>   agents      = {};
    std::vector<NamedObject>   callbacks   = {};
    std::vector<NamedObject>   init_events = {};
    std::vector<ActionObject*> actions     = {};

    QDir client_dir = {};

public:
    HcPluginManager* plugin_manager = {};
    HcMainWindow*    ui             = {};
    QSplashScreen*   splash         = {};
    HcTheme          Theme;

    //
    // toml based profiles
    // and configurations
    //
    toml_t config  = {};
    toml_t profile = {};

    HcPyEngine* PyEngine;

    /* havoc client constructor and destructor */
    explicit HcApplication();
    ~HcApplication() override;

    /* client entrypoint */
    auto Main(
        int    argc,
        char** argv
    ) -> void;

    /* exit application and free resources */
    static auto Exit(
        void
    ) -> void;

    /* get server address */
    auto Server(
        void
    ) const -> std::string;

    /* get server connection token */
    auto Token(
        void
    ) const -> std::string;

    static auto StyleSheet(
        void
    ) -> QByteArray;

    auto SetupThreads(
        void
    ) -> void;

    auto SetupDirectory(
        void
    ) -> bool;

    inline auto directory(
        void
    ) -> QDir { return client_dir; };

    auto ParseConfig(
        void
    ) -> bool;

    //
    // Callbacks
    //

    auto Callbacks(
        void
    ) -> std::vector<std::string>;

    auto AddCallbackObject(
        const std::string&  uuid,
        const py11::object& callback
    ) -> void;

    auto RemoveCallbackObject(
        const std::string& uuid
    ) -> void;

    auto CallbackObject(
        const std::string& uuid
    ) -> std::optional<py11::object>;

    //
    // Listeners
    //

    auto Listeners(
        void
    ) -> std::vector<std::string>;

    auto ListenerObject(
        const std::string& name
    ) -> std::optional<json>;

    auto AddListener(
        const json& listener
    ) -> void;

    auto RemoveListener(
        const std::string& name
    ) -> void;

    //
    // Protocols
    //

    auto Protocols(
        void
    ) -> std::vector<std::string>;

    auto AddProtocol(
        const std::string&  name,
        const py11::object& listener
    ) -> void;

    auto ProtocolObject(
        const std::string& name
    ) -> std::optional<py11::object>;

    //
    // Payload Builder
    //

    auto AddBuilder(
        const std::string & name,
        const py11::object& builder
    ) -> void;

    auto BuilderObject(
        const std::string& name
    ) -> std::optional<py11::object>;

    auto Builders() -> std::vector<std::string>;

    //
    // Agent api
    //

    auto Agent(
        const std::string& uuid
    ) const -> std::optional<HcAgent*>;

    auto Agents(
        void
    ) const -> std::vector<HcAgent*>;

    auto AddAgentObject(
        const std::string&  type,
        const py11::object& object
    ) -> void;

    auto AgentObject(
        const std::string& type
    ) -> std::optional<py11::object>;

    //
    // Menu actions
    //

    auto AddAction(
        ActionObject* action
    ) -> void;

    auto Actions(
        const ActionObject::ActionType& type
    ) -> std::vector<ActionObject*>;

    //
    // Agent Initialize Python Event
    //

    auto AddInitializeEvent(
        const std::string&  type,
        const py11::object& object
    ) -> void;

    auto InitializeEvents(
        void
    ) -> std::vector<std::tuple<std::string, py11::object>>;

    //
    // Server Api
    //

    auto ApiLogin(
        const json& data
    ) -> std::tuple<
        std::optional<std::string>,
        std::optional<std::string>
    >;

    auto ApiSend(
        const std::string& endpoint,
        const json&        body,
        const bool         keep_alive = false
    ) -> std::tuple<int, std::string>;

    auto ServerPullPlugins(
        void
    ) -> void;

    auto ServerPullResource(
        const std::string& name,
        const std::string& version,
        const std::string& resource
    ) -> bool;

    auto RequestSend(
        const std::string&   url,
        const std::string&   data,
        const bool           keep_alive   = false,
        const std::string&   method       = "POST",
        const std::string&   content_type = "application/json",
        const bool           havoc_token  = true,
        const json::array_t& headers      = {}
    ) -> std::tuple<
        int,
        std::optional<std::string>,
        std::string
    >;

    //
    // Scripts Manager
    //

    auto ScriptLoad(
        const std::string& path
    ) const -> void;

    auto ScriptConfigProcess(
        void
    ) -> void;

    //
    // Profile manager
    //

    auto ProfileSync(
        void
    ) -> void;

    auto ProfileSave(
        void
    ) -> void;

    auto ProfileInsert(
        const std::string& type,
        const toml::value& data
    ) -> void;

    auto ProfileQuery(
        const std::string& type
    ) -> toml::array;

    auto ProfileDelete(
        const std::string& type,
        const std::int32_t entry
    ) -> void;

    auto ProfileDelete(
        const std::string& type
    ) -> void;

Q_SIGNALS:
    auto eventHandle(
        const QByteArray& request
    ) -> void;

    auto eventDispatch(
        const json& event
    ) -> void;

    auto eventClosed() -> void;
};

/* a global Havoc app instance */
extern HcApplication* Havoc;

#endif //HAVOCCLIENT_HAVOC_H

#include <Havoc.h>
#include <vector>

HcApplication::HcApplication() {
    /* initialize logger */
    spdlog::set_pattern( "[%T %^%l%$] %v" );
    spdlog::info( "Havoc Framework [{} :: {}]", HAVOC_VERSION, HAVOC_CODENAME );

#ifdef Q_OS_MAC
    //
    // original fix and credit: https://github.com/HavocFramework/Havoc/pull/466/commits/8b75de9b4632a266badf64e1cc22e57cc55a5b7c
    //
    QApplication::setStyle( "Fusion" );
#endif

    QTextCodec::setCodecForLocale( QTextCodec::codecForName( "UTF-8" ) );
}

HcApplication::~HcApplication() = default;

/*!
 * @brief
 *  this entrypoint executes the connector dialog
 *  and tries to connect and login to the teamserver
 *
 *  after connecting it is going to start an event thread
 *  and starting the Havoc MainWindow.
 */
auto HcApplication::Main(
    int    argc,
    char** argv
) -> void {
    auto response   = json();
    auto data       = json();
    auto error      = std::string( "Failed to send login request: " );
    auto stylesheet = StyleSheet();
    auto found      = false;
    auto conn_entry = toml::value();
    auto message    = QMessageBox();
    auto primary    = QGuiApplication::primaryScreen();

    if ( argc >= 2 ) {
        //
        // check if we have specified the --debug/-d
        // flag to enable debug messages and logs
        //
        if ( ( strcmp( argv[ 1 ], "--debug" ) == 0 ) || ( strcmp( argv[ 1 ], "-d" ) == 0 ) ) {
            spdlog::set_level( spdlog::level::debug );
        }
    }

    //
    // setup directory and load up all profile connections for
    // the connection dialog to connect to already saved connections
    //

    if ( ! SetupDirectory() ) {
        spdlog::error( "failed to setup configuration directory. aborting" );
        return;
    }

    ProfileSync();

    if ( !ParseConfig() ) {
        spdlog::error( "failed to parse configuration. aborting" );
        return;
    };

    //
    // display the operator with the connection
    // dialog and saved profile connections and
    // save the connection details in the profile
    const auto connector = new HcConnectDialog();

    if ( ( data = connector->start() ).empty() || ( !connector->connected() ) ) {
        delete connector;
        Exit();
        return;
    }

    delete connector;

    //
    // login into the team server api
    //

    auto [token, ssl_hash] = ApiLogin( data );
    if ( ( !token.has_value() ) || ( ! ssl_hash.has_value() ) ) {
        Helper::MessageBox(
            QMessageBox::Critical,
            "Login failure",
            "Failed to login"
        );

        return Exit();
    }

    //
    // save the successful credentials into
    // the profile on disk with the ssl hash
    //

    session = {
        .name     = data[ "name" ].get<std::string>(),
        .host     = data[ "host" ].get<std::string>(),
        .port     = data[ "port" ].get<std::string>(),
        .user     = data[ "username" ].get<std::string>(),
        .pass     = data[ "password" ].get<std::string>(),
        .token    = token.value(),
        .ssl_hash = ssl_hash.value(),
    };

    for ( const auto& connection : ProfileQuery( "connection" ) ) {
        if ( toml::find<std::string>( connection, "name" ) == session.name ) {
            found      = true;
            conn_entry = connection;
            break;
        }
    }

    message.setStyleSheet( StyleSheet() );

    if ( !found ) {
        if ( ! ssl_hash.value().empty() ) {
            message.setWindowTitle( "Verify SSL Fingerprint" );
            message.setText( std::format(
                "The team server's SSL fingerprint is: \n\n{}\n\nDoes this match the fingerprint presented in the server console?",
                ssl_hash.value()
            ).c_str() );
            message.setIcon( QMessageBox::Warning );
            message.setStandardButtons( QMessageBox::Yes | QMessageBox::No );
            message.setDefaultButton( QMessageBox::Yes );

            if ( message.exec() == QMessageBox::No ) {
                return;
            }
        }

        ProfileInsert( "connection", toml::table {
           { "name",     session.name     },
           { "host",     session.host     },
           { "port",     session.port     },
           { "username", session.user     },
           { "password", session.pass     },
           { "ssl-hash", session.ssl_hash },
        } );
    } else {
        //
        // if we have found an entry then lets compare
        // the ssl hash to the profile to verify that
        // it's still the same as previously
        if ( conn_entry.contains( "ssl-hash" ) ) {
            if ( toml::find<std::string>( conn_entry, "ssl-hash" ) != ssl_hash.value() ) {
                message.setIcon( QMessageBox::Critical );
                message.setWindowTitle( "SSL Fingerprint" );
                message.setText( std::format(
                    "SSL fingerprint verification failure \n\n{}\n\nwas expected from previous profile connection but received\n\n{}",
                    toml::find<std::string>( conn_entry, "ssl-hash" ),
                    ssl_hash.value()
                ).c_str() );
                message.setStandardButtons( QMessageBox::Ok );
                message.exec();

                return;
            }
        } else {
            //
            // the profile connection seems incorrect or invalid... abort
            message.setIcon( QMessageBox::Critical );
            message.setWindowTitle( "SSL Fingerprint" );
            message.setText(
                "SSL fingerprint verification failure \n\ncouldn't resolve and validate ssl fingerprint hash from previous connection"
            );
            message.setStandardButtons( QMessageBox::Ok );
            message.exec();

            return;
        }
    }

    //
    // continue with the splash
    // screen and starting up the Ui
    //

    splash = new QSplashScreen( primary, QPixmap( ":/images/splash-screen" ) );
    splash->show();

    Theme.setStyleSheet( stylesheet );

    //
    // create main window
    //
    ui = new HcMainWindow;
    ui->setStyleSheet( stylesheet );

    //
    // initialize the plugin manager
    //
    plugin_manager = new HcPluginManager;

    //
    // setup Python thread
    //
    PyEngine = new HcPyEngine();

    //
    // set up the event thread and connect to the
    // server and dispatch all the incoming events
    //
    SetupThreads();

    //
    // pull server plugins and resources and process
    // locally registered scripts via the configuration
    //
    ServerPullPlugins();
}

auto HcApplication::Exit() -> void {
    exit( 0 );
}

auto HcApplication::ApiLogin(
    const json& data
) -> std::tuple<
    std::optional<std::string>,
    std::optional<std::string>
> {
    auto error     = std::string();
    auto object    = json();
    auto api_login = std::format( "https://{}:{}/api/login", data[ "host" ].get<std::string>(), data[ "port" ].get<std::string>() );

    //
    // send request with the specified login data
    // and also get the remote server ssl hash
    //
    auto [status_code, response, ssl_hash] = RequestSend(
        api_login,
        data.dump(),
        false,
        "POST",
        "application/json"
    );

    if ( status_code == 401 ) {
        //
        // 401 Unauthorized: failed to log in
        //

        if ( ! response.has_value() ) {
            spdlog::error( "failed to login: unauthorized (invalid credentials)" );
            return {};
        }

        try {
            if ( ( object = json::parse( response.value() ) ).is_discarded() ) {
                spdlog::error( "failed to parse login json response: json has been discarded" );
                return {};
            }
        } catch ( std::exception& e ) {
            spdlog::error( "failed to parse login json response: \n{}", e.what() );
            return {};
        }

        if ( ! object.contains( "error" ) ) {
            return {};
        }

        if ( ! object[ "error" ].is_string() ) {
            return {};
        }

        spdlog::error( "failed to login: {}", object[ "error" ].get<std::string>() );

        return {};
    }

    if ( status_code != 200 ) {
        spdlog::error( "unexpected response: http status code {}", status_code );
        return {};
    }

    spdlog::info( "ssl hash: {}", ssl_hash );

    if ( response.value().empty() ) {
        return {};
    }

    //
    // parse the token from the request
    //

    try {
        if ( ( object = json::parse( response.value() ) ).is_discarded() ) {
            spdlog::error( "failed to parse login json response: json has been discarded" );
            return {};
        }
    } catch ( std::exception& e ) {
        spdlog::error( "failed to parse login json response: \n{}", e.what() );
        return {};
    }

    if ( ! object.contains( "token" ) || ! object[ "token" ].is_string() ) {
        return {};
    }

    return {
        object[ "token" ].get<std::string>(),
        ssl_hash
    };
}

auto HcApplication::ApiSend(
    const std::string& endpoint,
    const json&        body,
    const bool         keep_alive
) -> std::tuple<int, std::string> {
    auto [status, response, ssl_hash] = RequestSend(
        std::format( "https://{}:{}/{}", Havoc->session.host, Havoc->session.port, endpoint ),
        body.dump(),
        keep_alive,
        "POST",
        "application/json",
        true
    );

    //
    // validate that the ssl hash against the
    // profile that has been marked as safe
    //
    if ( !ssl_hash.empty() && ssl_hash != Havoc->session.ssl_hash ) {
        //
        // TODO: use QMetaObject::invokeMethod to call MessageBox
        //       to alert that the ssl hash has been changed
        //
        spdlog::error( "failed to send request: invalid ssl fingerprint detected ({})", ssl_hash );
        return {};
    }

    return { status, response.value() };
}

auto HcApplication::ServerPullPlugins(
    void
) -> void {
    auto plugins   = json();
    auto uuid      = std::string();
    auto name      = std::string();
    auto version   = std::string();
    auto resources = std::vector<std::string>();
    auto message   = std::string();
    auto details   = std::string();
    auto box       = QMessageBox();

    splash->showMessage( "pulling plugins information", Qt::AlignLeft | Qt::AlignBottom, Qt::white );

    auto [status_code, response] = ApiSend( "api/plugin/list", {} );
    if ( status_code != 200 ) {
        Helper::MessageBox(
            QMessageBox::Critical,
            "plugin processing failure",
            "failed to pull all registered plugins"
        );
        goto END;
    }

    try {
        if ( ( plugins = json::parse( response ) ).is_discarded() ) {
            spdlog::debug( "plugin processing json response has been discarded" );
            splash->close();
            goto END;
        }
    } catch ( std::exception& e ) {
        spdlog::error( "failed to parse plugin processing json response: \n{}", e.what() );
        goto END;
    }

    if ( plugins.empty() ) {
        spdlog::debug( "no plugins to process" );
        goto END;
    }

    if ( !plugins.is_array() ) {
        spdlog::error( "plugins response is not an array" );
        goto END;
    }

    //
    // iterate over all available agents
    // and pull console logs as well
    //
    for ( auto& plugin : plugins ) {
        if ( ! plugin.is_object() ) {
            spdlog::debug( "warning! plugin processing item is not an object" );
            continue;
        }

        spdlog::debug( "processing plugin: {}", plugin.dump() );

        //
        // we just really assume that they
        // are contained inside the json object
        //
        name      = plugin[ "name"      ].get<std::string>();
        version   = plugin[ "version"   ].get<std::string>();
        resources = plugin[ "resources" ].get<std::vector<std::string>>();

        //
        // check if the plugin directory exists, if not then
        // pull the plugin resources from the remote server
        //
        if ( QDir( std::format( "{}/plugins/{}@{}", directory().path().toStdString(), name, version ).c_str() ).exists() ) {
            //
            // if the directory exists then it means we have pulled the
            // resources already so we are going to skip to the next plugin
            //
            continue;
        }

        Havoc->splash->showMessage( std::format( "processing plugin {} ({})", name, version ).c_str(), Qt::AlignLeft | Qt::AlignBottom, Qt::white );

        if ( ( ! plugin.contains( "resources" ) ) && ( ! plugin[ "resources" ].is_array() ) ) {
            continue;
        }

        //
        // we will display the user with a prompt to get confirmation
        // if they even want to install the plugin from the remote server
        //

        message = std::format( "A plugin will be installed from the remote server: {} ({})\n", name, version );
        details = "resources to be downloaded locally:\n";

        for ( const auto& res : resources ) {
            details += std::format( " - {}\n", res );
        }

        box.setStyleSheet( StyleSheet() );
        box.setWindowTitle( "Plugin install" );
        box.setText( message.c_str() );
        box.setIcon( QMessageBox::Warning );
        box.setDetailedText( details.c_str() );
        box.setStandardButtons( QMessageBox::Ok | QMessageBox::Cancel );
        box.setDefaultButton( QMessageBox::Ok );

        if ( box.exec() == QMessageBox::Cancel ) {
            continue;
        }

        spdlog::debug( "{} ({}):", name, version );
        for ( const auto& res : plugin[ "resources" ].get<std::vector<std::string>>() ) {
            spdlog::debug( " - {}", res );

            splash->showMessage(
                std::format( "pulling plugin {} ({}): {}", name, version, res ).c_str(),
                Qt::AlignLeft | Qt::AlignBottom,
                Qt::white
            );

            if ( ! ServerPullResource( name, version, res ) ) {
                spdlog::debug( "failed to pull resources for plugin: {}", name );
                break;
            }
        }
    }

END:
    splash->showMessage( "process plugins and store", Qt::AlignLeft | Qt::AlignBottom, Qt::white );

    //
    // start the plugin worker of the store to load up all the
    // plugins from the local file system that we just pulled
    //
    ui->PageScripts->processPlugins();

    splash->showMessage( "process scripts", Qt::AlignLeft | Qt::AlignBottom, Qt::white );

    //
    // load all scripts from the configuration
    //
    ScriptConfigProcess();

    //
    // now start up the meta worker to
    // pull the listeners and agent sessions
    //
    MetaWorker.Thread->start();
}

auto HcApplication::ServerPullResource(
    const std::string& name,
    const std::string& version,
    const std::string& resource
) -> bool {
    auto dir_resource = QDir();
    auto dir_plugin   = QDir();
    auto file_info    = QFileInfo();

    if ( ! ( dir_plugin = QDir( std::format( "{}/plugins/{}@{}", directory().path().toStdString(), name, version ).c_str() ) ).exists() ) {
        if ( ! dir_plugin.mkpath( "." ) ) {
            spdlog::error( "failed to create plugin directory {}", dir_plugin.path().toStdString() );
            return false;
        }
    }

    file_info    = QFileInfo( ( dir_plugin.path().toStdString() + "/" + resource ).c_str() );
    dir_resource = file_info.absoluteDir();

    if ( ! dir_resource.exists() ) {
        if ( ! dir_resource.mkpath( dir_resource.absolutePath() ) ) {
            spdlog::error( "failed to create resource path: {}", dir_resource.absolutePath().toStdString() );
            return false;
        }
    }

    auto fil_resource = QFile( file_info.absoluteFilePath() );
    if ( ! fil_resource.exists() ) {
        auto [status_code, response] = Havoc->ApiSend( "api/plugin/resource", {
            { "name",     name     },
            { "resource", resource },
        } );

        if ( status_code != 200 ) {
            spdlog::debug("failed to pull plugin resource {} from {}: {}", name, resource, response );
            return false;
        }

        if ( fil_resource.open( QIODevice::WriteOnly ) ) {
            fil_resource.write( QByteArray::fromStdString( response ) );
        } else {
            spdlog::debug( "failed to open file resource locally {}", file_info.absoluteFilePath().toStdString() );
        }
    } else {
        spdlog::debug( "file resource already exists: {}", file_info.absoluteFilePath().toStdString() );
    }

    return true;
}

auto HcApplication::RequestSend(
    const std::string&   url,
    const std::string&   data,
    const bool           keep_alive,
    const std::string&   method,
    const std::string&   content_type,
    const bool           havoc_token,
    const json::array_t& headers
) -> std::tuple<int, std::optional<std::string>, std::string> {
    auto request     = QNetworkRequest( QString::fromStdString( url ) );
    auto reply       = static_cast<QNetworkReply*>( nullptr );
    auto event       = QEventLoop();
    auto response    = std::string();
    auto status_code = 0;
    auto error       = QNetworkReply::NoError;
    auto error_str   = std::string();
    auto ssl_chain   = QList<QSslCertificate>();
    auto ssl_hash    = std::string();
    auto ssl_config  = QSslConfiguration::defaultConfiguration();
    auto _network    = QNetworkAccessManager();

    //
    // prepare request headers
    //

    if ( ! content_type.empty() ) {
        request.setHeader( QNetworkRequest::ContentTypeHeader, QString::fromStdString( content_type ) );
    }

    if ( havoc_token && ( ! Havoc->session.token.empty() ) ) {
        request.setRawHeader( "x-havoc-token", Havoc->session.token.c_str() );
    }

    for ( const auto& item : headers ) {
        for ( auto it = item.begin(); it != item.end(); ++it ) {
            std::cout << it.key() << ": " << it.value() << std::endl;
            request.setRawHeader( it.key().c_str(), it.value().get<std::string>().c_str() );
        }
    }

    if ( keep_alive ) {
        request.setRawHeader( "Connection", "Keep-Alive" );
    }

    //
    // we are going to adjust the ssl configuration
    // to disable certification verification
    //
    ssl_config.setPeerVerifyMode( QSslSocket::VerifyNone );
    request.setSslConfiguration( ssl_config );

    //
    // send request to the endpoint url via specified method
    //
    if ( method == "POST" ) {
        reply = _network.post( request, QByteArray::fromStdString( data ) );
    } else if ( method == "GET" ) {
        reply = _network.get( request );
    } else {
        spdlog::error( "[RequestSend] invalid http method: {}", method );
        return {};
    }

    //
    // we are going to wait for the request to finish, during the
    // waiting time we are going to continue executing the event loop
    //
    connect( reply, &QNetworkReply::finished, &event, &QEventLoop::quit );
    event.exec();

    //
    // parse the request data such as
    // error, response, and status code
    //
    error       = reply->error();
    error_str   = std::string( reply->errorString().toStdString().c_str() );
    response    = reply->readAll().toStdString();
    status_code = reply->attribute( QNetworkRequest::HttpStatusCodeAttribute ).toInt();

    //
    // get the SSL certificate SHA-256 hash
    //
    if ( error == QNetworkReply::NoError ) {
        if ( ! ( ssl_chain = reply->sslConfiguration().peerCertificateChain() ).isEmpty() ) {
            ssl_hash = ssl_chain.first().digest( QCryptographicHash::Sha256 ).toHex().toStdString();
        }
    } else {
        spdlog::debug( "QNetworkReply error: {}", error_str );
    }

    reply->deleteLater();

    //
    // now handle and retrieve the response from the request
    //
    if ( error != QNetworkReply::NoError ) {
        //
        // TODO: perhaps raise an exception...
        //
        spdlog::error( "[RequestSend] invalid network reply: {}", response );
        return {};
    }

    return std::make_tuple( status_code, response, ssl_hash );
}

auto HcApplication::eventClosed() -> void {
    spdlog::error( "websocket closed" );
    Exit();
}

auto HcApplication::Server() const -> std::string { return session.host + ":" + session.port; }
auto HcApplication::Token()  const -> std::string { return session.token; }

auto HcApplication::eventHandle(
    const QByteArray& request
) -> void {
    auto event = json::parse( request.toStdString() );

    /* check if we managed to parse the json event
     * if yes then dispatch it but if not then dismiss it */
    if ( ! event.is_discarded() ) {
        eventDispatch( event );
    } else {
        spdlog::error( "failed to parse event" );
        /* what now ?
         * I guess ignore since its not valid event
         * or debug print it I guess */
    }
}

auto HcApplication::StyleSheet(
    void
) -> QByteArray {
    if ( QFile::exists( "theme.css" ) ) {
        return Helper::FileRead( "theme.css" );
    }

    return Helper::FileRead( ":/style/default" );
}

auto HcApplication::AddProtocol(
    const std::string&  name,
    const py11::object& listener
) -> void {
    protocols.push_back( NamedObject{
        .name   = name,
        .object = listener
    } );
}

auto HcApplication::ProtocolObject(
    const std::string& name
) -> std::optional<py11::object> {
    for ( auto& listener : protocols ) {
        if ( listener.name == name ) {
            return listener.object;
        }
    }

    return std::nullopt;
}

auto HcApplication::Protocols(
    void
) -> std::vector<std::string> {
    auto names = std::vector<std::string>();

    for ( auto& listener : protocols ) {
        names.push_back( listener.name );
    }

    return names;
}

auto HcApplication::SetupThreads(
    void
) -> void {
    //
    // HcEventWorker thread
    //
    // now set up the event thread and dispatcher
    //
    {
        connect( Events.Thread, &QThread::started, Events.Worker, &HcEventWorker::run );
        connect( Events.Worker, &HcEventWorker::availableEvent, this, &HcApplication::eventHandle );
        connect( Events.Worker, &HcEventWorker::socketClosed, this, &HcApplication::eventClosed );
    }

    //
    // HcHeartbeatWorker thread
    //
    // start the heartbeat worker thread
    //
    {
        connect( Heartbeat.Thread, &QThread::started, Heartbeat.Worker, &HcHeartbeatWorker::run );

        //
        // fire up the even thread that is going to
        // process heart beat events
        //
        Heartbeat.Thread->start();
    }

    //
    // HcMetaWorker thread
    //
    // start the meta worker to pull listeners and agent sessions thread
    //
    {
        connect( MetaWorker.Thread, &QThread::started, MetaWorker.Worker, &HcMetaWorker::run );
        connect( MetaWorker.Worker, &HcMetaWorker::Finished, this, [&](){
            //
            // only start the event worker once the meta worker finished
            // pulling all the listeners, agents, etc. from the server
            //
            spdlog::info( "MetaWorker finished" );
            splash->close();

            ui->move(
                QGuiApplication::primaryScreen()->geometry().center() - ui->rect().center()
            );
            ui->renderWindow();

            Events.Thread->start();
        } );

        //
        // connect methods to add listeners, agents, etc. to the user interface (ui)
        //
        connect( MetaWorker.Worker, &HcMetaWorker::AddListener,     ui, &HcMainWindow::AddListener  );
        connect( MetaWorker.Worker, &HcMetaWorker::AddAgent,        ui, &HcMainWindow::AddAgent     );
        connect( MetaWorker.Worker, &HcMetaWorker::AddAgentConsole, ui, &HcMainWindow::AgentConsole );
    }
}

auto HcApplication::AddBuilder(
    const std::string & name,
    const py11::object& builder
) -> void {
    builders.push_back( NamedObject{
        .name   = name,
        .object = builder
    } );
}

auto HcApplication::BuilderObject(
    const std::string& name
) -> std::optional<py11::object> {

    for ( auto& builder : builders ) {
        if ( builder.name == name ) {
            return builder.object;
        }
    }

    return std::nullopt;
}

auto HcApplication::Builders() -> std::vector<std::string>
{
    auto names = std::vector<std::string>();

    for ( auto& builder : builders ) {
        names.push_back( builder.name );
    }

    return names;
}

auto HcApplication::AddListener(
    const json& listener
) -> void {
    spdlog::debug( "listener -> {}", listener.dump() );
    listeners.push_back( listener );

    ui->PageListener->addListener( listener );
}

auto HcApplication::RemoveListener(
    const std::string& name
) -> void {
    spdlog::debug( "removing listener: {}", name );

    const auto object = ListenerObject( name );

    //
    // remove listener from listener list
    //
    for ( auto iter = listeners.begin(); iter != listeners.end(); ++iter ) {
        if ( object.has_value() ) {
            const auto& data = object.value();
            if ( data.contains( "name" ) && data[ "name" ].is_string() ) {
                if ( data[ "name" ].get<std::string>() == name ) {
                    listeners.erase( iter );
                    break;
                }
            }
        }
    }

    ui->PageListener->removeListener( name );
}

auto HcApplication::ListenerObject(
    const std::string &name
) -> std::optional<json> {

    for ( auto& data : listeners ) {
        if ( data.contains( "name" ) && data[ "name" ].is_string() ) {
            if ( data[ "name" ].get<std::string>() == name ) {
                return data;
            }
        }
    }

    return std::nullopt;
}

auto HcApplication::Listeners() -> std::vector<std::string>
{
    auto names = std::vector<std::string>();

    for ( auto& data : listeners ) {
        if ( data.contains( "name" ) && data[ "name" ].is_string() ) {
            names.push_back( data[ "name" ].get<std::string>() );
        }
    }

    return names;
}

auto HcApplication::Agents() const -> std::vector<HcAgent*>
{
    return ui->PageAgent->agents;
}

auto HcApplication::Agent(
    const std::string& uuid
) const -> std::optional<HcAgent*> {
    return ui->PageAgent->Agent( uuid );
}

auto HcApplication::AddAgentObject(
    const std::string&  type,
    const py11::object& object
) -> void {
    agents.push_back( NamedObject {
        .name   = type,
        .object = object
    } );
}

auto HcApplication::AgentObject(
    const std::string& type
) -> std::optional<py11::object> {
    for ( auto [name, object] : agents ) {
        if ( name == type ) {
            return object;
        }
    }

    return std::nullopt;
}

auto HcApplication::ParseConfig(
    void
) -> bool {
    auto config_path = QFile();
    auto font_tbl    = toml::table();
    auto font_family = std::string();
    auto font_size   = 0lu;

    config_path.setFileName( client_dir.path() + "/config.toml" );
    config_path.open( QIODevice::ReadWrite );

    try {
        config = toml::parse( config_path.fileName().toStdString() );
    } catch ( std::exception& e ) {
        spdlog::error( "failed to parse toml configuration: {}", e.what() );
        return false;
    }

    if ( config.contains( "font" ) ) {
        font_tbl = config.at( "font" ).as_table();

        if ( font_tbl.contains( "family" ) ) {
            font_family = font_tbl.at( "family" ).as_string();
        } else {
            spdlog::error( "failed to parse font: font family not found" );
            return false;
        }

        if ( font_tbl.contains( "size" ) ) {
            font_size = font_tbl.at( "size" ).as_integer();
        } else {
            spdlog::error( "failed to parse font: font size not found" );
            return false;
        }

        QApplication::setFont( QFont( QString::fromStdString( font_family ), font_size ) );
    }

    return true;
}

auto HcApplication::Callbacks() -> std::vector<std::string>
{
    auto names = std::vector<std::string>();

    for ( auto& callback : callbacks ) {
        names.push_back( callback.name );
    }

    return names;
}

auto HcApplication::AddCallbackObject(
    const std::string&  uuid,
    const py11::object& callback
) -> void {
    callbacks.push_back( NamedObject{
        .name   = uuid,
        .object = callback
    } );
}

auto HcApplication::RemoveCallbackObject(
    const std::string& callback
) -> void {
    //
    // iterate over the callbacks
    //
    for ( auto iter = callbacks.begin(); iter != callbacks.end(); ++iter ) {
        if ( iter->name == callback ) {
            callbacks.erase( iter );
            break;
        }
    }
}

auto HcApplication::CallbackObject(
    const std::string& uuid
) -> std::optional<py11::object> {
    //
    // iterate over registered callbacks
    //
    for ( auto [name, object] : callbacks ) {
        if ( name == uuid ) {
            return object;
        }
    }

    return std::nullopt;
}

auto HcApplication::SetupDirectory(
    void
) -> bool {
    auto havoc_dir = QDir( QDir::homePath() + "/.havoc" );

    if ( ! havoc_dir.exists() ) {
        if ( ! havoc_dir.mkpath( "." ) ) {
            return false;
        }
    }

    client_dir.setPath( havoc_dir.path() + "/client" );
    if ( ! client_dir.exists() ) {
        if ( ! client_dir.mkpath( "." ) ) {
            return false;
        }
    }

    return true;
}

auto HcApplication::AddAction(
    ActionObject* action
) -> void {
    if ( ! action ) {
        spdlog::debug( "[AddAction] action is a nullptr" );
        return;
    }

    for ( auto _action : Actions( action->type ) ) {
        if ( _action->name == action->name ) {
            spdlog::debug( "[AddAction] action with the name \"{}\" and type already exists", action->name );
            return;
        }
    }

    actions.push_back( action );
}

auto HcApplication::Actions(
    const ActionObject::ActionType& type
) -> std::vector<ActionObject*> {
    auto actions_type = std::vector<ActionObject*>();

    for ( auto action : actions ) {
        if ( action->type == type ) {
            actions_type.push_back( action );
        }
    }

    return actions_type;
}

auto HcApplication::AddInitializeEvent(
    const std::string&  type,
    const py11::object& object
) -> void {
    init_events.push_back( NamedObject{
        .name   = type,
        .object = object
    } );
}

auto HcApplication::InitializeEvents(
    void
) -> std::vector<std::tuple<
    std::string,
    py11::object
>> {
    auto list = std::vector<std::tuple<std::string, py11::object>>();

    for ( const auto& [name, object] : init_events ) {
        list.push_back( std::make_tuple( name, object ) );
    }

    return list;
}

auto HcApplication::ScriptLoad(
    const std::string& path
) const -> void {
    ui->PageScripts->LoadScript( path );
}

auto HcApplication::ScriptConfigProcess(
    void
) -> void {
    //
    // load the registered scripts
    if ( config.contains( "scripts" ) && config.at( "scripts" ).is_table() ) {
        auto scripts_tbl = config.at( "scripts" ).as_table();

        if ( scripts_tbl.contains( "files" ) && scripts_tbl.at( "files" ).is_array() ) {
            for ( const auto& file : scripts_tbl.at( "files" ).as_array() ) {
                if ( ! file.is_string() ) {
                    spdlog::error( "configuration scripts file value is not an string" );
                    continue;
                }

                ScriptLoad( file.as_string() );
            }
        }
    }

    //
    // load scripts from the profile now that
    // have been previously loaded via the UI
    for ( const auto& script : ProfileQuery( "script" ) ) {
        if ( !script.contains( "path" ) ) {
            continue;
        }

        ScriptLoad( script.at( "path" ).as_string() );
    }
}

auto HcApplication::ProfileSync(
    void
) -> void {
    auto profile_path = QFile( directory().path() + "/.profile.toml" );

    if ( ! profile_path.exists() ) {
        profile_path.open( QIODevice::ReadWrite );
        profile_path.close();

        //
        // profile file does not exist yet so we have
        // nothing to load and sync to the client
        //
        return;
    }

    try {
        profile = toml::parse( profile_path.fileName().toStdString() );
    } catch ( std::exception& e ) {
        spdlog::error( "failed to parse toml profile: {}", e.what() );
    }
}

auto HcApplication::ProfileSave(
    void
) -> void {
    auto profile_path = QFile( directory().path() + "/.profile.toml" );

    if ( ! profile_path.open( QIODevice::WriteOnly | QIODevice::Truncate ) ) {
        spdlog::error( "failed to open profile file: {}", profile_path.errorString().toStdString() );
        return;
    }

    profile_path.write( QByteArray::fromStdString( toml::format( profile ) ) );
    profile_path.close();
}

auto HcApplication::ProfileInsert(
    const std::string& type,
    const toml::value& data
) -> void {
    auto type_tbl = toml::array();

    ProfileSync();

    if ( profile.contains( type ) ) {
        type_tbl = toml::find<toml::array>( profile, type );
    }

    type_tbl.push_back( data );

    profile[ type ] = type_tbl;

    ProfileSave();
}

auto HcApplication::ProfileQuery(
    const std::string& type
) -> toml::array {
    ProfileSync();

    if ( profile.contains( type ) ) {
        return toml::find<toml::array>( profile, type );
    }

    return toml::array();
}

auto HcApplication::ProfileDelete(
    const std::string& type,
    const std::int32_t entry
) -> void {
    auto array = toml::array();

    ProfileSync();

    if ( !profile.contains( type ) ) {
        return;
    }

    array = toml::find<toml::array>( profile, type );
    if ( array.size() > entry ) {
        array.erase( array.begin() + entry );
    }

    if ( array.empty() ) {
        profile.as_table().erase( type );
    } else {
        profile[ type ] = array;
    }

    ProfileSave();
}

auto HcApplication::ProfileDelete(
    const std::string& type
) -> void {
    ProfileSync();

    if ( !profile.contains( type ) ) {
        return;
    }

    profile.as_table().erase( type );

    ProfileSave();
}

HcApplication::ActionObject::ActionObject() {}

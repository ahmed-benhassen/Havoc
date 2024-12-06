#include <Havoc.h>
#include <api/HcAgent.h>

#include <IHcPlugin.h>

/*!
 * @brief
 *  register an agent interface to the havoc client
 *
 * @param type
 *  type of the agent (name)
 *
 * @param object
 *  object to register
 */
auto HcAgentRegisterInterface(
    const std::string&  type,
    const py11::object& object
) -> void {
    Havoc->AddAgentObject( type, object );
}

/*!
 * @brief
 *  writes content to specified agent console
 *  bottom label
 *
 * @param uuid
 *  uuid of the agent
 *
 * @param content
 *  content to set as the label of the console
 */
auto HcAgentConsoleLabel(
    const std::string& uuid,
    const std::string& content
) -> void {
    auto agent = Havoc->Agent( uuid );

    if ( agent.has_value() ) {
        //
        // if we are going to append nothing then hide the label
        //
        agent.value()->console()->setBottomLabel( content.c_str() );
        agent.value()->console()->LabelBottom->setFixedHeight( content.empty() ? 0 : 20 );
    }
}

/*!
 * @brief
 *  writes content to specified agent console header
 *
 * @param uuid
 *  uuid of the agent
 *
 * @param content
 *  content to set as the header of the console
 */
auto HcAgentConsoleHeader(
    const std::string& uuid,
    const std::string& content
) -> void {
    auto agent = Havoc->Agent( uuid );

    if ( agent.has_value() ) {
        //
        // if we are going to append nothing then hide the label
        //
        agent.value()->console()->setHeaderLabel( content.c_str() );
        agent.value()->console()->LabelHeader->setFixedHeight( content.empty() ? 0 : 20 );
    }
}

/*!
 * @brief
 *  writes content to specified agent console
 *
 * @param uuid
 *  uuid of the agent
 *
 * @param content
 *  content to write to the console
 */
auto HcAgentConsoleWrite(
    const std::string& uuid,
    const std::string& content
) -> void {
    auto agent = Havoc->Agent( uuid );

    if ( agent.has_value() ) {
        emit agent.value()->ui.signal.ConsoleWrite( uuid.c_str(), content.c_str() );
    }
}

/*!
 * @brief
 *  add command completion to the agent
 *
 * @param agent
 *  uuid of agent
 *
 * @param command
 *  command to add to the auto-completion
 *
 * @param description
 *  description of command
 */
auto HcAgentConsoleAddComplete(
    const std::string& agent,
    const std::string& command,
    const std::string& description
) -> void {
    if ( ! HcAgentCompletionList->contains( agent ) ) {
        HcAgentCompletionList->insert( { agent, std::vector<std::pair<std::string, std::string>>() } );
    }

    HcAgentCompletionList->at( agent ).push_back( std::pair( command, description ) );

    for ( const auto _agent : Havoc->Agents() ) {
        if ( _agent->type() == agent ) {
            //
            // we only have to add it once to the agent type
            //
            _agent->console()->addCompleteCommand( command, description );
        }
    }
}

/*!
 * @brief
 *  register a callback
 *
 * @param uuid
 *  uuid of the callback to be registered
 *
 * @param callback
 *  callback function to call
 */
auto HcAgentRegisterCallback(
    const std::string&  uuid,
    const py11::object& callback
) -> void {
    Havoc->AddCallbackObject( uuid, callback );
}

/*!
 * @brief
 *  unregister a callback
 *
 * @param uuid
 *  callback uuid to remove and unregister
 */
auto HcAgentUnRegisterCallback(
    const std::string& uuid
) -> void {
    Havoc->RemoveCallbackObject( uuid );
}

/*!
 * @brief
 *  send agent command to the server implant plugin handler
 *
 * @param uuid
 *  uuid of the agent
 *
 * @param data
 *  data to send to the handler
 *
 * @param wait
 *  wait for a response
 *
 * @return
 *  response from the server implant handler
 */
auto HcAgentExecute(
    const std::string& uuid,
    const json&        data,
    const bool         wait
) -> json {
    auto future  = QFuture<json>();
    auto request = json();
    auto object  = json();
    auto error   = std::string();

    //
    // build request that is going to be
    // sent to the server implant handler
    //
    request = {
        { "uuid", uuid },
        { "wait", wait },
        { "data", data }
    };

    //
    // send api request
    //
    auto [status, response] = Havoc->ApiSend( "/api/agent/execute", request, true );

    //
    // check for valid status response
    //
    if ( status != 200 ) {
        spdlog::debug( "failed to send request: status code {}", status );

        //
        // check for emtpy request
        //
        if ( ! response.empty() ) {
            try {
                if ( ( object = json::parse( response ) ).is_discarded() ) {
                    return { "error", "json object response has been discarded" };
                };
            } catch ( std::exception& e ) {
                spdlog::error( "failed to parse json object: \n{}", e.what() );
                return { "error", e.what() };
            };

            if ( object[ "error" ].is_string() ) {
                return json {
                    { "error", object[ "error" ].get<std::string>() }
                };
            };
        };

        return json {
            { "error", "failed to send request" }
        };
    };

    //
    // check for emtpy request
    //
    if ( ! response.empty() ) {
        try {
            if ( ( object = json::parse( response ) ).is_discarded() ) {
                spdlog::error( "failed to parse json object: json has been discarded" );
                object[ "error" ] = "failed to parse response";
            };
        } catch ( std::exception& e ) {
            spdlog::error( "exception raised while parsing json object: {}", e.what() );
            object[ "error" ] = "failed to parse response";
        };
    };

    return object;
}

/*!
 * @brief
 *  register agent context menu action
 *
 * @param agent_type
 *  agent type to add action to
 *
 * @param name
 *  name of the action to register
 *
 * @param icon_path
 *  icon path to add to action
 *
 * @param callback
 *  python callback function
 *  to call on action trigger
 */
auto HcAgentRegisterMenuAction(
    const std::string&  agent_type,
    const std::string&  name,
    const std::string&  icon_path,
    const py11::object& callback
) -> void {
    auto action = new HcApplication::ActionObject();

    action->type        = HcApplication::ActionObject::ActionAgent;
    action->name        = name;
    action->icon        = QIcon( QString::fromStdString( icon_path ) );
    action->callback_py = callback;
    action->agent.type  = agent_type;

    spdlog::debug( "HcAgentRegisterMenuAction( {}, {}, {} )", agent_type, name, icon_path );

    Havoc->AddAction( action );
}

/*!
 * @brief
 *  register a callback for agent initialization
 *
 * @param type
 *  agent type to register callback for
 *
 * @param callback
 *  python callback function to
 *  call on agent initialization
 */
auto HcAgentRegisterInitialize(
    const std::string&  type,
    const py11::object& callback
) -> void {
    spdlog::debug( "HcAgentRegisterInitialize( {}, ... )", type );

    Havoc->AddInitializeEvent( type, callback );
}

/*!
 * @brief
 *  register an icon for the agent session
 *
 * @param uuid
 *  agent uuid session
 *
 * @param icon
 *  icon buffer to use
 */
auto HcAgentRegisterIcon(
    const std::string& uuid,
    const std::string& icon
) -> void {
    const auto agent = Havoc->Agent( uuid );
    const auto image = QImage::fromData( QByteArray::fromStdString( icon ) );

    if ( agent.has_value() ) {
        emit agent.value()->ui.signal.RegisterIcon(
            QString::fromStdString( uuid ),
            image
        );
    }
}

/*!
 * @brief
 *  register a pre-existing icon
 *  name for the agent session
 *
 * @param uuid
 *  agent uuid session
 *
 * @param name
 *  icon name to use
 */
auto HcAgentRegisterIconName(
    const std::string& uuid,
    const std::string& name
) -> void {
    const auto agent = Havoc->Agent( uuid );

    if ( name != "linux"           &&
         name != "linux-high"      &&
         name != "macos"           &&
         name != "macos-high"      &&
         name != "unknown"         &&
         name != "unknown-high"    &&
         name != "win7-vista"      &&
         name != "win7-vista-high" &&
         name != "win10-8"         &&
         name != "win10-8-high"    &&
         name != "win11"           &&
         name != "win11-high"      &&
         name != "winxp"           &&
         name != "winxp-high"
    ) {
        spdlog::debug( "[ERROR] HcAgentRegisterIconName( {}, {} ): unknown icon name", uuid, name );
        return;
    }

    if ( agent.has_value() ) {
        emit agent.value()->ui.signal.RegisterIconName(
            QString::fromStdString( uuid ),
            QString::fromStdString( name )
        );
    }
}

/*!
 * @brief
 *  generate an agent payload binary from the specified profile
 *
 * @param agent_type
 *  agent type to generate
 *
 * @param profile
 *  profile to use for the payload
 *
 * @return
 *  generated payload binary
 */
auto HcAgentProfileBuild(
    const std::string& agent_type,
    const json&        profile
) -> py::bytes {
    auto payload = std::optional<std::string>();

    try {
        payload = HcPayloadBuild().generate( agent_type, profile );
    } catch ( std::exception& e ) {
        throw py11::value_error( e.what() );
    }

    return py::bytes( payload.value() );
}

/*!
 * @brief
 *  pop up a dialog to choose between available profiles and return
 *
 * @param agent_type
 *  agent type to list. if empty all available
 *  profiles are going to be displayed.
 *
 * @return
 *  selected payload
 */
auto HcAgentProfileSelect(
    const std::string& agent_type
) -> std::optional<json> {
    auto SelectDialog = new QDialog();
    auto GridLayout   = new QGridLayout( SelectDialog );
    auto LabelProfile = new QLabel( SelectDialog );
    auto ListProfiles = new QListWidget( SelectDialog );
    auto ButtonBox    = new QDialogButtonBox( SelectDialog );
    auto Profile      = std::optional<json>();

    SelectDialog->resize( 434, 316 );
    SelectDialog->setStyleSheet( HcApplication::StyleSheet() );
    SelectDialog->setObjectName( QString::fromUtf8( "SelectDialog" ) );

    LabelProfile->setObjectName( QString::fromUtf8("LabelProfile" ) );
    LabelProfile->setText( "<html><head/><body><p><span style=\"font-size:14pt;\">Profiles</span></p></body></html>" );

    ListProfiles->setProperty( "HcListWidget", "half-dark" );
    ListProfiles->setObjectName( QString::fromUtf8( "ListProfiles" ) );

    ButtonBox->setObjectName( QString::fromUtf8( "ButtonBox" ) );
    ButtonBox->setOrientation( Qt::Horizontal );
    ButtonBox->setStandardButtons( QDialogButtonBox::Cancel | QDialogButtonBox::Ok );

    GridLayout->setObjectName( QString::fromUtf8( "GridLayout" ) );
    GridLayout->addWidget( LabelProfile, 0, 0, 1, 1, Qt::AlignHCenter );
    GridLayout->addWidget( ListProfiles, 1, 0, 1, 1 );
    GridLayout->addWidget( ButtonBox,    2, 0, 1, 1 );

    QObject::connect( ButtonBox, &QDialogButtonBox::accepted, SelectDialog, &QDialog::accept );
    QObject::connect( ButtonBox, &QDialogButtonBox::rejected, SelectDialog, &QDialog::reject );
    QObject::connect( ListProfiles, &QListWidget::itemDoubleClicked, SelectDialog, &QDialog::accept );

    QMetaObject::connectSlotsByName( SelectDialog );

    //
    // add all profile items to the dialog
    //

    for ( const auto& entry : Havoc->ProfileQuery( "profile" ) ) {
        auto name    = std::string();
        auto type    = std::string();
        auto profile = json();

        if ( !entry.contains( "name" )    ||
             !entry.contains( "type" )    ||
             !entry.contains( "profile" )
        ) {
            spdlog::error( "failed to parse following profile: {}", toml::format( entry ) );
            continue;
        }

        name = toml::find<std::string>( entry, "name" );
        type = toml::find<std::string>( entry, "type" );

        if ( !agent_type.empty() && agent_type != type ) {
            //
            // if the agent type is the one from the
            // current profile entry then include it
            // in the profile list widget
            //
            continue;
        }

        try {
            if ( ( profile = json::parse( toml::find<std::string>( entry, "profile" ) ) ).is_discarded() ) {
                spdlog::error( "profile from toml entry has been discarded" );
                goto LEAVE;
            }
        } catch ( std::exception& e ) {
            spdlog::error( "failed to parse profile from toml entry:\n{}", e.what() );
            goto LEAVE;
        }

        auto item   = new QListWidgetItem;
        auto widget = new HcProfileItem(
            QString::fromStdString( name ),
            QString::fromStdString( type ),
            profile,
            SelectDialog
        );

        item->setSizeHint( widget->sizeHint() );
        widget->setFocusPolicy( Qt::NoFocus );

        ListProfiles->addItem( item );
        ListProfiles->setItemWidget( item, widget );
    }

    //
    // check if we even have any entries with the profile
    //

    if ( !ListProfiles->count() ) {
        spdlog::debug( "no profile entries in the list dialog" );
        goto LEAVE;
    }

    ListProfiles->clearSelection();
    ListProfiles->setFocusPolicy( Qt::NoFocus );

    if ( SelectDialog->exec() == QDialog::Rejected ) {
        goto LEAVE;
    }

    if ( !ListProfiles->currentItem() ) {
        spdlog::debug( "no profile has been specified" );
        goto LEAVE;
    }

    Profile = dynamic_cast<HcProfileItem*>(
        ListProfiles->itemWidget( ListProfiles->currentItem() )
    )->profile;

LEAVE:
    for ( int i = 0; i < ListProfiles->count(); ++i ) {
        auto item   = ListProfiles->item( i );
        auto widget = ListProfiles->itemWidget( item );

        if ( widget ) {
            delete widget;
        }

        delete item;
    }
    ListProfiles->clear();

    delete SelectDialog;

    return Profile;
}

/*!
 * @brief
 *  query a specific profile
 *
 * @param profile
 *  profile name to query
 *
 * @return
 *  profile object
 */
auto HcAgentProfileQuery(
    const std::string& profile
) -> json {
    auto name = std::string();
    auto type = std::string();
    auto conf = json();

    if ( profile.empty() ) {
        throw std::runtime_error( "profile name is empty" );
    }

    for ( const auto& _profile : Havoc->ProfileQuery( "profile" ) ) {
        if ( !_profile.contains( "name" ) ||
             !_profile.contains( "type" ) ||
             !_profile.contains( "profile" )
        ) {
            continue;
        };

        name = toml::find<std::string>( _profile, "name" );
        type = toml::find<std::string>( _profile, "type" );
        if ( ( conf = json::parse( toml::find<std::string>( _profile, "profile" ) ) ).is_discarded() ) {
            throw std::runtime_error( "failed to parse profile: has been discarded" );
        };

        if ( name != profile ) {
            continue;
        };

        return json {
            { "name",    name },
            { "type",    type },
            { "profile", conf },
        };
    };

    throw std::runtime_error( "profile has not been found" );
}

/*!
 * @brief
 *  return all registered profiles
 *
 * @return
 *  list of profiles
 */
auto HcAgentProfileList(
    void
) -> std::vector<json> {
    auto name = std::string();
    auto type = std::string();
    auto conf = json();
    auto list = std::vector<json>();

    for ( const auto& _profile : Havoc->ProfileQuery( "profile" ) ) {
        if ( !_profile.contains( "name" ) ||
             !_profile.contains( "type" ) ||
             !_profile.contains( "profile" )
        ) {
            continue;
        };

        name = toml::find<std::string>( _profile, "name" );
        type = toml::find<std::string>( _profile, "type" );
        if ( ( conf = json::parse( toml::find<std::string>( _profile, "profile" ) ) ).is_discarded() ) {
            throw std::runtime_error( "failed to parse profile: has been discarded" );
        };

        list.push_back( json {
            { "name",    name },
            { "type",    type },
            { "profile", conf },
        } );
    };

    return list;
}

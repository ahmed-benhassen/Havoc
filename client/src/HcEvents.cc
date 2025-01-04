#include <Havoc.h>

auto HcApplication::eventDispatch(
    const json& event
) -> void {
    auto type = std::string();
    auto data = json();

    if ( !event.contains( "type" ) ) {
        spdlog::debug( "invalid event: {}", event.dump() );
        return;
    }

    if ( !event.contains( "data" ) ) {
        spdlog::debug( "invalid event: {}", event.dump() );
        return;
    }

    type = event[ "type" ].get<std::string>();
    data = event[ "data" ].get<json>();

    if ( type == Event::user::login )
    {
        if ( data.empty() ) {
            spdlog::error( "user::login: invalid package (data emtpy)" );
            return;
        }
    }
    else if ( type == Event::user::logout )
    {
        if ( data.empty() ) {
            spdlog::error( "user::logout: invalid package (data emtpy)" );
            return;
        }
    }
    else if ( type == Event::user::message )
    {

    }
    else if ( type == Event::listener::add )
    {
        if ( data.empty() ) {
            spdlog::error( "listener::register: invalid package (data emtpy)" );
            return;
        }

        ui->PageListener->Protocols.push_back( data );
    }
    else if ( type == Event::listener::start )
    {
        auto name   = std::string();
        auto status = std::string();

        if ( data.empty() ) {
            spdlog::error( "Event::listener::start: invalid package (data emtpy)" );
            return;
        }

        ui->AddListener( data );
    }
    else if ( type == Event::listener::stop )
    {
        auto name   = std::string();
        auto status = std::string();

        if ( data.empty() ) {
            spdlog::error( "Event::listener::status: invalid package (data emtpy)" );
            return;
        }

        if ( data.contains( "name" ) ) {
            if ( data[ "name" ].is_string() ) {
                name = data[ "name" ].get<std::string>();
            } else {
                spdlog::error( "invalid listener status: \"name\" is not string" );
                return;
            }
        } else {
            spdlog::error( "invalid listener status: \"name\" is not found" );
            return;
        }

        if ( data.contains( "status" ) ) {
            if ( data[ "status" ].is_string() ) {
                status = data[ "status" ].get<std::string>();
            } else {
                spdlog::error( "invalid listener status: \"status\" is not string" );
                return;
            }
        } else {
            spdlog::error( "invalid listener status: \"status\" is not found" );
            return;
        }

        ui->PageListener->setListenerStatus( name, status );
    }
    else if ( type == Event::listener::status )
    {
        auto name = std::string();
        auto log  = std::string();

        if ( data.empty() ) {
            spdlog::error( "Event::listener::status: invalid package (data emtpy)" );
            return;
        }

        if ( data.contains( "name" ) ) {
            if ( data[ "name" ].is_string() ) {
                name = data[ "name" ].get<std::string>();
            } else {
                spdlog::error( "invalid listener status: \"name\" is not string" );
                return;
            }
        } else {
            spdlog::error( "invalid listener status: \"name\" is not found" );
            return;
        }

        if ( data.contains( "status" ) ) {
            if ( data[ "status" ].is_string() ) {
                log = data[ "status" ].get<std::string>();
            } else {
                spdlog::error( "invalid listener status: \"status\" is not string" );
                return;
            }
        } else {
            spdlog::error( "invalid listener status: \"status\" is not found" );
            return;
        }

        ui->PageListener->setListenerStatus( name, log );
    }
    else if ( type == Event::listener::log )
    {
        auto name = std::string();
        auto log  = std::string();

        if ( data.empty() ) {
            spdlog::error( "Event::listener::log: invalid package (data emtpy)" );
            return;
        }

        if ( data.contains( "name" ) ) {
            if ( data[ "name" ].is_string() ) {
                name = data[ "name" ].get<std::string>();
            } else {
                spdlog::error( "invalid listener log: \"name\" is not string" );
                return;
            }
        } else {
            spdlog::error( "invalid listener log: \"name\" is not found" );
            return;
        }

        if ( data.contains( "log" ) ) {
            if ( data[ "log" ].is_string() ) {
                log = data[ "log" ].get<std::string>();
            } else {
                spdlog::error( "invalid listener log: \"log\" is not string" );
                return;
            }
        } else {
            spdlog::error( "invalid listener log: \"log\" is not found" );
            return;
        }

        ui->PageListener->addListenerLog( name, log );
    }
    else if ( type == Event::listener::remove )
    {
        auto name = std::string();

        if ( data.empty() ) {
            spdlog::error( "Event::listener::remove: invalid package (data emtpy)" );
            return;
        }

        if ( data.contains( "name" ) ) {
            if ( data[ "name" ].is_string() ) {
                name = data[ "name" ].get<std::string>();
            } else {
                spdlog::error( "invalid listener remove: \"name\" is not string" );
                return;
            }
        } else {
            spdlog::error( "invalid listener remove: \"name\" is not found" );
            return;
        }

        RemoveListener( name );
    }
    else if ( type == Event::agent::add )
    {
        //
        // nothing
        //
    }
    else if ( type == Event::agent::initialize )
    {
        if ( data.empty() ) {
            spdlog::error( "Event::agent::initialize: invalid package (data emtpy)" );
            return;
        }

        ui->AddAgent( data );
    }
    else if ( type == Event::agent::callback )
    {
        auto uuid          = std::string();
        auto ctx           = json();
        auto pat           = std::string();
        auto out           = std::string();
        auto callback_uuid = std::string();

        //
        // TODO: when the client disconnects do following
        //       close all callbacks:
        //          close all callbacks associated with the client on the team server
        //          side so the current client wont receive any unregistered callbacks
        //          that does not longer exist etc.
        //       redesign callback api:
        //          design the callback api in a way where it gets restored and continue
        //          executing the script
        //

        if ( data.empty() ) {
            spdlog::error( "Event::agent::callback: invalid package (data emtpy)" );
            return;
        }

        if ( data.contains( "uuid" ) ) {
            if ( data[ "uuid" ].is_string() ) {
                uuid = data[ "uuid" ].get<std::string>();
            } else {
                spdlog::error( "invalid agent callback: \"uuid\" is not string" );
                return;
            }
        } else {
            spdlog::error( "invalid agent callback: \"uuid\" is not found" );
            return;
        }

        //
        // this type indicates that a callback function should be invoked
        // with the following callback uuid (inside the uuid member)
        //
        if ( data.contains( "data" ) && data[ "data" ].is_object() ) {
            data = data[ "data" ].get<json>();

            //
            // acquire python gil to interact with the
            // python function and call the callback
            //
            HcPythonAcquire();

            //
            // get the data to pass to the callback
            //
            if ( data.contains( "data" ) ) {
                if ( data[ "data" ].is_string() ) {
                    out = data[ "data" ].get<std::string>();
                } else {
                    spdlog::error( "invalid agent callback: \"data\" is not string" );
                    return;
                }
            } else {
                spdlog::error( "invalid agent callback: \"data\" is not found" );
                return;
            }

            //
            // get the task uuid to pass to the callback
            //
            if ( data.contains( "callback" ) ) {
                if ( data[ "callback" ].is_string() ) {
                    callback_uuid = data[ "callback" ].get<std::string>();
                } else {
                    spdlog::error( "invalid agent callback: \"callback\" is not a string" );
                    return;
                }
            } else {
                spdlog::error( "invalid agent callback: \"callback\" is not found" );
                return;
            }

            //
            // get context from the callback that needs to
            // be passed to the callback python function
            //
            if ( data.contains( "context" ) ) {
                if ( data[ "context" ].is_object() ) {
                    ctx = data[ "context" ].get<json>();
                } else {
                    spdlog::error( "invalid agent callback: \"context\" is not an object" );
                    return;
                }
            } else {
                spdlog::error( "invalid agent callback: \"context\" is not found" );
                return;
            }

            //
            // get registered callback object
            auto callback = Havoc->CallbackObject( callback_uuid );
            if ( !callback.has_value() ) {
                //
                // this happens if we restarted the client
                // while a script was in use of a callback
                spdlog::debug( "CallbackObject has no value: {}", callback_uuid );
                return;
            }

            //
            // search for the agent via the specified uuid &
            // check if we found the agent and if it contains
            // a python interface
            auto agent = Havoc->ui->PageAgent->Agent( uuid );
            if ( agent.has_value() && agent.value()->interface().has_value() ) {
                pat = QByteArray::fromBase64( out.c_str() ).toStdString();

                try {
                    //
                    // actually invoke the callback with following arguments:
                    //
                    //   def callback( agent: HcAgent, data: bytes, **kwargs ):
                    //      # agent interface can be invoked now
                    //      return
                    //
                    callback.value()(
                        agent.value()->interface().value(),
                        py11::bytes( pat.c_str(), pat.length() ),
                        **py11::dict( ctx )
                    );
                } catch ( py11::error_already_set &eas ) {
                    //
                    // catch exception and print it to the agent
                    // console as it was running under its context
                    //
                    emit agent.value()->ui.signal.ConsoleWrite( agent.value()->uuid().c_str(), eas.what() );
                }
            } else {
                spdlog::error(
                    "[agent.has_value(): {}] [agent.value()->interface.has_value(): {}]",
                    agent.has_value(),
                    agent.has_value() && agent.value()->interface().has_value()
                );
            }
        } else {
            spdlog::error( "invalid agent callback: \"data\" is not found or not an object" );
            return;
        }
    }
    else if ( type == Event::agent::console )
    {
        ui->AgentConsole( data );
    }
    else if ( type == Event::agent::heartbeat )
    {
        auto uuid = std::string();
        auto time = std::string();

        if ( data.empty() ) {
            spdlog::error( "Event::agent::heartbeat: invalid package (data emtpy)" );
            return;
        }

        if ( data.contains( "uuid" ) ) {
            if ( data[ "uuid" ].is_string() ) {
                uuid = data[ "uuid" ].get<std::string>();
            } else {
                spdlog::error( "invalid agent heartbeat: \"uuid\" is not string" );
                return;
            }
        } else {
            spdlog::error( "invalid agent heartbeat: \"uuid\" is not found" );
            return;
        }

        //
        // get the data to pass to the callback
        //
        if ( data.contains( "time" ) ) {
            if ( data[ "time" ].is_string() ) {
                time = data[ "time" ].get<std::string>();
            } else {
                spdlog::error( "invalid agent heartbeat: \"time\" is not string" );
                return;
            }
        } else {
            spdlog::error( "invalid agent heartbeat: \"time\" is not found" );
            return;
        }

        //
        // set the last callback of the agent
        //
        if ( auto _value = Agent( uuid ); _value.has_value() ) {
            auto agent = _value.value();

            agent->setLast( time );

            //
            // update the pulsation inside the graph
            //
            agent->ui.node->itemEdge()->startPulsation();
        } else {
            spdlog::error( "invalid agent heartbeat: \"uuid\" agent not found" );
        }
    }
    else if ( type == Event::agent::status )
    {
        spdlog::debug( "Event::agent::status: {}", data.dump() );

        auto uuid   = std::string();
        auto status = std::string();

        if ( data.empty() ) {
            spdlog::error( "Event::agent::heartbeat: invalid package (data emtpy)" );
            return;
        }

        if ( data.contains( "uuid" ) ) {
            if ( data[ "uuid" ].is_string() ) {
                uuid = data[ "uuid" ].get<std::string>();
            } else {
                spdlog::error( "invalid agent heartbeat: \"uuid\" is not string" );
                return;
            }
        } else {
            spdlog::error( "invalid agent heartbeat: \"uuid\" is not found" );
            return;
        }

        if ( data.contains( "status" ) ) {
            if ( data[ "status" ].is_string() ) {
                status = data[ "status" ].get<std::string>();
            } else {
                spdlog::error( "invalid agent heartbeat: \"uuid\" is not string" );
                return;
            }
        } else {
            spdlog::error( "invalid agent heartbeat: \"uuid\" is not found" );
            return;
        }

        if ( auto _value = Agent( uuid ); _value.has_value() ) {
            auto agent   = _value.value();
            auto _status = HcAgentStatus();

            if ( status == AgentStatus::disconnected ) {
                _status = AgentStatusDisconnected;
            } else if ( status == AgentStatus::unresponsive ) {
                _status = AgentStatusUnresponsive;
            } else if ( status == AgentStatus::healthy ) {
                _status = AgentStatusHealthy;
            }

            agent->setStatus( _status );
        }
    }
    else if ( type == Event::agent::remove )
    {
        auto uuid = std::string();

        if ( data.empty() ) {
            spdlog::error( "Event::agent::remove: invalid package (data emtpy)" );
            return;
        }

        if ( data.contains( "uuid" ) ) {
            if ( data[ "uuid" ].is_string() ) {
                uuid = data[ "uuid" ].get<std::string>();
            } else {
                spdlog::error( "invalid agent remove: \"uuid\" is not string" );
                return;
            }
        } else {
            spdlog::error( "invalid agent remove: \"uuid\" is not found" );
            return;
        }

        ui->RemoveAgent( uuid );
    }
    else if ( type == Event::agent::buildlog )
    {
        auto log = std::string();

        if ( data.empty() ) {
            spdlog::error( "Event::agent::buildlog: invalid package (data emtpy)" );
            return;
        }

        if ( data.contains( "log" ) ) {
            if ( data[ "log" ].is_string() ) {
                log = data[ "log" ].get<std::string>();

                //
                // signal opened dialog of the build message
                //
                emit ui->signalBuildLog( log.c_str() );
            } else {
                spdlog::error( "invalid agent build log: \"log\" is not string" );
                return;
            }
        } else {
            spdlog::error( "invalid agent build log: \"log\" is not found" );
            return;
        }
    } else if ( type == Event::agent::note ) {
        auto uuid = std::string();
        auto note = std::string();

        if ( data.empty() ) {
            spdlog::error( "Event::agent::note: invalid package (data emtpy)" );
            return;
        }

        spdlog::debug( "note: {}", data.dump() );

        if ( data.contains( "uuid" ) ) {
            if ( data[ "uuid" ].is_string() ) {
                uuid = data[ "uuid" ].get<std::string>();
            } else {
                spdlog::error( "invalid agent callback: \"uuid\" is not string" );
                return;
            }
        } else {
            spdlog::error( "invalid agent callback: \"uuid\" is not found" );
            return;
        }

        if ( data.contains( "note" ) ) {
            if ( data[ "note" ].is_string() ) {
                note = data[ "note" ].get<std::string>();
            } else {
                spdlog::error( "invalid agent callback: \"note\" is not string" );
                return;
            }
        } else {
            spdlog::error( "invalid agent callback: \"note\" is not found" );
            return;
        }

        //
        // set the note of the agent
        //
        if ( auto agent = Agent( uuid ) ) {
            if ( agent.has_value() ) {
                agent.value()->ui.table.Note->ignore = true;
                agent.value()->ui.table.Note->setText( note.c_str() );
                agent.value()->ui.table.Note->ignore = false;
            } else {
                spdlog::error( "invalid agent note: \"uuid\" agent does not have any value" );
            }
        } else {
            spdlog::error( "invalid agent note: \"uuid\" agent not found" );
        }

    } else {
        spdlog::debug( "invalid event: {} not found", type );
    }
}



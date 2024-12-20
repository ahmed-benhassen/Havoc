#include <Havoc.h>
#include <core/HcMetaWorker.h>

//
// TODO: pull agent console as well if configured
//       handle race condition when agent/listener
//       gets created inbetween the eventWorkerRun
//       and after the pulling has been done
//

HcMetaWorker::HcMetaWorker()  = default;
HcMetaWorker::~HcMetaWorker() = default;

void HcMetaWorker::run() {
    //
    // process all the plugins, listeners,
    // agents, operator connections, etc.
    //
    listeners();
    agents();

    emit Finished();

    quit();
}

auto HcMetaWorker::listeners(
    void
) -> void {
    auto listeners = json();

    Havoc->splash->showMessage( "pulling active listeners", Qt::AlignLeft | Qt::AlignBottom, Qt::white );

    auto [status_code, response] = Havoc->ApiSend( "api/listener/list", {} );

    if ( status_code != 200 ) {
        Helper::MessageBox(
            QMessageBox::Critical,
            "listener processing failure",
            "failed to pull all active listeners"
        );
        return;
    }

    try {
        if ( ( listeners = json::parse( response ) ).is_discarded() ) {
            spdlog::debug( "listeners processing json response has been discarded" );
            return;
        }
    } catch ( std::exception& e ) {
        spdlog::error( "failed to parse listener processing json response: \n{}", e.what() );
        return;
    }

    if ( listeners.empty() ) {
        spdlog::debug( "no listeners to process" );
        return;
    }

    if ( ! listeners.is_array() ) {
        spdlog::error( "listeners response is not an array" );
        return;
    }

    for ( auto& listener : listeners ) {
        if ( ! listener.is_object() ) {
            spdlog::debug( "warning! listener processing item is not an object" );
            continue;
        }

        spdlog::debug( "processing listener: {}", listener.dump() );

        emit AddListener( listener );
    }
}

auto HcMetaWorker::agents(
    void
) -> void {
    auto agents = json();
    auto uuid   = std::string();

    Havoc->splash->showMessage( "pulling agent sessions", Qt::AlignLeft | Qt::AlignBottom, Qt::white );

    auto [status_code, response] = Havoc->ApiSend( "api/agent/list", {} );

    if ( status_code != 200 ) {
        Helper::MessageBox(
            QMessageBox::Critical,
            "agent processing failure",
            "failed to pull all active agent sessions"
        );
        return;
    }

    try {
        if ( ( agents = json::parse( response ) ).is_discarded() ) {
            spdlog::debug( "agent processing json response has been discarded" );
            return;
        }
    } catch ( std::exception& e ) {
        spdlog::error( "failed to parse agent processing json response: \n{}", e.what() );
        return;
    }

    if ( agents.empty() ) {
        spdlog::debug( "no agents to process" );
        return;
    }

    if ( ! agents.is_array() ) {
        spdlog::error( "agents response is not an array" );
        return;
    }

    //
    // iterate over all available agents
    // and pull console logs as well
    //
    for ( auto& agent : agents ) {
        if ( ! agent.is_object() ) {
            spdlog::debug( "warning! agent processing item is not an object" );
            continue;
        }

        spdlog::debug( "processing agent: {}", agent.dump() );

        emit AddAgent( agent );

        //
        // parse the agent metadata to retrieve the uuid which will then be
        // passed to the console function to retrieve all console logs
        //
        if ( agent.contains( "uuid" ) && agent[ "uuid" ].is_string() ) {
            console(
                agent[ "uuid" ].get<std::string>()
            );
        } else {
            spdlog::error( "[HcMetaWorker::agents] agent does not contain valid uuid" );
            continue;
        }
    }
}

auto HcMetaWorker::console(
    const std::string& uuid
) -> void {
    auto list = json();

    auto [status_code, response] = Havoc->ApiSend( "api/agent/console", { { "uuid", uuid } } );

    if ( status_code != 200 ) {
        Helper::MessageBox(
            QMessageBox::Critical,
            "agent console processing failure",
            "failed to pull all agent console logs"
        );
        return;
    }

    try {
        if ( ( list = json::parse( response ) ).is_discarded() ) {
            spdlog::debug( "agent console processing json response has been discarded" );
            return;
        }
    } catch ( std::exception& e ) {
        spdlog::error( "failed to parse agent console processing json response: \n{}", e.what() );
        return;
    }

    if ( list.empty() ) {
        spdlog::debug( "no agent console to process" );
        return;
    }

    if ( ! list.is_array() ) {
        spdlog::error( "agents console response is not an array" );
        return;
    }

    for ( auto& console : list ) {
        if ( ! console.is_object() ) {
            spdlog::debug( "warning! agent console processing item is not an object" );
            continue;
        }

        emit AddAgentConsole( console );
    }
}
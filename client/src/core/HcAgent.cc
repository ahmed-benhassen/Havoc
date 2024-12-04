#include <Havoc.h>
#include <core/HcAgent.h>

std::map<
    std::string,
    std::vector<std::pair<std::string, std::string>>
>* HcAgentCompletionList = new std::map<std::string, std::vector<std::pair<std::string, std::string>>>();

HcAgent::HcAgent(
    const json& metadata
) : _data( metadata ), _console( nullptr ), _hidden( false ), ui() {
    return;
}

auto HcAgent::initialize() -> bool {
    auto arch    = QString();
    auto user    = QString();
    auto host    = QString();
    auto local   = QString();
    auto path    = QString();
    auto process = QString();
    auto pid     = QString();
    auto tid     = QString();
    auto system  = QString();
    auto note    = QString();
    auto meta    = json();

    _hidden = false;

    if ( _data.contains( "uuid" ) && _data[ "uuid" ].is_string() ) {
        _uuid = _data[ "uuid" ].get<std::string>();
    } else {
        spdlog::error( "[HcAgent::parse] agent does not contain valid uuid" );
        return false;
    }

    if ( _data.contains( "type" ) && _data[ "type" ].is_string() ) {
        _type = _data[ "type" ].get<std::string>();
    } else {
        spdlog::error( "[HcAgent::parse] agent does not contain valid type" );
        return false;
    }

    if ( _data.contains( "note" ) && _data[ "note" ].is_string() ) {
        note = QString( _data[ "note" ].get<std::string>().c_str() );
    } else {
        spdlog::debug( "[HcAgent::parse] agent does not contain any note" );
    }

    if ( _data.contains( "status" ) && _data[ "status" ].is_string() ) {
        _status = _data[ "status" ].get<std::string>();
    } else {
        spdlog::debug( "[HcAgent::parse] agent does not contain any status" );
    }

    if ( _data.contains( "meta" ) && _data[ "meta" ].is_object() ) {
        meta = _data[ "meta" ].get<json>();
    } else {
        spdlog::debug( "[HcAgent::parse] agent does not contain valid meta object" );
        return false;
    }

    if ( meta.contains( "parent" ) && meta[ "parent" ].is_string() ) {
        _parent = meta[ "parent" ].get<std::string>();
    } else {
        spdlog::debug( "{} has no parent (is direct connection)", uuid() );
    }

    if ( meta.contains( "user" ) && meta[ "user" ].is_string() ) {
        user = QString( meta[ "user" ].get<std::string>().c_str() );
    } else {
        spdlog::debug( "[HcAgent::parse] agent does not contain valid meta user" );
    }

    if ( meta.contains( "host" ) && meta[ "host" ].is_string() ) {
        host = QString( meta[ "host" ].get<std::string>().c_str() );
    } else {
        spdlog::debug( "[HcAgent::parse] agent does not contain valid meta host" );
    }

    if ( meta.contains( "arch" ) && meta[ "arch" ].is_string() ) {
        arch = QString( meta[ "arch" ].get<std::string>().c_str() );
    } else {
        spdlog::debug( "[HcAgent::parse] agent does not contain valid meta arch" );
    }

    if ( meta.contains( "local ip" ) && meta[ "local ip" ].is_string() ) {
        local = QString( meta[ "local ip" ].get<std::string>().c_str() );
    } else {
        spdlog::debug( "[HcAgent::parse] agent does not contain valid meta local ip" );
    }

    if ( meta.contains( "process path" ) && meta[ "process path" ].is_string() ) {
        path = QString( meta[ "process path" ].get<std::string>().c_str() );
    } else {
        spdlog::debug( "[HcAgent::parse] agent does not contain valid meta process path" );
    }

    if ( meta.contains( "process name" ) && meta[ "process name" ].is_string() ) {
        process = QString( meta[ "process name" ].get<std::string>().c_str() );
    } else {
        spdlog::debug( "[HcAgent::parse] agent does not contain valid meta process name" );
    }

    if ( meta.contains( "pid" ) && meta[ "pid" ].is_number_integer() ) {
        pid = QString::number( meta[ "pid" ].get<int>() );
    } else {
        spdlog::debug( "[HcAgent::parse] agent does not contain valid meta pid" );
    }

    if ( meta.contains( "tid" ) && meta[ "tid" ].is_number_integer() ) {
        tid = QString::number( meta[ "tid" ].get<int>() );
    } else {
        spdlog::debug( "[HcAgent::parse] agent does not contain valid meta tid" );
    }

    if ( meta.contains( "system" ) && meta[ "system" ].is_string() ) {
        system = QString( meta[ "system" ].get<std::string>().c_str() );
    } else {
        spdlog::debug( "[HcAgent::parse] agent does not contain valid meta system" );
    }

    if ( meta.contains( "last callback" ) && meta[ "last callback" ].is_string() ) {
        setLast( meta[ "last callback" ].get<std::string>() );
    } else {
        spdlog::debug( "[HcAgent::parse] agent does not contain valid meta last" );
    }

    ui.table = {
        .Uuid        = new HcAgentTableItem( uuid().c_str(), this ),
        .Internal    = new HcAgentTableItem( local, this ),
        .Username    = new HcAgentTableItem( user, this ),
        .Hostname    = new HcAgentTableItem( host, this ),
        .ProcessPath = new HcAgentTableItem( path, this ),
        .ProcessName = new HcAgentTableItem( process, this ),
        .ProcessId   = new HcAgentTableItem( pid, this ),
        .ThreadId    = new HcAgentTableItem( tid, this ),
        .Arch        = new HcAgentTableItem( arch, this ),
        .System      = new HcAgentTableItem( system, this ),
        .Note        = new HcAgentTableItem( note, this, Qt::NoItemFlags, Qt::AlignVCenter ),
        .Last        = new HcAgentTableItem( "", this ),
    };

    _console = new HcAgentConsole( this );
    _console->setBottomLabel( QString( "[User: %1] [Process: %2] [Pid: %3] [Tid: %4]" ).arg( user ).arg( path ).arg( pid ).arg( tid ) );
    _console->setInputLabel( ">>>" );
    _console->LabelHeader->setFixedHeight( 0 );

    if ( ! HcAgentCompletionList->empty() ) {
        for ( int i = 0; i < HcAgentCompletionList->at( _type ).size(); i++ ) {
            auto [command, description] = HcAgentCompletionList->at( _type ).at( i );

            _console->addCompleteCommand( command, description );
        }
    }

    return true;
}

auto HcAgent::post(
    void
) -> void {
    if ( status() == AgentStatus::disconnected ) {
        // disconnected();
    } else if ( status() == AgentStatus::unresponsive ) {
        // unresponsive();
    } else if ( status() == AgentStatus::healthy ) {
        // healthy();
    }

    //
    // if an interface has been registered then assign it to the agent
    //
    _interface = std::nullopt;
    if ( const auto object = Havoc->AgentObject( _type ); object.has_value() ) {
        HcPythonAcquire();

        try {
            _interface = object.value()( _uuid, _type, _data[ "meta" ].get<json>() );
        } catch ( py11::error_already_set &eas ) {
            spdlog::error( "failed to invoke agent interface [uuid: {}] [type: {}]: \n{}", uuid(), type(), eas.what() );

            emit ui.signal.ConsoleWrite( QString::fromStdString( _uuid ), eas.what() );
        }


        //
        // once the interface has been initialized we
        // can call all registered callbacks that wait
        // to be notified on newly connected
        //

        for ( const auto& [init_type, callback] : Havoc->InitializeEvents() ) {
            spdlog::debug( "init_type: {} ({})", init_type, init_type.empty() );

            if ( !init_type.empty() ) {
                if ( init_type == type() ) {
                    try {
                        callback( _interface.value() );
                    } catch ( py11::error_already_set& e ) {
                        spdlog::error( "failed to execute initialization event callback:\n{}", e.what() );
                    };
                };
            } else {
                try {
                    callback( _interface.value() );
                } catch ( py11::error_already_set& e ) {
                    spdlog::error( "failed to execute initialization event callback:\n{}", e.what() );
                };
            };
        };
    }

    if ( _image.isNull() ) {
        //
        // if the python object agent interface has not
        // registered any kind of image or icon for the
        // agent session then we are going to set unknown
        //
        emit ui.signal.RegisterIconName( QString::fromStdString( _uuid ), "unknown" );
    }
}

HcAgent::~HcAgent() = default;

auto HcAgent::uuid() -> std::string
{
    return _uuid;
}

auto HcAgent::type() -> std::string
{
    return _type;
}

auto HcAgent::parent() -> std::string
{
    return _parent;
}

auto HcAgent::setParent(
    const std::string& parent
) -> void {
    _parent = parent;

    //
    // TODO: do some reprocessing of agent parent (relink in the graph etc.)
    //
}

auto HcAgent::status() -> std::string
{
    return _status;
}

auto HcAgent::setStatus(
    HcAgentStatus status
) -> void {
    auto edge_color   = Havoc->Theme.getComment();
    auto status_color = QColor();
    auto status_name  = QString();

    //
    // set the agent status
    if ( status == AgentStatusHealthy ) {
        _status    = AgentStatus::healthy;
        edge_color = parent().empty() ? Havoc->Theme.getGreen() : Havoc->Theme.getPurple();
    }
    else if ( status == AgentStatusDisconnected ) {
        status_name  = QString::fromStdString( _status = AgentStatus::disconnected );
        status_color = Havoc->Theme.getRed();
    }
    else if ( status == AgentStatusUnresponsive ) {
        status_name  = QString::fromStdString( _status = AgentStatus::unresponsive );
        status_color = Havoc->Theme.getOrange();
    }

    //
    // set the edge color and the status string & color
    if ( ui.node->itemEdge() ) {
        ui.node->itemEdge()->setColor( edge_color );
        ui.node->itemEdge()->setStatus( status_name, status_color );
    }
}

auto HcAgent::setLast(
    const std::string& last
) -> QDateTime {
    return ( _last = QDateTime::fromString(
        QString::fromStdString( last ),
        "dd-MMM-yyyy HH:mm:ss t"
    ) );
}

auto HcAgent::last() -> QDateTime
{
    return _last;
}

auto HcAgent::data() -> nlohmann::json
{
    return _data;
}

auto HcAgent::interface() -> std::optional<pybind11::object>
{
    return _interface;
}

auto HcAgent::image() -> QImage
{
    return _image;
}

auto HcAgent::setImage(
    const QImage& image
) -> void {
    _image = image;
}

auto HcAgent::writeConsole(
    const std::string& text
) -> void {
    //
    // write to the agent console from any thread
    emit ui.signal.ConsoleWrite(
        QString::fromStdString( uuid() ),
        QString::fromStdString( text )
    );
}

auto HcAgent::remove() -> void {
    spdlog::debug( "agent::remove {}", uuid() );

    auto [status, response] = Havoc->ApiSend( "/api/agent/remove", { { "uuid", uuid() } } );

    if ( status != 200 ) {
        Helper::MessageBox(
            QMessageBox::Critical,
            "agent removal failure",
            std::format( "failed to remove agent {}: {}", uuid(), response )
        );

        spdlog::error( "failed to remove agent {}: {}", uuid(), response );
    }
}

auto HcAgent::hidden() -> bool
{
    return _hidden;
}

auto HcAgent::setHidden(
    bool hide
) -> void {
    const auto show_hidden = Havoc->ui->PageAgent->show_hidden;
    const auto table       = Havoc->ui->PageAgent->AgentTable;
    const auto row         = ui.table.Uuid->row();

    //
    // TODO: fix this. this again doesnt work after the rewrite
    if ( (_hidden != hide) ) {
        _hidden = hide;

        if ( _hidden ) {
            if ( !show_hidden ) {
                table->hideRow( row );
            }
            for ( int i = 0; i < table->columnCount(); i++ ) {
                table->item( row, i )->setForeground( Havoc->Theme.getComment() );
            }
        } else {
            table->showRow( row );
            for ( int i = 0; i < table->columnCount(); i++ ) {
                table->item( row, i )->setForeground( Havoc->Theme.getForeground() );
            }
        }
    }
}

// auto HcAgent::hide(
//     void
// ) -> void {
//     const auto show_hidden = Havoc->ui->PageAgent->show_hidden;
//     const auto table       = Havoc->ui->PageAgent->AgentTable;
//     const auto row         = ui.table.Uuid->row();
//
//     if ( !show_hidden ) {
//         table->hideRow( row );
//     }
//
//     if ( hidden() ) {
//         for ( int i = 0; i < table->columnCount(); i++ ) {
//             table->item( row, i )->setForeground( Havoc->Theme.getComment() );
//         }
//     }
// }
//
// auto HcAgent::unhide(
//     void
// ) -> void {
//     const auto table = Havoc->ui->PageAgent->AgentTable;
//     const auto row   = ui.table.Uuid->row();
//
//     table->showRow( row );
//
//     if ( !hidden() ) {
//         for ( int i = 0; i < table->columnCount(); i++ ) {
//             table->item( row, i )->setForeground( Havoc->Theme.getForeground() );
//         }
//     }
// }
//
// auto HcAgent::disconnected(
//     void
// ) -> void {
//     spdlog::debug( "agent::disconnected {}", uuid() );
//
//     if ( ui.node->itemEdge() ) {
//         ui.node->itemEdge()->setColor( Havoc->Theme.getComment() );
//         ui.node->itemEdge()->setStatus(
//             QString::fromStdString( AgentStatus::disconnected ),
//             Havoc->Theme.getRed()
//         );
//     }
// }
//
// auto HcAgent::unresponsive(
//     void
// ) -> void {
//     spdlog::debug( "agent::unresponsive {}", uuid() );
//
//     if ( ui.node->itemEdge() ) {
//         ui.node->itemEdge()->setColor( Havoc->Theme.getComment() );
//         ui.node->itemEdge()->setStatus(
//             QString::fromStdString( AgentStatus::unresponsive ),
//             Havoc->Theme.getOrange()
//         );
//     }
// }
//
// auto HcAgent::healthy(
//     void
// ) -> void {
//     spdlog::debug( "agent::healthy {}", uuid() );
//
//     if ( ui.node->itemEdge() ) {
//         ui.node->itemEdge()->setColor( parent().empty() ? Havoc->Theme.getGreen() : Havoc->Theme.getPurple() );
//         ui.node->itemEdge()->setStatus( QString(), QColor() );
//     }
// }

auto HcAgent::console() const -> HcAgentConsole*
{
    return _console;
}

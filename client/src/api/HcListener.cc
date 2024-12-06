#include <Common.h>
#include <Havoc.h>
#include <api/HcListener.h>

auto HcListenerProtocolData(
    const std::string& protocol
) -> json {

    for ( auto& p : Havoc->ui->PageListener->Protocols ) {
        if ( p.contains( "data" ) ) {
            if ( p[ "data" ].contains( "protocol" ) ) {
                if ( p[ "data" ][ "protocol" ] == protocol ) {
                    if ( p[ "data" ].contains( "data" ) ) {
                        return p[ "data" ][ "data" ];
                    }
                }
            }
        }
    }

    return {};
}

auto HcListenerAll(
    void
) -> std::vector<std::string> {
    return Havoc->Listeners();
}

auto HcListenerQueryType(
    const std::string& name
) -> std::string {
    for ( auto& n : Havoc->Listeners() ) {
        if ( auto obj = Havoc->ListenerObject( n ) ) {
            if ( obj.has_value() && obj.value()[ "name" ] == name ) {
                return obj.value()[ "protocol" ].get<std::string>();
            }
        }
    }

    return std::string();
}

auto HcListenerRegisterMenuAction(
    const std::string&  type,
    const std::string&  name,
    const std::string&  icon,
    const py11::object& callback
) -> void {
    auto action = new HcApplication::ActionObject();

    action->type          = HcApplication::ActionObject::ActionListener;
    action->name          = name;
    action->icon          = QIcon( QString::fromStdString( icon ) );
    action->callback_py   = callback;
    action->listener.type = type;

    Havoc->AddAction( action );
}

auto HcListenerPopupSelect(
    const std::string& protocol
) -> json {
    auto dialog   = new HcListenerChooseDialog( protocol );
    auto listener = json();

    //
    // we are queueing up a function call to
    // be executed int the GUI main thread
    // and wait til the execution finished
    //
    QMetaObject::invokeMethod( qApp, [&]() {
        dialog->start();

        listener = dialog->listenerData();

        delete dialog;
    }, Qt::BlockingQueuedConnection );

    return listener;
}

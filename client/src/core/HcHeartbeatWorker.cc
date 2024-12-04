#include <Havoc.h>
#include <core/HcHeartbeatWorker.h>

HcHeartbeatWorker::HcHeartbeatWorker()  = default;
HcHeartbeatWorker::~HcHeartbeatWorker() = default;

void HcHeartbeatWorker::run() {
    HeartbeatTimer = new QTimer;

    //
    // connect the updateHeartbeats to the timer
    //
    QObject::connect( HeartbeatTimer, &QTimer::timeout, this, &HcHeartbeatWorker::updateHeartbeats );

    //
    // execute the updater every 500 milliseconds
    //
    HeartbeatTimer->start( 500 );
}

/*!
 * @brief
 *  iterates over all registered agents
 *  and updates the last called time
 */
auto HcHeartbeatWorker::updateHeartbeats() -> void
{
    //
    // iterate over registered agents
    //
    for ( const auto& agent : Havoc->Agents() ) {
        //
        // parse the last called time, calculate the difference,
        // and get seconds, minutes, hours and days from it
        //
        auto last    = agent->last();
        auto current = QDateTime::currentDateTimeUtc();
        auto diff    = last.secsTo( current );
        auto days    = diff / ( 24 * 3600 );
        auto hours   = ( diff % ( 24 * 3600 ) ) / 3600;
        auto minutes = ( diff % 3600 ) / 60;
        auto seconds = diff % 60;
        auto text    = std::string();

        //
        // update the table value
        //

        if ( diff < 60 ) {
            text = std::format( "{}s", seconds );
        } else if ( diff < ( 60 * 60 ) ) {
            text = std::format( "{}m {}s", minutes, seconds );
        } else if ( diff < 24 * 60 * 60 ) {
            text = std::format( "{}h {}m", hours, minutes );
        } else {
            text = std::format( "{}d {}h", days, hours );
        }

        // spdlog::debug( "{} (from {}) : {}", last.toString().toStdString(), agent->last.toStdString(), current.toString().toStdString() );
        // spdlog::debug( "{} : {}", agent->uuid, text );

        agent->ui.table.Last->setText( QString::fromStdString( text ) );
    }
}


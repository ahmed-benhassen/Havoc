#include <Havoc.h>
#include <ui/HcPageAgent.h>
#include <QtConcurrent/QtConcurrent>

HcPageAgent::HcPageAgent(
    QWidget* parent
) : QWidget( parent ) {

    if ( objectName().isEmpty() ) {
        setObjectName( QString::fromUtf8( "PageAgent" ) );
    }

    gridLayout = new QGridLayout( this );
    gridLayout->setObjectName( "gridLayout" );

    ComboAgentView = new QComboBox( this );
    ComboAgentView->setObjectName( "ComboAgentView" );
    ComboAgentView->view()->setProperty( "ComboBox", "true" );

    Splitter = new QSplitter( this );
    Splitter->setObjectName( QString::fromUtf8( "Splitter" ) );
    Splitter->setOrientation( Qt::Vertical );

    AgentTable = new QTableWidget( Splitter );
    AgentTable->setObjectName( "AgentTable" );

    TitleAgentID      = new QTableWidgetItem( "UUID" );
    TitleInternal     = new QTableWidgetItem( "Internal" );
    TitleUsername     = new QTableWidgetItem( "User" );
    TitleHostname     = new QTableWidgetItem( "Host" );
    TitleSystem       = new QTableWidgetItem( "System" );
    TitleProcessID    = new QTableWidgetItem( "Pid" );
    TitleProcessName  = new QTableWidgetItem( "Process" );
    TitleProcessArch  = new QTableWidgetItem( "Arch" );
    TitleThreadID     = new QTableWidgetItem( "Tid" );
    TitleNote         = new QTableWidgetItem( "Note" );
    TitleLastCallback = new QTableWidgetItem( "Last" );

    if ( AgentTable->columnCount() < 11 ) {
        AgentTable->setColumnCount( 11 );
    }

    /* TODO: get how we should add this from the settings
     * for now we just do a default one */
    AgentTable->setHorizontalHeaderItem( 0,  TitleAgentID      );
    AgentTable->setHorizontalHeaderItem( 1,  TitleInternal     );
    AgentTable->setHorizontalHeaderItem( 2,  TitleUsername     );
    AgentTable->setHorizontalHeaderItem( 3,  TitleHostname     );
    AgentTable->setHorizontalHeaderItem( 4,  TitleProcessName  );
    AgentTable->setHorizontalHeaderItem( 5,  TitleProcessID    );
    AgentTable->setHorizontalHeaderItem( 6,  TitleThreadID     );
    AgentTable->setHorizontalHeaderItem( 7,  TitleProcessArch  );
    AgentTable->setHorizontalHeaderItem( 8,  TitleSystem       );
    AgentTable->setHorizontalHeaderItem( 9,  TitleLastCallback );
    AgentTable->setHorizontalHeaderItem( 10, TitleNote         );

    /* table settings */
    AgentTable->setEnabled( true );
    AgentTable->setShowGrid( false );
    AgentTable->setSortingEnabled( false );
    AgentTable->setWordWrap( true );
    AgentTable->setCornerButtonEnabled( true );
    AgentTable->horizontalHeader()->setVisible( true );
    AgentTable->setSelectionBehavior( QAbstractItemView::SelectRows );
    AgentTable->setContextMenuPolicy( Qt::CustomContextMenu );
    AgentTable->horizontalHeader()->setSectionResizeMode( QHeaderView::ResizeMode::Stretch );
    AgentTable->horizontalHeader()->setStretchLastSection( true );
    AgentTable->verticalHeader()->setVisible( false );
    AgentTable->setFocusPolicy( Qt::NoFocus );

    AgentTab = new QTabWidget( Splitter );
    AgentTab->setObjectName( "AgentTab" );
    AgentTab->setTabsClosable( true );
    AgentTab->setMovable( true );
    AgentTab->tabBar()->setProperty( "HcTab", "true" );

    AgentDisplayerSessions = new QLabel( this );
    AgentDisplayerSessions->setObjectName( "LabelDisplaySessions" );
    AgentDisplayerSessions->setProperty( "HcLabelDisplay", "true" );

    AgentDisplayerTargets = new QLabel( this );
    AgentDisplayerTargets->setObjectName( "AgentDisplayTargets" );
    AgentDisplayerTargets->setProperty( "HcLabelDisplay", "true" );

    AgentDisplayerPivots = new QLabel( this );
    AgentDisplayerPivots->setObjectName( "AgentDisplayPivots" );
    AgentDisplayerPivots->setProperty( "HcLabelDisplay", "true" );

    AgentDisplayerElevated = new QLabel( this );
    AgentDisplayerElevated->setObjectName( "AgentDisplayElevated" );
    AgentDisplayerElevated->setProperty( "HcLabelDisplay", "true" );

    horizontalSpacer = new QSpacerItem( 40, 20, QSizePolicy::Expanding, QSizePolicy::Minimum );
    AgentActionMenu  = new QMenu( this );

    ActionPayload    = AgentActionMenu->addAction( "Payload Builder" );
    ActionShowHidden = AgentActionMenu->addAction( "Show Hidden" );
    ActionShowHidden->setCheckable( true );

    AgentActionButton = new QToolButton( this );
    AgentActionButton->setObjectName( "AgentActionButton" );
    AgentActionButton->setText( "Actions" );
    AgentActionButton->setProperty( "HcButton", "true" );
    AgentActionButton->setMenu( AgentActionMenu );
    AgentActionButton->setPopupMode( QToolButton::InstantPopup );

    Splitter->addWidget( AgentTable );
    Splitter->addWidget( AgentTab );
    Splitter->handle( 1 )->setEnabled( SplitterMoveToggle ); /* disabled by default */

    QObject::connect( AgentTable,       &QTableWidget::customContextMenuRequested, this, &HcPageAgent::handleAgentMenu );
    QObject::connect( AgentTable,       &QTableWidget::doubleClicked, this, &HcPageAgent::handleAgentDoubleClick );
    QObject::connect( AgentTab,         &QTabWidget::tabCloseRequested, this, &HcPageAgent::tabCloseRequested );
    QObject::connect( ActionPayload,    &QAction::triggered, this, &HcPageAgent::actionPayloadBuilder );
    QObject::connect( ActionShowHidden, &QAction::triggered, this, &HcPageAgent::actionShowHidden );

    gridLayout->addWidget( ComboAgentView,         0, 0, 1, 1 );
    gridLayout->addWidget( Splitter,               1, 0, 1, 7 );
    gridLayout->addWidget( AgentDisplayerSessions, 0, 2, 1, 1 );
    gridLayout->addWidget( AgentDisplayerTargets,  0, 1, 1, 1 );
    gridLayout->addWidget( AgentDisplayerPivots,   0, 3, 1, 1 );
    gridLayout->addWidget( AgentDisplayerElevated, 0, 4, 1, 1 );
    gridLayout->addItem( horizontalSpacer,         0, 5, 1, 1 );
    gridLayout->addWidget( AgentActionButton,      0, 6, 1, 1 );

    retranslateUi( );

    QMetaObject::connectSlotsByName( this );
}

HcPageAgent::~HcPageAgent() = default;

auto HcPageAgent::retranslateUi() -> void {
    setStyleSheet( Havoc->getStyleSheet() );

    AgentDisplayerElevated->setText( "Elevated: 0" );
    AgentDisplayerSessions->setText( "Sessions: 0" );
    AgentDisplayerTargets->setText( "Targets: 0" );
    AgentDisplayerPivots->setText( "Pivots: 0" );
    ComboAgentView->addItems( QStringList() << "Sessions" << "Sessions Graph" << "Targets" );
}

auto HcPageAgent::addTab(
    const QString& name,
    QWidget*       widget
) const -> void {
    if ( AgentTab->count() == 0 ) {
        Splitter->setSizes( QList<int>() << 200 << 220 );
        Splitter->handle( 1 )->setEnabled( true );
        Splitter->handle( 1 )->setCursor( Qt::SplitVCursor );
    }

    AgentTab->setCurrentIndex( AgentTab->addTab( widget, name ) );
}

inline auto HcTableWidget(
    const QString&     value,
    const Qt::ItemFlag flags = Qt::ItemIsEditable
) -> QTableWidgetItem* {
    auto item = new QTableWidgetItem( value );

    item->setTextAlignment( Qt::AlignCenter );
    item->setFlags( item->flags() ^ flags );

    return item;
}

auto HcPageAgent::addAgent(
    const json& metadata
) -> void {
    auto uuid    = QString();
    auto arch    = QString();
    auto user    = QString();
    auto host    = QString();
    auto local   = QString();
    auto path    = QString();
    auto process = QString();
    auto pid     = QString();
    auto tid     = QString();
    auto system  = QString();
    auto last    = QString();
    auto row     = AgentTable->rowCount();
    auto sort    = AgentTable->isSortingEnabled();

    //
    // TODO: sanity check
    //

    uuid    = QString( metadata[ "uuid" ].get<std::string>().c_str() );
    user    = QString( metadata[ "meta" ][ "user" ].get<std::string>().c_str() );
    arch    = QString( metadata[ "meta" ][ "arch" ].get<std::string>().c_str() );
    host    = QString( metadata[ "meta" ][ "host" ].get<std::string>().c_str() );
    local   = QString( metadata[ "meta" ][ "local ip" ].get<std::string>().c_str() );
    path    = QString( metadata[ "meta" ][ "process path" ].get<std::string>().c_str() );
    process = QString( metadata[ "meta" ][ "process name"].get<std::string>().c_str() );
    pid     = QString::number( metadata[ "meta" ][ "pid" ].get<int>() );
    tid     = QString::number( metadata[ "meta" ][ "tid" ].get<int>() );
    system  = QString( metadata[ "meta" ][ "system" ].get<std::string>().c_str() );
    last    = QString( metadata[ "meta" ][ "last callback" ].get<std::string>().c_str() );

    auto agent = new HcAgent {
        .uuid = uuid.toStdString(),
        .type = metadata[ "type" ].get<std::string>(),
        .data = metadata,
        .last = last,
        .ui   = {
            .Uuid        = HcTableWidget( uuid ),
            .Internal    = HcTableWidget( local ),
            .Username    = HcTableWidget( user ),
            .Hostname    = HcTableWidget( host ),
            .ProcessPath = HcTableWidget( path ),
            .ProcessName = HcTableWidget( process ),
            .ProcessId   = HcTableWidget( pid ),
            .ThreadId    = HcTableWidget( tid ),
            .Arch        = HcTableWidget( arch ),
            .System      = HcTableWidget( system ),
            .Note        = HcTableWidget( "", Qt::NoItemFlags ),
            .Last        = HcTableWidget( last ),
        }
    };

    agent->console = new HcAgentConsole( agent );
    agent->console->setBottomLabel( QString( "[User: %1] [Process: %2] [Pid: %3] [Tid: %4]" ).arg( user ).arg( path ).arg( pid ).arg( tid ) );
    agent->console->setInputLabel( ">>>" );
    agent->console->LabelHeader->setFixedHeight( 0 );

    //
    // connect signals and slots
    //
    QObject::connect( & agent->emitter, & HcAgentEmit::ConsoleWrite, this, []( const QString& uuid, const QString& text ) {
        auto agent = Havoc->Agent( uuid.toStdString() );

        if ( agent.has_value() ) {
            agent.value()->console->appendConsole( HcConsole::formatString( text.toStdString() ).c_str() );
        }
    } );

    agents.push_back( agent );

    AgentTable->setRowCount( row + 1 );
    AgentTable->setSortingEnabled( false );
    AgentTable->setItem( row, 0,  agent->ui.Uuid        );
    AgentTable->setItem( row, 1,  agent->ui.Internal    );
    AgentTable->setItem( row, 2,  agent->ui.Username    );
    AgentTable->setItem( row, 3,  agent->ui.Hostname    );
    AgentTable->setItem( row, 4,  agent->ui.ProcessName );
    AgentTable->setItem( row, 5,  agent->ui.ProcessId   );
    AgentTable->setItem( row, 6,  agent->ui.ThreadId    );
    AgentTable->setItem( row, 7,  agent->ui.Arch        );
    AgentTable->setItem( row, 8,  agent->ui.System      );
    AgentTable->setItem( row, 9,  agent->ui.Last        );
    AgentTable->setItem( row, 10, agent->ui.Note        );
    AgentTable->setSortingEnabled( sort );

    //
    // if an interface has been registered then assign it to the agent
    //
    agent->interface = std::nullopt;
    if ( auto interface = Havoc->AgentObject( agent->type ) ) {
        if ( interface.has_value() ) {
            py11::gil_scoped_acquire gil;

            try {
                agent->interface = interface.value()( agent->uuid, agent->type, metadata[ "meta" ] );
            } catch ( py11::error_already_set &eas ) {
                spdlog::error( "failed to invoke agent interface [uuid: {}] [type: {}]: {}", agent->uuid, agent->type, eas.what() );
            }
        }
    }

    AgentDisplayerTargets->setText( QString( "Targets: %1" ).arg( agents.size() ) ); /* TODO: all targets (only show one host) */
    AgentDisplayerSessions->setText( QString( "Sessions: %1" ).arg( agents.size() ) ); /* TODO: only set current alive beacons/sessions */
    AgentDisplayerPivots->setText( "Pivots: 0" );
    AgentDisplayerElevated->setText( "Elevated: 0" );
}

auto HcPageAgent::handleAgentMenu(
    const QPoint& pos
) -> void {
    auto menu = QMenu( this );
    auto uuid = std::string();

    /* check if we point to a session table item/agent */
    if ( ! AgentTable->itemAt( pos ) ) {
        return;
    }

    uuid = AgentTable->item( AgentTable->currentRow(), 0 )->text().toStdString();

    menu.addAction( "Interact" );

    if ( auto action = menu.exec( AgentTable->horizontalHeader()->viewport()->mapToGlobal( pos ) ) ) {
        if ( action->text().compare( "Interact" ) == 0 ) {
            spawnAgentConsole( uuid );
        } else {
            spdlog::debug( "[ERROR] invalid action from selected agent menu" );
        }
    }
}

auto HcPageAgent::handleAgentDoubleClick(
    const QModelIndex& index
) -> void {
    auto uuid = AgentTable->item( index.row(), 0 )->text();

    spawnAgentConsole( uuid.toStdString() );
}

auto HcPageAgent::spawnAgentConsole(
    const std::string& uuid
) -> void {
    for ( auto& agent : agents ) {
        if ( agent->uuid == uuid ) {
            auto user  = agent->data[ "meta" ][ "user" ].get<std::string>();
            auto title = QString( "[%1] %2" ).arg( uuid.c_str() ).arg( user.c_str() );

            //
            // check if we already have the agent console open.
            // if yes then just focus on the opened tab already
            //
            for ( int i = 0; i < AgentTab->count(); i++ ) {
                if ( AgentTab->widget( i ) == agent->console ) {
                    AgentTab->setCurrentIndex( i );
                    return;
                }
            }

            //
            // no tab with the title name found so lets just add a new one
            //
            addTab( title, agent->console );

            break;
        }
    }
}

auto HcPageAgent::tabCloseRequested(
    int index
) const -> void {
    if ( index == -1 ) {
        return;
    }

    AgentTab->removeTab( index );

    if ( AgentTab->count() == 0 ) {
        Splitter->setSizes( QList<int>() << 0 );
        Splitter->handle( 1 )->setEnabled( false );
        Splitter->handle( 1 )->setCursor( Qt::ArrowCursor );
    }
}

auto HcPageAgent::AgentConsole(
    const std::string& uuid,
    const std::string& format,
    const std::string& output
) -> void {
    auto agent = Agent( uuid );

    if ( agent.has_value() ) {
        //
        // now print the content
        //
        agent.value()->console->appendConsole( HcConsole::formatString( format, output ).c_str() );
    } else {
        spdlog::debug( "[AgentConsole] agent not found: {}", uuid );
    }
}

auto HcPageAgent::Agent(
    const std::string &uuid
) -> std::optional<HcAgent*> {
    for ( auto agent : agents ) {
        if ( agent->uuid == uuid ) {
            return agent;
        }
    }

    return std::nullopt;
}

auto HcPageAgent::actionShowHidden(
    bool checked
) -> void {

}

auto HcPageAgent::actionPayloadBuilder(
    bool checked
) -> void {
    auto gil    = py11::gil_scoped_acquire();
    auto dialog = HcDialogBuilder();

    QObject::connect( Havoc->Gui, &HcMainWindow::signalBuildLog, &dialog, &HcDialogBuilder::EventBuildLog );

    //
    // if there was an error while loading or executing
    // any scripts do not display the window
    //
    if ( ! dialog.ErrorReceived ) {
        dialog.exec();
    }
}

HcAgentConsole::HcAgentConsole(
    HcAgent* meta,
    QWidget* parent
) : HcConsole( parent ), Meta( meta ) {}

auto HcAgentConsole::inputEnter(
    void
) -> void {
    auto input = std::string();

    input = Input->text().toStdString();
    Input->clear();

    HcPythonReleaseGil();

    //
    // invoke the command in a separate thread
    //
    QtConcurrent::run( []( HcAgent* agent, const std::string& input ) {
        py11::gil_scoped_acquire gil;

        if ( agent->interface.has_value() ) {
            try {
                agent->interface.value().attr( "_input_dispatch" )( input );
            } catch ( py11::error_already_set &eas ) {
                emit agent->emitter.ConsoleWrite( agent->uuid.c_str(), eas.what() );
            }
        } else {
            emit agent->emitter.ConsoleWrite( agent->uuid.c_str(), "[!] No agent script handler registered for this type" );
        }
    }, Meta, input );
}
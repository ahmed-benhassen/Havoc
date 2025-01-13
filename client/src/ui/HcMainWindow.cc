#include <Havoc.h>
#include <ui/HcMainWindow.h>

/* Havoc side button hover over event. */
class Hover : public QObject
{
    QString* TabName = nullptr;

    void handleButtonSideEvent( const QString& Name, QPushButton* button, QObject* watched, QEvent* event ) {
        if ( watched == button ) {
            if ( event->type() == QEvent::HoverEnter ) {
                button->setIcon( QIcon( ":/icons/32px-" + Name + "-white" ) );
                button->setIconSize( QSize( 32, 32 ) );
            } else if ( event->type() == QEvent::HoverLeave ) {
                /* only apply icon if it's not used */
                if ( ! ( ( HavocButton* ) button )->currentlyUsed() ) {
                    button->setIcon( Helper::GrayScaleIcon( QImage( ":/icons/32px-" + Name + "-white" ) ) );
                    button->setIconSize( QSize( 32, 32 ) );
                }
            }
        }
    }

public:
    bool eventFilter( QObject* watched, QEvent* event ) override {
        handleButtonSideEvent( "connection", Havoc->ui->ButtonAgents,    watched, event );
        handleButtonSideEvent( "listener",   Havoc->ui->ButtonListeners, watched, event );
        handleButtonSideEvent( "hosting",    Havoc->ui->ButtonServer,    watched, event );
        handleButtonSideEvent( "script",     Havoc->ui->ButtonScripts,   watched, event );
        handleButtonSideEvent( "settings",   Havoc->ui->ButtonSettings,  watched, event );

        return false;
    }
};

HcMainWindow::HcMainWindow() {
    auto HoverEvent = new Hover;

    if ( objectName().isEmpty() ) {
        setObjectName( QString::fromUtf8( "HcMainWindow" ) );
    }

    resize( 1400, 800 );

    MainWidget = new QWidget( this );
    MainWidget->setObjectName( QString::fromUtf8( "MainWidget" ) );

    gridLayout = new QGridLayout( MainWidget );
    gridLayout->setObjectName( QString::fromUtf8("gridLayout" ) );

    SideBarWidget = new QWidget( MainWidget );
    SideBarWidget->setObjectName( QString::fromUtf8( "SideBarWidget" ) );

    gridLayout_2 = new QGridLayout( SideBarWidget );
    gridLayout_2->setObjectName( QString::fromUtf8( "gridLayout_2" ) );

    ButtonHavoc = new HavocButton( SideBarWidget );
    ButtonHavoc->setObjectName( QString::fromUtf8( "SideHavoc" ) );
    ButtonHavoc->setIcon( QIcon( ":/icons/havoc-white" ) );
    ButtonHavoc->setIconSize( QSize( 40, 40 ) );
    ButtonHavoc->setMinimumSize( QSize( 0, 50 ) );

    ButtonAgents = new HavocButton( SideBarWidget );
    ButtonAgents->setObjectName( QString::fromUtf8( "SideButtonAgents" ) );
    ButtonAgents->setMinimumSize( QSize( 0, 50 ) );
    ButtonAgents->setMinimumSize( QSize( 0, 50 ) );

    ButtonListeners = new HavocButton( SideBarWidget );
    ButtonListeners->setObjectName( QString::fromUtf8( "SideButtonListeners" ) );
    ButtonListeners->setMinimumSize( QSize( 0, 50 ) );

    ButtonServer = new HavocButton( SideBarWidget );
    ButtonServer->setObjectName( QString::fromUtf8( "SideButtonServer" ) );
    ButtonServer->setMinimumSize( QSize( 0, 50 ) );

    ButtonScripts = new HavocButton( SideBarWidget );
    ButtonScripts->setObjectName( QString::fromUtf8( "SideButtonScripts" ) );
    ButtonScripts->setMinimumSize( QSize( 0, 50 ) );

    ButtonSettings  = new HavocButton( SideBarWidget );
    ButtonSettings->setObjectName( QString::fromUtf8( "SideButtonSettings" ) );
    ButtonSettings->setMinimumSize( QSize( 0, 50 ) );
    ButtonSettings->setStyleSheet( "margin-bottom: 5px" );

    Spacer = new QSpacerItem( 20, 40, QSizePolicy::Minimum, QSizePolicy::Expanding );

    StackedWidget = new QStackedWidget( MainWidget );
    StackedWidget->setObjectName( QString::fromUtf8( "StackedWidget" ) );

    gridLayout_2->addWidget( ButtonHavoc,     0, 0, 1, 1 );
    gridLayout_2->addWidget( ButtonAgents,    1, 0, 1, 1 );
    gridLayout_2->addWidget( ButtonListeners, 2, 0, 1, 1 );
    gridLayout_2->addWidget( ButtonServer,    3, 0, 1, 1 );
    gridLayout_2->addWidget( ButtonScripts,   4, 0, 1, 1 );
    gridLayout_2->addItem( Spacer, 5, 0, 1, 1 );
    gridLayout_2->addWidget( ButtonSettings,  6, 0, 1, 1 );
    gridLayout_2->setContentsMargins( 0, 0, 0, 0 );

    gridLayout->addWidget( SideBarWidget, 0, 0, 1, 1 );
    gridLayout->addWidget( StackedWidget, 0, 1, 1, 1 );
    gridLayout->setContentsMargins( 0, 0, 0, 0 );

    /* create page objects */
    PageAgent    = new HcPageAgent;
    PageListener = new HcPageListener;
    PageScripts  = new HcPagePlugins;
    PageServer   = new QWidget;
    PageSettings = new QWidget;

    /* add stacked pages to the GUI */
    StackedWidget->addWidget( PageAgent );
    StackedWidget->addWidget( PageListener );
    StackedWidget->addWidget( PageServer );
    StackedWidget->addWidget( PageScripts );
    StackedWidget->addWidget( PageSettings );

    setCentralWidget( MainWidget );

    connect( ButtonAgents, &QPushButton::clicked, this, [&] () {
        switchPageAgent();
    } );

    connect( ButtonListeners, &QPushButton::clicked, this, [&] () {
        switchPageListener();
    } );

    connect( ButtonServer, &QPushButton::clicked, this, [&] () {
        switchPageServer();
    } );

    connect( ButtonScripts, &QPushButton::clicked, this, [&] () {
        switchPageScripts();
    } );

    connect( ButtonSettings, &QPushButton::clicked, this, [&] () {
        switchPageSettings();
    } );

    retranslateUi();
    unusedSideButtons();

    /* default button is the Agent */
    useSideButton( ButtonAgents, "connection" );

    /* Side panel stuff */
    ButtonAgents->installEventFilter( HoverEvent );
    ButtonListeners->installEventFilter( HoverEvent );
    ButtonServer->installEventFilter( HoverEvent );
    ButtonScripts->installEventFilter( HoverEvent );
    ButtonSettings->installEventFilter( HoverEvent );

    QMetaObject::connectSlotsByName( this );
}

auto HcMainWindow::retranslateUi() -> void {
    setWindowTitle( QString( "Havoc [Version: %1 %2]" ).arg( HAVOC_VERSION ).arg( HAVOC_CODENAME ) );

}

auto HcMainWindow::renderWindow() -> void {
    show();
}

auto HcMainWindow::unusedSideButtons() -> void {
    ButtonAgents->setIcon( Helper::GrayScaleIcon( QImage( ":/icons/32px-connection-white" ) ) );
    ButtonAgents->setIconSize( QSize( 32, 32 ) );
    ButtonAgents->setToolTip( "Agents" );
    ButtonAgents->setUsed( false );

    ButtonListeners->setIcon( Helper::GrayScaleIcon( QImage( ":/icons/32px-listener-white" ) ) );
    ButtonListeners->setIconSize( QSize( 32, 32 ) );
    ButtonListeners->setToolTip( "Listeners" );
    ButtonListeners->setUsed( false );

    ButtonServer->setIcon( Helper::GrayScaleIcon( QImage( ":/icons/32px-hosting-white" ) ) );
    ButtonServer->setIconSize( QSize( 32, 32 ) );
    ButtonServer->setToolTip( "Server" );
    ButtonServer->setUsed( false );

    ButtonScripts->setIcon( Helper::GrayScaleIcon( QImage( ":/icons/32px-script-white" ) ) );
    ButtonScripts->setIconSize( QSize( 32, 32 ) );
    ButtonScripts->setToolTip( "Scripts" );
    ButtonScripts->setUsed( false );

    ButtonSettings->setIcon( Helper::GrayScaleIcon( QImage( ":/icons/32px-settings-white" ) ) );
    ButtonSettings->setIconSize( QSize( 32, 32 ) );
    ButtonSettings->setToolTip( "Settings" );
    ButtonSettings->setUsed( false );
}

auto HcMainWindow::useSideButton(
    HavocButton*   button,
    const QString& name
) -> void {
    unusedSideButtons();

    button->setUsed( true );
    button->setIcon( QIcon( ":/icons/32px-" + name + "-white" ) );
    button->setIconSize( QSize( 32, 32 ) );
}

auto HcMainWindow::switchPageAgent() -> void {
    useSideButton( ButtonAgents, "connection" );
    StackedWidget->setCurrentIndex( 0 );
}

auto HcMainWindow::switchPageListener() -> void {
    useSideButton( ButtonListeners, "listener" );
    StackedWidget->setCurrentIndex( 1 );
}

auto HcMainWindow::switchPageServer() -> void {
    useSideButton( ButtonServer, "hosting" );
    StackedWidget->setCurrentIndex( 2 );
}

auto HcMainWindow::switchPageScripts() -> void {
    useSideButton( ButtonScripts, "script" );
    StackedWidget->setCurrentIndex( 3 );
}

auto HcMainWindow::switchPageSettings() -> void {
    useSideButton( ButtonSettings, "settings" );
    StackedWidget->setCurrentIndex( 4 );
}

auto HcMainWindow::MessageBox(
    QMessageBox::Icon  icon,
    const std::string& title,
    const std::string& text
) -> void {
    auto message = QMessageBox();

    message.setStyleSheet( HcApplication::StyleSheet() );
    message.setWindowTitle( title.c_str() );
    message.setText( text.c_str() );
    message.setIcon( icon );

    message.exec();
}

auto HcMainWindow::AddListener(
    const json& listener
) const -> void {
    auto name   = std::string();
    auto status = std::string();

    if ( listener.empty() ) {
        spdlog::error( "HcMainWindow::AddListener: invalid package (data emtpy)" );
        return;
    }

    if ( listener.contains( "name" ) ) {
        if ( listener[ "name" ].is_string() ) {
            name = listener[ "name" ].get<std::string>();
        } else {
            spdlog::error( "invalid listener: \"name\" is not string" );
            return;
        }
    } else {
        spdlog::error( "invalid listener: \"name\" is not found" );
        return;
    }

    if ( listener.contains( "status" ) ) {
        if ( listener[ "status" ].is_string() ) {
            status = listener[ "status" ].get<std::string>().c_str();
        } else {
            spdlog::error( "invalid listener: \"status\" is not string" );
            return;
        }
    } else {
        spdlog::error( "invalid listener: \"status\" is not found" );
        return;
    }

    if ( Havoc->ListenerObject( name ).has_value() ) {
        //
        // if listener already exists then change
        // the status instead to being started or available
        //
        PageListener->setListenerStatus( name, status );
    } else {
        Havoc->AddListener( listener );
    }
}

auto HcMainWindow::AddAgent(
    const json& metadata
) -> std::optional<HcAgent*> {
    const auto agent = new HcAgent( metadata );

    agent_mutex.lock();

    if ( !agent->initialize() ) {
        spdlog::debug( "[HcPageAgent::addAgent] failed to initialize agent" );
        agent_mutex.unlock();
        return std::nullopt;
    }

    PageAgent->addAgent( agent );

    //
    // post-processing of the agent
    agent->post();

    agent_mutex.unlock();

    return agent;
}

auto HcMainWindow::RemoveAgent(
    const std::string& uuid
) -> void {
    PageAgent->removeAgent( uuid );
}

auto HcMainWindow::AgentConsole(
    const json& console
) -> void {
    auto uuid   = std::string();
    auto format = std::string();
    auto output = std::string();
    auto data   = json();

    if ( console.empty() ) {
        spdlog::error( "[AgentConsole] invalid package (data emtpy)" );
        return;
    }

    if ( console.contains( "uuid" ) ) {
        if ( console[ "uuid" ].is_string() ) {
            uuid = console[ "uuid" ].get<std::string>();
        } else {
            spdlog::error( "invalid agent console: \"uuid\" is not string" );
            return;
        }
    } else {
        spdlog::error( "invalid agent console: \"uuid\" is not found" );
        return;
    }

    if ( console.contains( "data" ) ) {
        if ( console[ "data" ].is_object() ) {
            data = console[ "data" ].get<json>();
        } else {
            spdlog::error( "invalid agent console: \"data\" is not an object" );
            return;
        }
    } else {
        spdlog::error( "invalid agent console: \"data\" is not found" );
        return;
    }

    if ( data.contains( "format" ) ) {
        if ( data[ "format" ].is_string() ) {
            format = data[ "format" ].get<std::string>();
        } else {
            spdlog::error( "invalid agent console: \"format\" is not string" );
            return;
        }
    } else {
        spdlog::error( "invalid agent console: \"format\" is not found" );
        return;
    }

    if ( data.contains( "output" ) ) {
        if ( data[ "output" ].is_string() ) {
            output = data[ "output" ].get<std::string>();
        } else {
            spdlog::error( "invalid agent console: \"output\" is not string" );
            return;
        }
    } else {
        spdlog::error( "invalid agent console: \"output\" is not found" );
        return;
    }

    PageAgent->AgentConsole( uuid, format, output );
}

auto HcMainWindow::closeEvent(
    QCloseEvent* event
) -> void {
    auto question = QMessageBox( this );

    question.setIcon( QMessageBox::Question );
    question.setWindowTitle( "Close Confirmation" );
    question.setText( "Are you sure you want to exit?" );
    question.setStandardButtons( QMessageBox::Yes | QMessageBox::No );
    question.setDefaultButton( QMessageBox::Yes );
    question.setStyleSheet( HcApplication::StyleSheet() );

    if ( question.exec() == QMessageBox::Yes ) {
        HcApplication::Exit();
        return;
    }

    event->ignore();
}

HavocButton::HavocButton(
    QWidget *w
) : QPushButton( w ) {
    setFocusPolicy( Qt::NoFocus );
}

auto HavocButton::setUsed(
    bool state
) -> void {
    buttonUsed = state;
    setProperty( "HcButtonUsed", buttonUsed ? "true" : "false" );
    style()->unpolish( this );
    style()->polish( this );
}

auto HavocButton::currentlyUsed() const -> bool { return buttonUsed; }

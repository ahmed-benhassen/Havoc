#include <Havoc.h>
#include <ui/HcPageAgent.h>
#include <QtConcurrent/QtConcurrent>
#include <DockManager.h>

#include "DockAreaWidget.h"

HcPageAgent::HcPageAgent(
    QWidget* parent
) : QWidget( parent ) {
    if ( objectName().isEmpty() ) {
        setObjectName( "PageAgent" );
    }

    gridLayout = new QGridLayout( this );
    gridLayout->setObjectName( "gridLayout" );

    DockManager = new ads::CDockManager( this );
    DockManager->setStyleSheet( HcApplication::StyleSheet() );

    ads::CDockManager::setConfigFlag( ads::CDockManager::AlwaysShowTabs, false );
    ads::CDockManager::setConfigFlag( ads::CDockManager::DefaultDockAreaButtons, false );
    ads::CDockManager::setConfigFlag( ads::CDockManager::ActiveTabHasCloseButton, false );
    ads::CDockManager::setConfigFlag( ads::CDockManager::AllTabsHaveCloseButton, true );
    ads::CDockManager::setConfigFlag( ads::CDockManager::DockAreaDynamicTabsMenuButtonVisibility, true );
    ads::CDockManager::setConfigFlag( ads::CDockManager::FloatingContainerHasWidgetIcon, false );
    ads::CDockManager::setConfigFlag( ads::CDockManager::DisableTabTextEliding, true );

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

    AgentTable = new QTableWidget( this );
    AgentTable->setObjectName( "AgentTable" );
    AgentTable->setColumnCount( 11 );

    AgentTable->setIconSize( QSize( 25, 25 ) );

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
    AgentTable->horizontalHeader()->setHighlightSections( false );
    AgentTable->verticalHeader()->setVisible( false );
    AgentTable->setFocusPolicy( Qt::NoFocus );
    AgentTable->setAlternatingRowColors( true );

    AgentGraph = new HcSessionGraph( this );
    AgentGraph->setObjectName( "AgentGraph" );

    auto graph = new ads::CDockWidget( "Session Graph" );
    auto table = new ads::CDockWidget( "Session Table" );
    auto ghost = new ads::CDockWidget( "Ghost Widget"  );

    graph->setWidget( AgentGraph );
    graph->setFeatures( ads::CDockWidget::DockWidgetMovable );
    graph->setIcon( QIcon( ":/icons/32px-network" ) );

    table->setWidget( AgentTable );
    table->setFeatures( ads::CDockWidget::DockWidgetMovable );
    table->setIcon( QIcon( ":/icons/32px-table-list" ) );

    //
    // this is just a "ghost" widget. It is meant to be added
    // first on the bottom dock widget area where in future the
    // agent console are going to be places as well.
    ghost->setWidget( new QWidget );

    auto area         = DockManager->addDockWidget( ads::TopDockWidgetArea,    graph );
    ConsoleAreaWidget = DockManager->addDockWidget( ads::BottomDockWidgetArea, ghost );
    DockManager->addDockWidgetTabToArea( table, area );

    //
    // turn it invisible. it is not meant
    // to be seen or interacted with
    ghost->toggleView( false );

    AgentDisplayerSessions = new QLabel( this );
    AgentDisplayerSessions->setObjectName( "LabelDisplaySessions" );
    AgentDisplayerSessions->setProperty( "HcLabelDisplay", "true" );

    AgentDisplayerPivots = new QLabel( this );
    AgentDisplayerPivots->setObjectName( "AgentDisplayPivots" );
    AgentDisplayerPivots->setProperty( "HcLabelDisplay", "true" );

    AgentDisplayerElevated = new QLabel( this );
    AgentDisplayerElevated->setObjectName( "AgentDisplayElevated" );
    AgentDisplayerElevated->setProperty( "HcLabelDisplay", "true" );

    horizontalSpacer = new QSpacerItem( 40, 20, QSizePolicy::Expanding, QSizePolicy::Minimum );

    AgentActionMenu  = new QMenu( this );
    ActionPayload    = AgentActionMenu->addAction( QIcon( ":/icons/16px-payload-build" ), "Payload Builder" );
    ActionShowHidden = AgentActionMenu->addAction( QIcon( ":/icons/32px-blind-white" ), "Show Hidden" );

    AgentActionButton = new QToolButton( this );
    AgentActionButton->setObjectName( "AgentActionButton" );
    AgentActionButton->setToolButtonStyle( Qt::ToolButtonTextBesideIcon );
    AgentActionButton->setText( "Actions" );
    AgentActionButton->setProperty( "HcButton", "true" );
    AgentActionButton->setMenu( AgentActionMenu );
    AgentActionButton->setIcon( QIcon( ":/icons/32px-flash" ) );
    AgentActionButton->setPopupMode( QToolButton::InstantPopup );
    AgentActionButton->setLayoutDirection( Qt::LeftToRight );
    AgentActionButton->setMaximumWidth( 90 );

    connect( AgentTable, &QTableWidget::customContextMenuRequested, this, &HcPageAgent::handleAgentMenu );
    connect( AgentTable, &QTableWidget::doubleClicked, this, &HcPageAgent::handleAgentDoubleClick );
    connect( ActionPayload, &QAction::triggered, this, &HcPageAgent::actionPayloadBuilder );
    connect( ActionShowHidden, &QAction::triggered, this, &HcPageAgent::actionShowHidden );
    connect( AgentActionButton, &QToolButton::triggered, this, &HcPageAgent::actionTriggered );
    connect( AgentTable, &QTableWidget::itemChanged, this, &HcPageAgent::itemChanged );

    gridLayout->addWidget( AgentDisplayerSessions, 0, 0, 1, 1 );
    gridLayout->addWidget( AgentDisplayerPivots,   0, 1, 1, 1 );
    gridLayout->addWidget( AgentDisplayerElevated, 0, 2, 1, 1 );
    gridLayout->addItem( horizontalSpacer,         0, 3, 1, 1 );
    gridLayout->addWidget( AgentActionButton,      0, 4, 1, 1 );
    gridLayout->addWidget( DockManager,            1, 0, 1, 7 );

    retranslateUi();

    QMetaObject::connectSlotsByName( this );
}

HcPageAgent::~HcPageAgent() = default;

auto HcPageAgent::retranslateUi() -> void {
    setStyleSheet( Havoc->StyleSheet() );

    AgentDisplayerElevated->setText( "Elevated: 0" );
    AgentDisplayerSessions->setText( "Sessions: 0" );
    AgentDisplayerPivots->setText( "Pivots: 0" );
}

auto HcPageAgent::addTab(
    const QString& name,
    const QIcon&   icon,
    QWidget*       widget
) -> void {
    //
    // first try to find the widget and
    // if found try to make it reappear
    for ( const auto dock : DockManager->dockWidgetsMap() ) {
        if ( dock->widget() == widget ) {
            dock->toggleView( true );
            return;
        }
    }

    //
    // if not found then create a new widget
    // and add it to the bottom dock area
    //

    const auto tab = new ads::CDockWidget( name );

    tab->setWidget( widget );
    tab->setIcon( icon );
    tab->setFeatures(
        ads::CDockWidget::DockWidgetMovable   |
        ads::CDockWidget::DockWidgetFloatable |
        ads::CDockWidget::DockWidgetFocusable |
        ads::CDockWidget::DockWidgetClosable
    );

    DockManager->addDockWidgetTabToArea( tab, ConsoleAreaWidget );
}

auto HcPageAgent::addTab(
    const QString& name,
    QWidget*       widget
) -> void {
    addTab( name, QIcon( ":/icons/32px-agent-console" ), widget );
}

HcAgentTableItem::HcAgentTableItem(
    const QString&          value,
    HcAgent*                agent,
    const Qt::ItemFlag      flags,
    const Qt::AlignmentFlag align
)  : agent( agent ) {
    setText( value );
    setToolTip( value );
    setTextAlignment( align );
    setFlags( this->flags() ^ flags );
}

auto HcPageAgent::addAgent(
    HcAgent* agent
) -> void {
    const auto row  = AgentTable->rowCount();
    const auto sort = AgentTable->isSortingEnabled();

    //
    // bind the initialized agent to
    // the console write signals
    //

    connect( &agent->ui.signal, &HcAgentSignals::ConsoleWrite, this, []( const QString& uuid, const QString& text ) {
        if ( const auto _agent = Havoc->Agent( uuid.toStdString() ); _agent.has_value() ) {
            _agent.value()->console()->appendConsole( HcConsole::formatString( text.toStdString() ).c_str() );
        }
    } );

    connect( &agent->ui.signal, &HcAgentSignals::RegisterIcon, this, []( const QString& uuid, const QImage& icon ) {
        if ( const auto _agent = Havoc->Agent( uuid.toStdString() ); _agent.has_value() ) {
            _agent.value()->setImage( icon );
        }
    } );

    connect( &agent->ui.signal, &HcAgentSignals::RegisterIconName, this, []( const QString& uuid, const QString& name ) {
        if ( const auto _agent = Havoc->Agent( uuid.toStdString() ); _agent.has_value() ) {
            _agent.value()->setImage( QImage( std::format( ":/graph/{}", name.toStdString() ).c_str() ) );
        }
    } );

    agents.push_back( agent );

    AgentTable->setRowCount( row + 1 );
    AgentTable->setSortingEnabled( false );
    AgentTable->setItem( row, 0,  agent->ui.table.Uuid        );
    AgentTable->setItem( row, 1,  agent->ui.table.Internal    );
    AgentTable->setItem( row, 2,  agent->ui.table.Username    );
    AgentTable->setItem( row, 3,  agent->ui.table.Hostname    );
    AgentTable->setItem( row, 4,  agent->ui.table.ProcessName );
    AgentTable->setItem( row, 5,  agent->ui.table.ProcessId   );
    AgentTable->setItem( row, 6,  agent->ui.table.ThreadId    );
    AgentTable->setItem( row, 7,  agent->ui.table.Arch        );
    AgentTable->setItem( row, 8,  agent->ui.table.System      );
    AgentTable->setItem( row, 9,  agent->ui.table.Last        );
    AgentTable->setItem( row, 10, agent->ui.table.Note        );
    AgentTable->setSortingEnabled( sort );

    agent->ui.node = AgentGraph->addAgent( agent );

    if ( !agent->parent().empty() ) {
        pivots++;
    }

    if ( agent->data().contains( "elevated" ) && agent->data()[ "elevated" ].is_boolean() ) {
        if ( agent->data()[ "elevated" ].get<bool>() ) {
            elevated++;
        }
    }

    AgentDisplayerSessions->setText( QString( "Sessions: %1" ).arg( agents.size() ) );
    AgentDisplayerPivots->setText( QString( "Pivots: %1" ).arg( pivots ) );
    AgentDisplayerElevated->setText( QString( "Elevated: %1" ).arg( elevated ) );
}

auto HcPageAgent::handleAgentMenu(
    const QPoint& pos
) -> void {
    auto menu          = QMenu( this );
    auto uuid          = std::string();
    auto type          = std::string();
    auto agent         = std::optional<HcAgent*>();
    auto selections    = AgentTable->selectionModel()->selectedRows();
    auto agent_actions = Havoc->Actions( HcApplication::ActionObject::ActionAgent );
    auto remove        = std::vector<HcAgent*>();

    /* check if we point to a session table item/agent */
    if ( !AgentTable->itemAt( pos ) ) {
        return;
    }

    if ( selections.isEmpty() ) {
        return;
    }

    if ( selections.count() > 1 ) {
        //
        // for now we dont allow custom actions for multiple sessions
        // as we have to take the following things into consideration:
        //  - agent type
        //  - add action to the menu while other agent types are selected as well
        //  - check every selected row for agent type and add only the selected action etc.
        //
        // TODO: while this is surely possible i am going
        //       to move this into the future to implement.
        //
        menu.addAction( QIcon( ":/icons/16px-agent-console" ), "Interact" );
        menu.addSeparator();
        menu.addAction(
            !show_hidden ? QIcon( ":/icons/16px-blind-white" ) : QIcon( ":/icons/16px-eye-white" ),
            !show_hidden ? "Hide" : "Un-Hide"
        );
        menu.addAction( QIcon( ":/icons/16px-remove" ), "Remove" );
    } else {
        //
        // if a single selected agent item then try
        // to add the registered actions as well
        //
        menu.addAction( QIcon( ":/icons/16px-agent-console" ), "Interact" );
        menu.addSeparator();

        //
        // add the registered agent type actions
        //
        uuid  = AgentTable->item( selections.at( 0 ).row(), 0 )->text().toStdString();
        agent = Agent( uuid );

        if ( !agent.has_value() ) {
            return;
        }

        type = agent.value()->type();

        for ( const auto action : agent_actions ) {
            if ( action->agent.type == type ) {
                if ( action->icon.isNull() ) {
                    menu.addAction( QString::fromStdString( action->name ) );
                } else {
                    menu.addAction( action->icon, QString::fromStdString( action->name ) );
                }
            }
        }

        menu.addSeparator();
        menu.addAction(
            !agent.value()->hidden() ? QIcon( ":/icons/16px-blind-white" ) : QIcon( ":/icons/16px-eye-white" ),
            !agent.value()->hidden() ? "Hide" : "Un-Hide"
        );
        menu.addAction( QIcon( ":/icons/16px-remove" ), "Remove" );
    }

    if ( auto action = menu.exec( AgentTable->horizontalHeader()->viewport()->mapToGlobal( pos ) ) ) {
        for ( const auto& selected : selections ) {
            uuid  = AgentTable->item( selected.row(), 0 )->text().toStdString();
            agent = Agent( uuid );

            if ( action->text().compare( "Interact" ) == 0 ) {
                spawnAgentConsole( uuid );
            } else if ( action->text().compare( "Remove" ) == 0 ) {
                remove.push_back( Agent( uuid ).value() );
            } else if ( action->text().compare( "Hide" ) == 0 ) {
                agent.value()->setHidden( true );
            } else if ( action->text().compare( "Un-Hide" ) == 0 ) {
                agent.value()->setHidden( false );
            } else {
                for ( const auto _action : agent_actions ) {
                    if ( _action->name       == action->text().toStdString() &&
                         _action->agent.type == type
                    ) {
                        if ( agent.has_value() && agent.value()->interface().has_value() ) {
                            if ( _action->callback ) {
                                reinterpret_cast<HcFnCallbackCtx<std::string>>( _action->callback )(
                                    agent.value()->uuid()
                                );
                            } else {
                                try {
                                    HcPythonAcquire();

                                    _action->callback_py( agent.value()->interface().value() );
                                } catch ( py11::error_already_set& e ) {
                                    spdlog::error( "failed to execute action callback: {}", e.what() );
                                }
                            }
                        }
                        return;
                    }
                }

                spdlog::debug( "[ERROR] invalid action from selected agent menu" );
            }
        }
    }

    //
    // now remove all the agents that
    // have been specified to remove
    //

    for ( const auto& _agent : remove ) {
        _agent->remove();
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
        if ( agent->uuid() == uuid ) {
            const auto user  = agent->data()[ "meta" ][ "user" ].get<std::string>();
            const auto title = QString( "[%1] %2" ).arg( uuid.c_str() ).arg( user.c_str() );

            //
            // check if we already have the agent console open.
            // if yes then just focus on the opened tab already
            //
            // for ( int i = 0; i < AgentTab->count(); i++ ) {
            //     if ( AgentTab->widget( i ) == agent->console ) {
            //         AgentTab->setCurrentIndex( i );
            //         return;
            //     }
            // }

            //
            // no tab with the title name found so lets just add a new one
            //
            addTab( title, agent->console() );

            break;
        }
    }
}

auto HcPageAgent::AgentConsole(
    const std::string& uuid,
    const std::string& format,
    const std::string& output
) -> void {
    if ( const auto agent = Agent( uuid ); agent.has_value() ) {
        //
        // now print the content
        //
        agent.value()->console()->appendConsole( HcConsole::formatString( format, output ).c_str() );
    } else {
        spdlog::debug( "[AgentConsole] agent not found: {}", uuid );
    }
}

auto HcPageAgent::Agent(
    const std::string &uuid
) -> std::optional<HcAgent*> {
    for ( auto agent : agents ) {
        if ( agent->uuid() == uuid ) {
            return agent;
        }
    }

    return std::nullopt;
}

auto HcPageAgent::actionShowHidden(
    bool checked
) -> void {
    ActionShowHidden->setText( show_hidden ? "Show Hidden" : "Show All" );
    ActionShowHidden->setIcon( show_hidden ? QIcon( ":/icons/32px-blind-white" ) : QIcon( ":/icons/32px-eye-white" ) );

    show_hidden = !show_hidden;

    for ( const auto agent : agents ) {
        if ( agent->hidden() ) {
            if ( show_hidden ) {
                agent->unhide();
            } else {
                agent->hide();
            }
        }
    }
}

auto HcPageAgent::actionPayloadBuilder(
    bool checked
) -> void {
    HcPythonAcquire();

    auto dialog = HcPayloadBuild();

    connect( Havoc->ui, &HcMainWindow::signalBuildLog, &dialog, &HcPayloadBuild::EventBuildLog );

    //
    // if there was an error while loading or executing
    // any scripts do not display the window
    //
    if ( !dialog.ErrorReceived ) {
        dialog.exec();
    }
}

auto HcPageAgent::itemChanged(
    QTableWidgetItem *item
) -> void {
    auto agent_item = dynamic_cast<HcAgentTableItem*>( item );

    //
    // check if it is the agent item is equal to the note widget
    //
    if ( agent_item == agent_item->agent->ui.table.Note ) {
        //
        // check if is the note item getting created
        //
        if ( agent_item->ignore ) {
            agent_item->ignore = false;
            return;
        }

        auto note               = agent_item->text().toStdString();
        auto [status, response] = Havoc->ApiSend( "api/agent/note", {
            { "uuid", agent_item->agent->uuid() },
            { "note", note }
        } );

        if ( status != 200 ) {
            spdlog::error( "failed to set the agent {} note: ({}) {}", agent_item->agent->uuid(), status, response );
        }
    }
}

auto HcPageAgent::actionTriggered(
    QAction* triggered
) -> void {
    for ( auto action : Havoc->Actions( HcApplication::ActionObject::ActionHavoc ) ) {
        if ( action->name == triggered->text().toStdString() ) {

            try {
                HcPythonAcquire();

                action->callback_py().cast<void>();
            } catch ( py11::error_already_set& e ) {
                spdlog::error( "failed to execute action callback: {}", e.what() );
            }

            break;
        }
    }
}

auto HcPageAgent::removeAgent(
    const std::string& uuid
) -> void {
    HcAgent* agent       = {};
    auto     item_uuid   = std::string();
    auto     is_pivot    = false;
    auto     is_elevated = false;

    //
    // remove the agent from
    // the table ui widget entry
    //
    for ( int i = 0; i < AgentTable->rowCount(); i++ ) {
        item_uuid = static_cast<HcAgentTableItem*>( AgentTable->item( i, 0 ) )->agent->uuid();

        if ( item_uuid == uuid ) {
            spdlog::debug( "remove agent from table: {}", uuid );
            AgentTable->removeRow( i );
            break;
        }
    }

    //
    // remove the agent entry from
    // the table entry vector list
    //
    for ( int i = 0; i < agents.size(); i++ ) {
        if ( agents[ i ]->uuid() == item_uuid ) {
            agent = agents[ i ];

            agents.erase( agents.begin() + i );
            break;
        }
    }

    if ( agent ) {
        HcPythonAcquire();

        is_pivot = !agent->parent().empty();

        if ( agent->data().contains( "elevated" ) && agent->data()[ "elevated" ].is_boolean() ) {
            is_elevated = agent->data()[ "elevated" ].get<bool>();
        }

        for ( const auto dock : DockManager->dockWidgetsMap() ) {
            if ( dock->widget() == agent->console() ) {
                DockManager->removeDockWidget( dock );

                //
                // TODO: looks like this causes the client to crash. investigate!!
                //
                // _dock->closeDockWidget();
                // delete _dock;
                break;
            }
        }

        AgentGraph->removeAgent( agent );

        spdlog::debug( "agent delete objects" );

        delete agent->console();
        delete agent;
    }

    AgentDisplayerSessions->setText( QString( "Sessions: %1" ).arg( agents.size() ) );
    AgentDisplayerPivots->setText( QString( "Pivots: %1" ).arg( is_pivot ? --pivots : pivots ) );
    AgentDisplayerPivots->setText( QString( "Elevated: %1" ).arg( is_elevated ? --elevated : elevated ) );
}

HcDescriptionDelegate::HcDescriptionDelegate(
    QCompleter* completer,
    QObject*    parent
) : QStyledItemDelegate( parent ), completer( completer ) {}

auto HcDescriptionDelegate::paint(
    QPainter*                   painter,
    const QStyleOptionViewItem& option,
    const QModelIndex&          index
) const -> void {
    //
    // TODO: highlight the typed out message inside of the popup
    //
    // auto mainText      = index.data(Qt::DisplayRole).toString();
    // auto description   = index.data(Qt::ToolTipRole).toString();
    // auto typedText     = completer->completionPrefix();
    // auto prefixLength  = typedText.length();
    // auto matchingPart  = mainText.left(prefixLength);
    // auto remainingPart = mainText.mid(prefixLength);
    //
    // painter->save();
    //
    // // Set the option rect height for better visual
    // QRect rect = option.rect; // Get the rectangle
    // // rect.setHeight(rect.height() + 20); // Add extra height to prevent clipping
    //
    // // Preserve the font used in the option
    // painter->setFont(option.font); // Set the font to the same as the option
    //
    // // Calculate the widths for matching and remaining parts
    // int matchingWidth = option.fontMetrics.horizontalAdvance(matchingPart);
    // int remainingWidth = option.fontMetrics.horizontalAdvance( remainingPart + " " + description ); // Include description in width
    //
    // // If there is no matching part, ensure we still draw the remaining text
    // if (matchingPart.isEmpty()) {
    //     painter->setPen(Havoc->Theme.getForeground());
    //     painter->drawText(rect, Qt::AlignLeft | Qt::AlignVCenter, remainingPart + " " + description);
    // } else {
    //     // Draw the matching part
    //     painter->setPen(Havoc->Theme.getPurple());
    //     painter->drawText(rect.left(), rect.top(), matchingPart); // Align with top
    //
    //     // Draw the remaining part
    //     painter->setPen(Havoc->Theme.getForeground());
    //     // Adjust the left position for the remaining part
    //     int remainingLeft = rect.left() + matchingWidth;
    //     painter->drawText(remainingLeft, rect.top(), remainingPart + " " + description); // Align with top
    // }
    //
    // painter->restore();

    auto mainText        = index.data( Qt::DisplayRole ).toString();
    auto description     = index.data( Qt::ToolTipRole ).toString();
    auto descriptionRect = option.rect;

    painter->save();
    painter->drawText( option.rect, Qt::AlignLeft, mainText );

    descriptionRect.setLeft( option.rect.left() + 200 );
    painter->drawText( descriptionRect, Qt::AlignLeft, description );

    painter->restore();
}

HcAgentLineEdit::HcAgentLineEdit(
    QWidget* parent
) : QLineEdit( parent ), m_completerShown( false ) {
    setObjectName( "HcAgentLineEdit" );
    setProperty( "HcAgentLineEdit", true );

    m_timer = new QTimer( this );
    m_timer->setSingleShot( true );

    updatePrefixPosition();
    updateMargins();

    connect( m_timer, &QTimer::timeout,       this, &HcAgentLineEdit::showCompleter );
    connect( this,    &QLineEdit::textEdited, this, &HcAgentLineEdit::onTextEdited );
}

auto HcAgentLineEdit::setPrefixLabel(
    QLabel* label
) -> void {
    m_label = label;
    m_label->setProperty( "HcAgentLineEdit", true );

    updatePrefixPosition();
    updateMargins();
}

auto HcAgentLineEdit::CursorRect(
    void
) const -> QRect {
    return cursorRect();
}

auto HcAgentLineEdit::onTextEdited(
    const QString& text
) -> void {
    updatePrefixPosition();
    updateMargins();

    m_completerShown = false;
    m_timer->start( 0 );
}

auto HcAgentLineEdit::showCompleter(
    void
) -> void {
    updatePrefixPosition();
    updateMargins();

    if ( completer() && ! m_completerShown && ! text().isEmpty() ) {
        auto cr = CursorRect();
        cr.setWidth( completer()->popup()->sizeHintForColumn( 0 ) +
                     completer()->popup()->verticalScrollBar()->sizeHint().width() );
        completer()->complete( cr );

        auto popupRect = completer()->popup()->geometry();
        popupRect.moveBottom( mapToGlobal( QPoint( 0, 0 ) ).y() );
        completer()->popup()->setGeometry( popupRect );

        m_completerShown = true;
    }
}

auto HcAgentLineEdit::resizeEvent(
    QResizeEvent* event
) -> void {
    updatePrefixPosition();
    updateMargins();

    QWidget::resizeEvent( event );
}

auto HcAgentLineEdit::updatePrefixPosition(
    void
) const -> void {
    if ( m_label ) {
        m_label->move( 10, ( height() - m_label->height() ) / 2 );
    }
}

auto HcAgentLineEdit::updateMargins(
    void
) -> void {
    if ( m_label ) {
        setTextMargins( m_label->fontMetrics().horizontalAdvance( m_label->text() ) + 5, 0, 0, 0 );
    }
}

HcAgentCompleter::HcAgentCompleter(
    QObject* parent
) : QCompleter( parent ) {
    _model = new QStandardItemModel( this );

    setModel( _model );

    popup()->setObjectName( "HcAgentCompleter" );
    popup()->setStyleSheet( HcApplication::StyleSheet() );
    popup()->setItemDelegate( new HcDescriptionDelegate( this, popup() ) );
    popup()->installEventFilter( this );

    auto current = popup()->font();
    auto size    = static_cast<int>( current.pointSize() * 1.10 );

    current.setPointSize( size );
    popup()->setFont( current );
}

HcAgentCompleter::~HcAgentCompleter() {}

auto HcAgentCompleter::eventFilter(
    QObject* parent,
    QEvent*  event
) -> bool {
    if ( event->type() == QEvent::KeyPress ) {
        const auto keyEvent = dynamic_cast<QKeyEvent*>( event );

        if ( keyEvent->key() == Qt::Key_Tab ) {
            auto completions = getCurrentCompletions();

            if ( completions.size() == 1 ) {
                if ( const auto edit = dynamic_cast<HcAgentLineEdit*>( widget() ) ) {
                    edit->setText( completions[ 0 ] );
                    edit->setCursorPosition( static_cast<int>( completions[ 0 ].length() ) );
                    popup()->hide();
                    return true;
                }
            } else if ( completions.size() > 1 ) {
                const auto commonPrefix = findLongestCommonPrefix( completions );

                if ( ! commonPrefix.isEmpty() ) {
                    if ( const auto edit = dynamic_cast<HcAgentLineEdit*>( widget() ) ) {
                        edit->setText( commonPrefix );
                        edit->setCursorPosition( static_cast<int>( commonPrefix.length() ) );

                        auto cr = edit->CursorRect();
                        cr.setWidth( popup()->sizeHintForColumn( 0 ) +
                                     popup()->verticalScrollBar()->sizeHint().width() );
                        complete( cr );

                        auto popupRect = popup()->geometry();
                        popupRect.moveBottom( edit->mapToGlobal( QPoint( 0, 0 ) ).y() );
                        popup()->setGeometry( popupRect );

                        return true;
                    }
                }
            }
        }
    } else if ( event->type() == QEvent::Show ) {
        popup()->setFixedWidth( dynamic_cast<QLineEdit*>( this->parent() )->width() - 200 );
    }

    return QCompleter::eventFilter( parent, event );
}

auto HcAgentCompleter::addCommand(
    const std::string& command,
    const std::string& description
) -> void {
    _items.push_back( std::pair( command, description ) );

    const auto item = new QStandardItem( command.c_str() );
    item->setToolTip( description.c_str() );

    _model->appendRow( item );
}

auto HcAgentCompleter::getCurrentCompletions(
    void
) -> QStringList {
    auto completions = QStringList();

    for ( int row = 0; row < popup()->model()->rowCount(); ++row ) {
        auto index = popup()->model()->index( row, 0 );
        auto text  = popup()->model()->data( index, Qt::DisplayRole ).toString();
        completions.append( text );
    }

    return completions;
}

auto HcAgentCompleter::findLongestCommonPrefix(
    const QStringList& strings
) -> QString {
    if ( strings.isEmpty() ) {
        return QString();
    }

    auto prefix = strings[ 0 ];

    for ( const QString &str : strings ) {
        int j = 0;

        while ( j < prefix.length() && j < str.length() && prefix[ j ] == str[ j ] ) {
            ++j;
        }

        prefix = prefix.left( j );
        if ( prefix.isEmpty() ) {
            break;
        }
    }

    return prefix;
}

HcAgentConsole::HcAgentConsole(
    HcAgent* meta,
    QWidget* parent
) : QWidget( parent ), Meta( meta ) {
    if ( objectName().isEmpty() ) {
        setObjectName( "HcAgentConsole" );
    }

    gridLayout = new QGridLayout( this );
    gridLayout->setObjectName( "gridLayout" );
    gridLayout->setContentsMargins( 0, 0, 0, 0 );

    LabelHeader = new QLabel( this );
    LabelHeader->setObjectName( "LabelHeader" );

    Console = new QTextEdit( this );
    Console->setObjectName( "Console" );
    Console->setReadOnly( true );
    Console->setProperty( "HcConsole", "true" );

    LabelBottom = new QLabel( this );
    LabelBottom->setObjectName( "LabelBottom" );

    Input = new HcAgentLineEdit( this );
    Input->setObjectName( "Input" );
    Input->addAction( tr( "Clear console screen" ), QKeySequence( Qt::CTRL | Qt::Key::Key_L ), this, [&](){ Console->clear(); } );

    LabelInput = new QLabel( Input );
    LabelInput->setObjectName( "LabelInput" );
    Input->setPrefixLabel( LabelInput );

    Completer = new HcAgentCompleter( Input );
    Completer->setCompletionMode( QCompleter::PopupCompletion );
    Completer->setCaseSensitivity( Qt::CaseInsensitive );

    Input->setCompleter( Completer );

    connect( Input, &QLineEdit::returnPressed, this, &HcAgentConsole::inputEnter );

    gridLayout->addWidget( LabelHeader, 0, 0, 1, 2 );
    gridLayout->addWidget( Console,     1, 0, 1, 2 );
    gridLayout->addWidget( LabelBottom, 2, 0, 1, 2 );
    gridLayout->addWidget( Input,       3, 0, 1, 1 );

    QMetaObject::connectSlotsByName( this );
}

auto HcAgentConsole::setHeaderLabel(
    const QString& text
) -> void {
    LabelHeader->setText( text );
}

auto HcAgentConsole::setBottomLabel(
    const QString& text
) -> void {
    LabelBottom->setText( text );
}

auto HcAgentConsole::setInputLabel(
    const QString& text
) -> void {
    LabelInput->setText( text );
}

auto HcAgentConsole::inputEnter(
    void
) -> void {
    auto input = std::string();

    input = Input->text().toStdString();
    Input->clear();

    //
    // invoke the command in a separate thread
    // TODO: perhaps capture the future and add it
    //       to a list that can be canceled.
    auto future = QtConcurrent::run( []( HcAgent* agent, const std::string& input ) {
        HcPythonAcquire();

        if ( agent->interface().has_value() ) {
            try {
                agent->interface().value().attr( "_input_dispatch" )( input );
            } catch ( py11::error_already_set &eas ) {
                emit agent->ui.signal.ConsoleWrite( agent->uuid().c_str(), eas.what() );
            }
        } else {
            emit agent->ui.signal.ConsoleWrite( agent->uuid().c_str(), "[!] No agent script handler registered for this type" );
        }
    }, Meta, input );
}

auto HcAgentConsole::appendConsole(
    const QString& text
) const -> void {
    Console->append( text );
}

auto HcAgentConsole::addCompleteCommand(
    const std::string& command,
    const std::string& description
) const -> void {
    Completer->addCommand( command, description );
}

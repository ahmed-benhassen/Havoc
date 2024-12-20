#include <Havoc.h>
#include <ui/HcPageListener.h>

#include <QtCore/QVariant>
#include <QtWidgets/QApplication>
#include <QtWidgets/QGridLayout>
#include <QtWidgets/QLabel>
#include <QtWidgets/QWidget>

HcListenerItem::HcListenerItem(
        const QString& text,
        QWidget*       parent
) : HcListenerItem( parent ) {
    setText( text );
}

HcListenerItem::HcListenerItem( QWidget* parent ) : QWidget( parent ) {
    setObjectName( "HcStatusItem" );

    Item = new QTableWidgetItem();

    GridLayout = new QGridLayout( this );
    GridLayout->setObjectName( "GridLayout" );
    GridLayout->setSpacing( 0 );
    GridLayout->setContentsMargins( 0, 0, 0, 0 );

    LabelStatus = new QLabel( this );
    LabelStatus->setObjectName( "LabelStatus" );

    auto policy = QSizePolicy( QSizePolicy::Maximum, QSizePolicy::Maximum );
    policy.setHorizontalStretch( 0 );
    policy.setVerticalStretch( 0 );
    policy.setHeightForWidth( LabelStatus->sizePolicy().hasHeightForWidth() );

    LabelStatus->setSizePolicy( policy );
    LabelStatus->setAlignment( Qt::AlignCenter );

    GridLayout->addWidget( LabelStatus, 0, 0, 1, 1 );

    QMetaObject::connectSlotsByName( this );
}

auto HcListenerItem::setText(
    const QString& string
) const -> void {
    auto text = QString( string );

    if ( text.isEmpty() ) {
        //
        // if the string status is emtpy then do no
        // change anything about the current status
        //
        return;
    }

    if ( text.startsWith( "+" ) ) {
        text.remove( 0, 1 );
        LabelStatus->setProperty( "HcLabelDisplay", "green" );
    } else if ( text.startsWith( "-" ) ) {
        text.remove( 0, 1 );
        LabelStatus->setProperty( "HcLabelDisplay", "red" );
        LabelStatus->setToolTip( text );
    } else if ( text.startsWith( "*" ) ) {
        text.remove( 0, 1 );
        LabelStatus->setProperty( "HcLabelDisplay", "tag" );
    }

    LabelStatus->setText( text );
    LabelStatus->style()->unpolish( LabelStatus );
    LabelStatus->style()->polish( LabelStatus );
}

HcPageListener::HcPageListener() {
    if ( objectName().isEmpty() ) {
        setObjectName( QString::fromUtf8( "PageListener" ) );
    }

    gridLayout = new QGridLayout( this );
    gridLayout->setObjectName( QString::fromUtf8( "gridLayout" ) );

    ActiveLabel = new QLabel( this );
    ActiveLabel->setObjectName( QString::fromUtf8("LabelDisplayListenerActive" ) );
    ActiveLabel->setProperty( "HcLabelDisplay", "true" );

    horizontalSpacer  = new QSpacerItem( 40, 20, QSizePolicy::Expanding, QSizePolicy::Minimum );
    ButtonNewListener = new QPushButton( this );
    ButtonNewListener->setObjectName( QString::fromUtf8( "ButtonNewListener" ) );
    ButtonNewListener->setProperty( "HcButton", "true" );

    Splitter = new QSplitter( this );
    Splitter->setObjectName( QString::fromUtf8( "Splitter" ) );
    Splitter->setOrientation( Qt::Vertical );

    TableWidget = new QTableWidget( Splitter );
    TableWidget->setObjectName( QString::fromUtf8( "TableWidget" ) );

    TitleName   = new QTableWidgetItem( "Name" );
    TitleType   = new QTableWidgetItem( "Type" );
    TitleHost   = new QTableWidgetItem( "Host" );
    TitlePort   = new QTableWidgetItem( "Port" );
    TitleStatus = new QTableWidgetItem( "Status" );

    if ( TableWidget->columnCount() < 5 ) {
        TableWidget->setColumnCount( 5 );
    }

    TableWidget->setHorizontalHeaderItem( 0, TitleName   );
    TableWidget->setHorizontalHeaderItem( 1, TitleType   );
    TableWidget->setHorizontalHeaderItem( 2, TitleHost   );
    TableWidget->setHorizontalHeaderItem( 3, TitlePort   );
    TableWidget->setHorizontalHeaderItem( 4, TitleStatus );

    /* table settings */
    TableWidget->setEnabled( true );
    TableWidget->setShowGrid( false );
    TableWidget->setSortingEnabled( false );
    TableWidget->setWordWrap( true );
    TableWidget->setAlternatingRowColors( true );
    TableWidget->setCornerButtonEnabled( true );
    TableWidget->horizontalHeader()->setVisible( true );
    TableWidget->setSelectionBehavior( QAbstractItemView::SelectRows );
    TableWidget->setContextMenuPolicy( Qt::CustomContextMenu );
    TableWidget->horizontalHeader()->setSectionResizeMode( QHeaderView::ResizeMode::Stretch );
    TableWidget->horizontalHeader()->setStretchLastSection( true );
    TableWidget->verticalHeader()->setVisible( false );
    TableWidget->setFocusPolicy( Qt::NoFocus );
    
    TabWidget = new QTabWidget( Splitter );
    TabWidget->setObjectName( QString::fromUtf8( "TabWidget" ) );
    TabWidget->setMovable( true );
    TabWidget->setTabsClosable( true );
    TabWidget->tabBar()->setProperty( "HcTab", "true" );

    Splitter->addWidget( TableWidget );
    Splitter->addWidget( TabWidget );
    Splitter->handle( 1 )->setEnabled( false ); /* disabled by default */

    gridLayout->addWidget( ActiveLabel, 0, 0, 1, 1 );
    gridLayout->addItem( horizontalSpacer, 0, 1, 1, 1 );
    gridLayout->addWidget( ButtonNewListener, 0, 2, 1, 1 );
    gridLayout->addWidget( Splitter, 1, 0, 1, 3 );

    retranslateUi();

    QObject::connect( ButtonNewListener, &QPushButton::clicked, this, &HcPageListener::buttonAddListener );
    QObject::connect( TableWidget, &QTableWidget::customContextMenuRequested, this, &HcPageListener::handleListenerContextMenu );
    QObject::connect( TabWidget->tabBar(), &QTabBar::tabCloseRequested, this, &HcPageListener::tabCloseRequested );

    QMetaObject::connectSlotsByName( this );
}

auto HcPageListener::retranslateUi() -> void {
    setStyleSheet( HcApplication::StyleSheet() );

    setWindowTitle( "PageListener" );
    ActiveLabel->setText( "Listeners: 0" );
    ButtonNewListener->setText( "Add Listener" );
}

auto HcPageListener::buttonAddListener() -> void {
    auto Dialog = new HcListenerDialog;

    Dialog->start();

    if ( Dialog->getCloseState() != Closed ) {
        HcPythonAcquire();
        delete Dialog;
    }
}

auto HcPageListener::addListener(
    const json& data
) -> void {
    auto name     = QString();
    auto type     = QString();
    auto host     = QString();
    auto port     = QString();
    auto status   = QString();
    auto listener = new HcListener();

    if ( data.contains( "name" ) ) {
        if ( data[ "name" ].is_string() ) {
            name = data[ "name" ].get<std::string>().c_str();
        } else {
            spdlog::error( "invalid listener: \"name\" is not string" );
            return;
        }
    } else {
        spdlog::error( "invalid listener: \"name\" is not found" );
        return;
    }

    if ( data.contains( "protocol" ) ) {
        if ( data[ "protocol" ].is_string() ) {
            type = data[ "protocol" ].get<std::string>().c_str();
        } else {
            spdlog::error( "invalid listener: \"protocol\" is not string" );
            return;
        }
    } else {
        spdlog::error( "invalid listener: \"protocol\" is not found" );
        return;
    }

    if ( data.contains( "host" ) ) {
        if ( data[ "host" ].is_string() ) {
            host = data[ "host" ].get<std::string>().c_str();
        } else {
            spdlog::error( "invalid listener: \"host\" is not string" );
            return;
        }
    } else {
        spdlog::error( "invalid listener: \"host\" is not found" );
        return;
    }

    if ( data.contains( "port" ) ) {
        if ( data[ "port" ].is_string() ) {
            port = data[ "port" ].get<std::string>().c_str();
        } else {
            spdlog::error( "invalid listener: \"port\" is not string" );
            return;
        }
    } else {
        spdlog::error( "invalid listener: \"port\" is not found" );
        return;
    }

    if ( data.contains( "status" ) ) {
        if ( data[ "status" ].is_string() ) {
            status = data[ "status" ].get<std::string>().c_str();
        } else {
            spdlog::error( "invalid listener: \"status\" is not string" );
            return;
        }
    } else {
        spdlog::error( "invalid listener: \"status\" is not found" );
        return;
    }

    listener->name       = name.toStdString();
    listener->protocol   = type.toStdString();
    listener->NameItem   = new HcListenerItem( name );
    listener->TypeItem   = new HcListenerItem( type );
    listener->HostItem   = new HcListenerItem( host );
    listener->PortItem   = new HcListenerItem( port );
    listener->StatusItem = new HcListenerItem();
    listener->Logger     = new QTextEdit();

    listener->Logger->setProperty( "HcConsole", "true" );
    listener->Logger->setReadOnly( true );

    if ( TableWidget->rowCount() < 1 ) {
        TableWidget->setRowCount( 1 );
    } else {
        TableWidget->setRowCount( TableWidget->rowCount() + 1 );
    }

    listener->StatusItem->setText( status );

    //
    // note: we have to also add an empty item to the table cell,
    //       so we can use itemAt in the context handler
    //

    TableWidget->setItem( TableWidget->rowCount() - 1, 0, listener->NameItem->Item );
    TableWidget->setCellWidget( TableWidget->rowCount() - 1, 0, listener->NameItem );

    TableWidget->setItem( TableWidget->rowCount() - 1, 1, listener->TypeItem->Item );
    TableWidget->setCellWidget( TableWidget->rowCount() - 1, 1, listener->TypeItem );

    TableWidget->setItem( TableWidget->rowCount() - 1, 2, listener->HostItem->Item );
    TableWidget->setCellWidget( TableWidget->rowCount() - 1, 2, listener->HostItem );

    TableWidget->setItem( TableWidget->rowCount() - 1, 3, listener->PortItem->Item );
    TableWidget->setCellWidget( TableWidget->rowCount() - 1, 3, listener->PortItem );

    TableWidget->setItem( TableWidget->rowCount() - 1, 4, listener->StatusItem->Item );
    TableWidget->setCellWidget( TableWidget->rowCount() - 1, 4, listener->StatusItem );

    TableEntries.push_back( listener );

    /* increase the number of listeners */
    ListenersRunning++;

    updateListenersRunningLabel( ListenersRunning );
}

auto HcPageListener::removeListener(
    const std::string& name
) -> void {
    HcListener* listener      = {};
    auto        listener_name = std::string();

    //
    // remove the listener from
    // the table ui widget entry
    //
    for ( int i = 0; i < TableWidget->rowCount(); i++ ) {
        listener_name = ( ( HcListenerItem* ) TableWidget->cellWidget( i, 0 ) )->LabelStatus->text().toStdString();

        if ( listener_name == name ) {
            spdlog::debug( "Remove listener from table: {}", name );
            TableWidget->removeRow( i );
            break;
        }
    }

    //
    // remove the listener entry from
    // the table entry vector list
    //
    for ( int i = 0; i < TableEntries.size(); i++ ) {
        if ( TableEntries[ i ]->name == listener_name ) {
            listener = TableEntries[ i ];

            TableEntries.erase( TableEntries.begin() + i );
            break;
        }
    }

    if ( listener ) {
         delete listener->NameItem;
         delete listener->TypeItem;
         delete listener->HostItem;
         delete listener->PortItem;
         delete listener->StatusItem;
         delete listener->Logger;
         delete listener;
    }

    //
    // NOTE: for what ever it causes the TableWidget to disappear lol
    //
    //    if ( TabWidget->count() == 0 ) {
    //        Splitter->setSizes( QList<int>() << 0 );
    //        Splitter->handle( 1 )->setEnabled( false );
    //        Splitter->handle( 1 )->setCursor( Qt::ArrowCursor );
    //    }

    /* decrease the number of listeners */
    ListenersRunning--;

    updateListenersRunningLabel( ListenersRunning );
}

auto HcPageListener::updateListenersRunningLabel(
    int value
) const -> void {
    ActiveLabel->setText( QString( "Listeners: %1" ).arg( value ) );
}

auto HcPageListener::addListenerLog(
    const std::string& name,
    const std::string& log
) -> void {
    auto listener = getListener( name.c_str() );
    if ( listener.has_value() ) {
        listener.value()->Logger->append( log.c_str() );
    }
}

auto HcPageListener::setListenerStatus(
    const std::string& name,
    const std::string& status
) -> void {
    auto listener = getListener( name.c_str() );
    if ( listener.has_value() ) {
        listener.value()->StatusItem->setText( status.c_str() );
    }
}

auto HcPageListener::handleListenerContextMenu(
    const QPoint& pos
) -> void {
    auto menu       = QMenu( this );
    auto name       = QString();
    auto type       = std::string();
    auto selections = TableWidget->selectionModel()->selectedRows();
    auto actions    = Havoc->Actions( HcApplication::ActionObject::ActionListener );

    if ( ! TableWidget->itemAt( pos ) ) {
        return;
    }

    if ( selections.count() > 1 ) {
        menu.addAction( QIcon( ":/icons/16px-listener-start" ), "Start" );
        menu.addAction( QIcon( ":/icons/16px-listener-stop" ), "Stop" );
        menu.addAction( QIcon( ":/icons/16px-listener-restart" ), "Restart" );
        menu.addSeparator();
        menu.addAction( QIcon( ":/icons/16px-listener-logs" ), "Logs" );
        menu.addSeparator();
        menu.addAction( QIcon( ":/icons/16px-remove" ), "Remove" );
    } else {
        menu.addAction( QIcon( ":/icons/16px-listener-start" ), "Start" );
        menu.addAction( QIcon( ":/icons/16px-listener-stop" ), "Stop" );
        menu.addAction( QIcon( ":/icons/16px-listener-restart" ), "Restart" );
        menu.addSeparator();

        type = ( ( HcListenerItem* ) TableWidget->cellWidget( selections.at( 0 ).row(), 1 ) )->LabelStatus->text().toStdString();

        for ( auto action : actions ) {
            spdlog::debug( "action->listener: {} == {}", action->listener.type, type );
            if ( action->listener.type == type ) {
                if ( action->icon.isNull() ) {
                    menu.addAction( action->name.c_str() );
                } else {
                    menu.addAction( action->icon, action->name.c_str() );
                }
            }
        }

        menu.addSeparator();
        menu.addAction( QIcon( ":/icons/16px-listener-edit" ), "Edit" );
        menu.addAction( QIcon( ":/icons/16px-listener-logs" ), "Logs" );
        menu.addSeparator();
        menu.addAction( QIcon( ":/icons/16px-remove" ), "Remove" );
    }

    if ( auto action = menu.exec( TableWidget->horizontalHeader()->viewport()->mapToGlobal( pos ) ) ) {
        for ( const auto& selected : selections ) {
            name = ( ( HcListenerItem* ) TableWidget->cellWidget( selected.row(), 0 ) )->LabelStatus->text();

            auto listener = getListener( name );
            if ( ! listener.has_value() ) {
                spdlog::debug( "[ERROR] listener not found" );
                continue;
            }

            if ( action->text().compare( "Logs" ) == 0 ) {
                if ( TabWidget->count() == 0 ) {
                    Splitter->setSizes( QList<int>() << 200 << 220 );
                    Splitter->handle( 1 )->setEnabled( true );
                    Splitter->handle( 1 )->setCursor( Qt::SplitVCursor );
                }

                TabWidget->addTab( listener.value()->Logger, QIcon( ":/icons/16px-listener-logs" ), name );
            }
            else if ( action->text().compare( "Start" ) == 0 ) {
                auto error = listener.value()->start();

                if ( error.has_value() ) {
                    Helper::MessageBox(
                        QMessageBox::Critical,
                        "listener failure",
                        std::format( "listener starting failure with {}: {}", name.toStdString(), error.value() )
                    );

                    spdlog::debug( "listener starting failure with {}: {}", name.toStdString(), error.value() );
                }
            }
            else if ( action->text().compare( "Stop" ) == 0 ) {
                auto error = listener.value()->stop();

                if ( error.has_value() ) {
                    Helper::MessageBox(
                        QMessageBox::Critical,
                        "listener failure",
                        std::format( "listener stopping failure with {}: {}", name.toStdString(), error.value() )
                    );

                    spdlog::debug( "listener stopping failure with {}: {}", name.toStdString(), error.value() );
                }
            }
            else if ( action->text().compare( "Restart" ) == 0 ) {
                auto error = listener.value()->restart();

                if ( error.has_value() ) {
                    Helper::MessageBox(
                        QMessageBox::Critical,
                        "listener failure",
                        std::format( "listener restarting failure with {}: {}", name.toStdString(), error.value() )
                    );

                    spdlog::debug( "listener restarting failure with {}: {}", name.toStdString(), error.value() );
                }
            }
            else if ( action->text().compare( "Edit" ) == 0 ) {
                auto error = listener.value()->edit();

                if ( error.has_value() ) {
                    Helper::MessageBox(
                        QMessageBox::Critical,
                        "listener failure",
                        std::format( "listener editing failure with {}: {}", name.toStdString(), error.value() )
                    );

                    spdlog::debug( "listener editing failure with {}: {}", name.toStdString(), error.value() );
                }
            }
            else if ( action->text().compare( "Remove" ) == 0 ) {
                auto error = listener.value()->remove();

                if ( error.has_value() ) {
                    Helper::MessageBox(
                        QMessageBox::Critical,
                        "listener failure",
                        std::format( "listener removal failure with {}: {}", name.toStdString(), error.value() )
                    );

                    spdlog::debug( "listener removal failure with {}: {}", name.toStdString(), error.value() );
                }
            }
            else {
                for ( auto listener_action : actions ) {
                    if ( action->text().toStdString()   == listener_action->name &&
                         listener_action->listener.type == type
                    ) {
                        try {
                            HcPythonAcquire();

                            listener_action->callback_py( name.toStdString() );
                        } catch ( py11::error_already_set& e ) {
                            spdlog::error( "failed to execute action callback: {}", e.what() );
                        }
                        return;
                    }
                }

                spdlog::debug( "[ERROR] invalid action from selected listener menu" );
            }
        }
    }
}

auto HcPageListener::tabCloseRequested(
    int index
) const -> void {
    if ( index == -1 ) {
        return;
    }

    TabWidget->removeTab( index );

    if ( TabWidget->count() == 0 ) {
        Splitter->setSizes( QList<int>() << 0 );
        Splitter->handle( 1 )->setEnabled( false );
        Splitter->handle( 1 )->setCursor( Qt::ArrowCursor );
    }
}

auto HcPageListener::getListener(
    const QString& name
) -> std::optional<HcListener*> {
    for ( int i = 0; i < TableEntries.size(); i++ ) {
        if ( name.compare( TableEntries[ i ]->name.c_str() ) == 0 ) {
            return TableEntries[ i ];
        }
    }

    return {};
}

auto HcListener::stop(
    void
) -> std::optional<std::string> {
    auto error = std::string();
    auto data  = json();

    auto [status, response] = Havoc->ApiSend( "api/listener/stop", {
        { "name", name }
    } );

    if ( status != 200 ) {
        if ( response.empty() ) {
            goto ERROR_SERVER_RESPONSE;
        }

        try {
            if ( ( data = json::parse( response ) ).is_discarded() ) {
                spdlog::error( "failed to parse json object: json has been discarded" );
                goto ERROR_SERVER_RESPONSE;
            };
        } catch ( std::exception& e ) {
            spdlog::error( "exception raised while parsing json object: {}", e.what() );
            goto ERROR_SERVER_RESPONSE;
        };

        if ( ! data.contains( "error" ) ) {
            goto ERROR_SERVER_RESPONSE;
        }

        if ( ! data[ "error" ].is_string() ) {
            goto ERROR_SERVER_RESPONSE;
        }

        return data[ "error" ].get<std::string>();
    }

    return {};

ERROR_SERVER_RESPONSE:
    return std::optional<std::string>( "invalid response from the server" );
}

auto HcListener::start(
    void
) -> std::optional<std::string> {
    auto error = std::string();
    auto data  = json();

    auto [status, response] = Havoc->ApiSend( "api/listener/start", {
        { "name",     name     },
        { "protocol", protocol }
    } );

    if ( status != 200 ) {
        if ( response.empty() ) {
            goto ERROR_SERVER_RESPONSE;
        }

        try {
            if ( ( data = json::parse( response ) ).is_discarded() ) {
                spdlog::error( "failed to parse json object: json has been discarded" );
                goto ERROR_SERVER_RESPONSE;
            };
        } catch ( std::exception& e ) {
            spdlog::error( "exception raised while parsing json object: {}", e.what() );
            goto ERROR_SERVER_RESPONSE;
        };

        if ( ! data.contains( "error" ) ) {
            goto ERROR_SERVER_RESPONSE;
        }

        if ( ! data[ "error" ].is_string() ) {
            goto ERROR_SERVER_RESPONSE;
        }

        return data[ "error" ].get<std::string>();
    }

    return {};

ERROR_SERVER_RESPONSE:
    return std::optional<std::string>( "invalid response from the server" );
}

auto HcListener::restart(
    void
) -> std::optional<std::string> {
    auto error = std::string();
    auto data  = json();

    auto [status, response] = Havoc->ApiSend( "api/listener/restart", {
        { "name", name }
    } );

    if ( status != 200 ) {
        if ( response.empty() ) {
            goto ERROR_SERVER_RESPONSE;
        }

        try {
            if ( ( data = json::parse( response ) ).is_discarded() ) {
                spdlog::error( "failed to parse json object: json has been discarded" );
                goto ERROR_SERVER_RESPONSE;
            };
        } catch ( std::exception& e ) {
            spdlog::error( "exception raised while parsing json object: {}", e.what() );
            goto ERROR_SERVER_RESPONSE;
        };

        if ( ! data.contains( "error" ) ) {
            goto ERROR_SERVER_RESPONSE;
        }

        if ( ! data[ "error" ].is_string() ) {
            goto ERROR_SERVER_RESPONSE;
        }

        return data[ "error" ].get<std::string>();
    }

    return {};

ERROR_SERVER_RESPONSE:
    return std::optional<std::string>( "invalid response from the server" );
}

auto HcListener::edit(
    void
) -> std::optional<std::string> {
    auto listener = new HcListenerDialog( protocol.c_str() );
    auto error    = std::string();
    auto data     = json();

    //
    // first we have to get the
    // config from the listener
    //

    auto [status, response] = Havoc->ApiSend( "api/listener/config", { { "name", name } } );

    if ( status != 200 ) {
        if ( response.empty() ) {
            goto ERROR_SERVER_RESPONSE;
        }

        try {
            if ( ( data = json::parse( response ) ).is_discarded() ) {
                goto ERROR_SERVER_RESPONSE;
            }
        } catch ( std::exception& e ) {
            spdlog::error( "failed to parse json: {}", e.what() );
            goto ERROR_SERVER_RESPONSE;
        }

        if ( ! data.contains( "error" ) ) {
            goto ERROR_SERVER_RESPONSE;
        }

        if ( ! data[ "error" ].is_string() ) {
            goto ERROR_SERVER_RESPONSE;
        }

        return data[ "error" ].get<std::string>();
    } else {
        try {
            if ( ( data = json::parse( response ) ).is_discarded() ) {
                goto ERROR_SERVER_RESPONSE;
            }
        } catch ( std::exception& e ) {
            spdlog::error( "failed to parse json: {}", e.what() );
            goto ERROR_SERVER_RESPONSE;
        }
    }

    //
    // generate the UI listener and allow
    // the listener to be edited
    //

    listener->setEditingListener( name.c_str(), data );
    listener->start();

    if ( listener->getCloseState() != Closed ) {
        delete listener;
    }

    return {};

ERROR_SERVER_RESPONSE:
    return std::optional<std::string>( "invalid response from the server" );
}

auto HcListener::remove(
    void
) -> std::optional<std::string> {
    auto error = std::string();
    auto data  = json();

    auto [status, response] = Havoc->ApiSend( "api/listener/remove", {
        { "name", name }
    } );

    if ( status != 200 ) {
        if ( response.empty() ) {
            goto ERROR_SERVER_RESPONSE;
        }

        try {
            if ( ( data = json::parse( response ) ).is_discarded() ) {
                spdlog::error( "failed to parse json object: json has been discarded" );
                goto ERROR_SERVER_RESPONSE;
            };
        } catch ( std::exception& e ) {
            spdlog::error( "exception raised while parsing json object: {}", e.what() );
            goto ERROR_SERVER_RESPONSE;
        };

        if ( ! data.contains( "error" ) ) {
            goto ERROR_SERVER_RESPONSE;
        }

        if ( ! data[ "error" ].is_string() ) {
            goto ERROR_SERVER_RESPONSE;
        }

        return data[ "error" ].get<std::string>();
    }

    return {};

ERROR_SERVER_RESPONSE:
    return std::optional<std::string>( "invalid response from the server" );
}
#include <Havoc.h>
#include <ui/HcListenerDialog.h>

#include <QtCore/QVariant>
#include <QtWidgets/QGridLayout>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QWidget>

HcListenerDialog::HcListenerDialog(
    const QString& editing
) {
    if ( objectName().isEmpty() ) {
        setObjectName( QString::fromUtf8( "HcListenerDialog" ) );
    }

    gridLayout = new QGridLayout( this );
    gridLayout->setObjectName( "gridLayout" );

    InputName = new HcLineEdit( this );
    InputName->Input->setObjectName( QString::fromUtf8( "InputName" ) );

    ComboProtocol = new HcComboBox;
    ComboProtocol->Combo->setObjectName( QString::fromUtf8( "ComboProtocol" ) );

    StackedProtocols = new QStackedWidget( this );
    StackedProtocols->setObjectName( QString::fromUtf8( "HcListenerDialog.StackedProtocols" ) );

    ButtonSave = new QPushButton( this );
    ButtonSave->setObjectName( "ButtonClose" );
    ButtonSave->setProperty( "HcButton", "true" );

    ButtonClose = new QPushButton( this );
    ButtonClose->setObjectName( "ButtonClose" );
    ButtonClose->setProperty( "HcButton", "true" );

    gridLayout->addWidget( InputName,        0, 0, 1, 6 );
    gridLayout->addWidget( ComboProtocol,    1, 0, 1, 6 );
    gridLayout->addWidget( StackedProtocols, 2, 0, 1, 6 );

    horizontal[ 0 ] = new QSpacerItem( 40, 20, QSizePolicy::Expanding, QSizePolicy::Minimum );
    horizontal[ 1 ] = new QSpacerItem( 40, 20, QSizePolicy::Expanding, QSizePolicy::Minimum );
    horizontal[ 2 ] = new QSpacerItem( 40, 20, QSizePolicy::Expanding, QSizePolicy::Minimum );
    horizontal[ 3 ] = new QSpacerItem( 40, 20, QSizePolicy::Expanding, QSizePolicy::Minimum );

    gridLayout->addWidget( ButtonSave,  3, 2, 1, 1 );
    gridLayout->addWidget( ButtonClose, 3, 3, 1, 1 );

    gridLayout->addItem( horizontal[ 0 ], 3, 0, 1, 1 );
    gridLayout->addItem( horizontal[ 2 ], 3, 1, 1, 1 );
    gridLayout->addItem( horizontal[ 3 ], 3, 4, 1, 1 );
    gridLayout->addItem( horizontal[ 1 ], 3, 5, 1, 1 );

    retranslateUi();

    QMetaObject::connectSlotsByName( this );

    connect( ComboProtocol->Combo, &QComboBox::currentTextChanged, this, &HcListenerDialog::changeProtocol );
    connect( ButtonSave,  &QPushButton::clicked, this, &HcListenerDialog::save );
    connect( ButtonClose, &QPushButton::clicked, this, [&]() {
        State = Closed;
        close();
    } );

    retranslateUi();

    for ( auto& name : Havoc->Protocols() ) {
        if ( Havoc->ProtocolObject( name ).has_value() ) {
            if ( editing.isEmpty() ) {
                AddProtocol( name, Havoc->ProtocolObject( name ).value() );
            } else if ( editing.toStdString() == name ) {
                AddProtocol( name, Havoc->ProtocolObject( name ).value() );
                break;
            }
        }
    }

    QMetaObject::connectSlotsByName( this );
}

HcListenerDialog::~HcListenerDialog() {
    HcPythonAcquire();

    for ( auto& protocol : Protocols ) {
        protocol.instance.dec_ref();

        delete protocol.widget;
    }
}

auto HcListenerDialog::retranslateUi() -> void {
    setStyleSheet( Havoc->StyleSheet() );
    setWindowTitle( "Listener" );

    ComboProtocol->Combo->clear();

    InputName->setLabelText( "Name:      " );
    ComboProtocol->setLabelText( "Protocol:  " );
    ButtonSave->setText( "Save" );
    ButtonClose->setText( "Close" );
}

auto HcListenerDialog::changeProtocol(
    const QString &text
) -> void {
    for ( int i = 0; i < Protocols.size(); i++ ) {
        if ( Protocols[ i ].name == text.toStdString() ) {
            StackedProtocols->setCurrentIndex( i );
            break;
        }
    }
}

auto HcListenerDialog::getCloseState() -> ListenerState { return State; }

auto HcListenerDialog::start() -> void {
    if ( Havoc->Protocols().empty() ) {
        Helper::MessageBox(
            QMessageBox::Icon::Warning,
            "Listener error",
            "No protocols available"
        );
        return;
    }

    ComboProtocol->Combo->setCurrentIndex( 0 );
    exec();
}

auto HcListenerDialog::save() -> void {
    auto data      = json();
    auto exception = std::string();

    if ( InputName->text().isEmpty() ) {
        Helper::MessageBox(
            QMessageBox::Critical,
            "Listener failure",
            "Failed to start listener: name is empty"
        );
    }

    auto protocol = getCurrentProtocol();
    try {
        HcPythonAcquire();

        /* sanity check input */
        if ( ! protocol.instance.attr( "sanity_check" )().cast<bool>() ) {
            /* sanity check failed. exit and dont send request */
            spdlog::debug( "sanity check failed. exit and dont send request" );
            return;
        }

        data = {
            { "name",     InputName->text().toStdString() },
            { "protocol", protocol.name },
            { "config",   protocol.instance.attr( "save" )() }
        };
    } catch ( py11::error_already_set &eas ) {
        Helper::MessageBox(
            QMessageBox::Icon::Critical,
            "Listener saving error",
            std::string( eas.what() )
        );
        return;
    }

    State = Error;

    auto [status_code, response] = Havoc->ApiSend(
        edited_config.empty() ? "api/listener/start" : "api/listener/edit",
        data
    );

    if ( status_code != 200 ) {
        if ( response.empty() ) {
            goto ERROR_SERVER_RESPONSE;
        }

        try {
            if ( ( data = json::parse( response ) ).is_discarded() ) {
                goto ERROR_SERVER_RESPONSE;
            }
        } catch ( std::exception& e ) {
            spdlog::error( "failed to parse json response: \n{}", e.what() );
            goto ERROR_SERVER_RESPONSE;
        }

        if ( ! data.contains( "error" ) ) {
            goto ERROR_SERVER_RESPONSE;
        }

        if ( ! data[ "error" ].is_string() ) {
            goto ERROR_SERVER_RESPONSE;
        }

        Helper::MessageBox(
            QMessageBox::Critical,
            "Listener failure",
            QString( "failed to start listener: %1" ).arg( data[ "error" ].get<std::string>().c_str() ).toStdString()
        );

        State = Error;
    } else {
        State = Saved;
    }

    close();
    return;

ERROR_SERVER_RESPONSE:
    Helper::MessageBox(
        QMessageBox::Critical,
        "Listener failure",
        "listener failure: Invalid response from the server"
    );

    close();
}

auto HcListenerDialog::getCurrentProtocol() -> Protocol { return Protocols[ StackedProtocols->currentIndex() ]; }

auto HcListenerDialog::AddProtocol(
    const std::string&  name,
    const py11::object& object
) -> void {
    HcPythonAcquire();

    auto protocol = Protocol {
        .name   = name,
        .widget = new QWidget,
    };

    try {
        protocol.instance = object( U_PTR( protocol.widget ), name );
        auto _ = protocol.instance.attr( "_hc_main" )();
    } catch ( py11::error_already_set &eas ) {
        Helper::MessageBox(
            QMessageBox::Icon::Critical,
            "Listener loading error",
            std::string( eas.what() )
        );
        return;
    }

    ComboProtocol->Combo->addItem( protocol.name.c_str() );
    StackedProtocols->addWidget( protocol.widget );

    Protocols.push_back( protocol );
}

auto HcListenerDialog::setEditingListener(
    const QString& name,
    const json&    config
) -> void {

    InputName->setInputText( name );
    ComboProtocol->Combo->setDisabled( true );
    InputName->Input->setDisabled( true );

    edited_config = config;

    try {
        HcPythonAcquire();

        auto _ = getCurrentProtocol().instance.attr( "edit" )( config );
    } catch ( py11::error_already_set& e ) {
        spdlog::debug( "failed to execute edit method of listener: \n{}", e.what() );
        Helper::MessageBox(
            QMessageBox::Icon::Critical,
            "Listener editing error",
            std::string( e.what() )
        );
    }
}

//
// HcListenerChooseDialog
//

HcListenerChooseDialog::HcListenerChooseDialog( const std::string& protocol ) : Protocol( protocol ) {}
HcListenerChooseDialog::~HcListenerChooseDialog() { delete Dialog; }

auto HcListenerChooseDialog::start(
    void
) -> void {
    Dialog = new QDialog;

    Dialog->setWindowTitle( "Listener Select" );
    Dialog->resize( 743, 378 );
    Dialog->setStyleSheet( HcApplication::StyleSheet() );

    GridLayout    = new QGridLayout( Dialog );
    TableListener = new QTableWidget( Dialog );

    TableListener->setObjectName( QString::fromUtf8( "TableListener" ) );
    TableListener->setColumnCount( 5 );
    TableListener->setHorizontalHeaderItem( 0, new QTableWidgetItem( "Name" ) );
    TableListener->setHorizontalHeaderItem( 1, new QTableWidgetItem( "Type" ) );
    TableListener->setHorizontalHeaderItem( 2, new QTableWidgetItem( "Host" ) );
    TableListener->setHorizontalHeaderItem( 3, new QTableWidgetItem( "Port" ) );
    TableListener->setHorizontalHeaderItem( 4, new QTableWidgetItem( "Status" ) );

    TableListener->setEnabled( true );
    TableListener->setShowGrid( false );
    TableListener->setSortingEnabled( false );
    TableListener->setWordWrap( true );
    TableListener->setAlternatingRowColors( true );
    TableListener->setCornerButtonEnabled( true );
    TableListener->horizontalHeader()->setVisible( true );
    TableListener->setSelectionBehavior( QAbstractItemView::SelectRows );
    TableListener->setContextMenuPolicy( Qt::CustomContextMenu );
    TableListener->horizontalHeader()->setSectionResizeMode( QHeaderView::ResizeMode::Stretch );
    TableListener->horizontalHeader()->setStretchLastSection( true );
    TableListener->verticalHeader()->setVisible( false );
    TableListener->setFocusPolicy( Qt::NoFocus );

    GridLayout->setObjectName( QString::fromUtf8( "GridLayout" ) );
    GridLayout->addWidget( TableListener, 0, 0, 1, 4 );

    for ( const auto& listener_name : Havoc->Listeners() ) {
        const auto& opt = Havoc->ListenerObject( listener_name );

        if ( opt.has_value() ) {
            auto data   = opt.value();
            auto name   = QString();
            auto type   = QString();
            auto host   = QString();
            auto port   = QString();
            auto status = QString();

            if ( data.contains( "name" ) ) {
                if ( data[ "name" ].is_string() ) {
                    name = data[ "name" ].get<std::string>().c_str();
                } else {
                    spdlog::error( "invalid listener: \"name\" is not string" );
                    continue;
                }
            } else {
                spdlog::error( "invalid listener: \"name\" is not found" );
                continue;
            }

            if ( data.contains( "protocol" ) ) {
                if ( data[ "protocol" ].is_string() ) {
                    type = data[ "protocol" ].get<std::string>().c_str();
                } else {
                    spdlog::error( "invalid listener: \"protocol\" is not string" );
                    continue;
                }
            } else {
                spdlog::error( "invalid listener: \"protocol\" is not found" );
                continue;
            }

            if ( data.contains( "host" ) ) {
                if ( data[ "host" ].is_string() ) {
                    host = data[ "host" ].get<std::string>().c_str();
                } else {
                    spdlog::error( "invalid listener: \"host\" is not string" );
                    continue;
                }
            } else {
                spdlog::error( "invalid listener: \"host\" is not found" );
                continue;
            }

            if ( data.contains( "port" ) ) {
                if ( data[ "port" ].is_string() ) {
                    port = data[ "port" ].get<std::string>().c_str();
                } else {
                    spdlog::error( "invalid listener: \"port\" is not string" );
                    continue;
                }
            } else {
                spdlog::error( "invalid listener: \"port\" is not found" );
                continue;
            }

            if ( data.contains( "status" ) ) {
                if ( data[ "status" ].is_string() ) {
                    status = data[ "status" ].get<std::string>().c_str();
                } else {
                    spdlog::error( "invalid listener: \"status\" is not string" );
                    continue;
                }
            } else {
                spdlog::error( "invalid listener: \"status\" is not found" );
                continue;
            }

            if ( ! Protocol.empty() && type.toStdString() != Protocol ) {
                //
                // if the protocol has been specified and the current
                // listener data is the specified protocol then we are
                // going to include them into the table to display
                //
                continue;
            }

            auto NameItem   = new HcListenerItem( name );
            auto TypeItem   = new HcListenerItem( type );
            auto HostItem   = new HcListenerItem( host );
            auto PortItem   = new HcListenerItem( port );
            auto StatusItem = new HcListenerItem();

            StatusItem->setText( status );

            TableListener->setRowCount( TableListener->rowCount() + 1 );

            TableListener->setItem( TableListener->rowCount() - 1, 0, NameItem->Item );
            TableListener->setCellWidget( TableListener->rowCount() - 1, 0, NameItem );

            TableListener->setItem( TableListener->rowCount() - 1, 1, TypeItem->Item );
            TableListener->setCellWidget( TableListener->rowCount() - 1, 1, TypeItem );

            TableListener->setItem( TableListener->rowCount() - 1, 2, HostItem->Item );
            TableListener->setCellWidget( TableListener->rowCount() - 1, 2, HostItem );

            TableListener->setItem( TableListener->rowCount() - 1, 3, PortItem->Item );
            TableListener->setCellWidget( TableListener->rowCount() - 1, 3, PortItem );

            TableListener->setItem( TableListener->rowCount() - 1, 4, StatusItem->Item );
            TableListener->setCellWidget( TableListener->rowCount() - 1, 4, StatusItem );
        }
    }

    connect( TableListener, &QTableWidget::doubleClicked, [&]( const QModelIndex& index ) {
        SelectedName = dynamic_cast<HcListenerItem*>(
            TableListener->cellWidget( index.row(), 0 )
        )->LabelStatus->text().toStdString();

        Dialog->close();
    } );

    if ( TableListener->rowCount() < 1 ) {
        //
        // if there are no listeners with the specified protocol
        // or any kind of listeners available to choose from then
        // abort and do not show the dialog
        //
        return;
    }

    Dialog->exec();
}

auto HcListenerChooseDialog::listenerData(
    void
) const -> json {
    if ( ! SelectedName.empty() ) {
        const auto& data = Havoc->ListenerObject( SelectedName );

        return data.has_value() ? data.value() : json();
    }

    return {};
}

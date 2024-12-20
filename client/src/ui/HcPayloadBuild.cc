#include <Common.h>
#include <Havoc.h>
#include <ui/HcPayloadBuild.h>

HcProfileItem::HcProfileItem(
    const QString& name,
    const QString& type,
    const json&    profile,
    QObject*       parent
) : name( name ), type( type ), profile( profile ) {
    GridLayout = new QGridLayout( this );
    GridLayout->setObjectName( "gridLayout" );

    LabelName = new QLabel( this );
    LabelName->setText( name );

    auto font = LabelName->font();
    font.setBold( true );
    LabelName->setFont( font );

    LabelDetails = new QLabel( this );
    LabelDetails->setText( type );

    GridLayout->addWidget( LabelName,    0, 0, 1, 1 );
    GridLayout->addWidget( LabelDetails, 1, 0, 1, 1 );
    GridLayout->setColumnStretch( 0, 1 );

    QMetaObject::connectSlotsByName( this );
}

HcPayloadBuild::HcPayloadBuild(
    QWidget* parent
) : QDialog( parent ) {

    if ( objectName().isEmpty() ) {
        setObjectName( "HcPageBuilder" );
    }

    setWindowModality( Qt::WindowModal );

    gridLayout_2 = new QGridLayout( this );
    gridLayout_2->setObjectName( "gridLayout_2" );

    LabelPayload = new QLabel( this );
    LabelPayload->setObjectName( "LabelPayload" );
    LabelPayload->setProperty( "HcLabelDisplay", "true" );

    ComboPayload = new QComboBox( this );
    ComboPayload->setObjectName( "ComboPayload" );
    ComboPayload->setMinimumWidth( 200 );

    horizontalSpacer = new QSpacerItem( 789, 20, QSizePolicy::Expanding, QSizePolicy::Minimum );

    SplitterTopBottom = new QSplitter( this );
    SplitterTopBottom->setObjectName( "SplitterTopBottom" );
    SplitterTopBottom->setOrientation( Qt::Vertical );

    SplitterLeftRight = new QSplitter( SplitterTopBottom );
    SplitterLeftRight->setObjectName( "SplitterLeftRight" );
    SplitterLeftRight->setOrientation( Qt::Horizontal );

    StackedBuilders = new QStackedWidget( SplitterLeftRight );
    StackedBuilders->setObjectName( "StackedBuilders" );

    ProfileWidget = new QWidget( SplitterLeftRight );
    ProfileWidget->setObjectName( "ProfileWidget" );
    ProfileWidget->setContentsMargins( 0, 0, 0, 0 );

    gridLayout = new QGridLayout( ProfileWidget );
    gridLayout->setObjectName( "gridLayout" );

    ButtonGenerate = new QPushButton( ProfileWidget );
    ButtonGenerate->setObjectName( "ButtonGenerate" );
    ButtonGenerate->setProperty( "HcButton", "true" );
    ButtonGenerate->setProperty( "HcButtonPurple", "true" );

    ButtonSaveProfile = new QPushButton( ProfileWidget );
    ButtonSaveProfile->setObjectName( "ButtonSaveProfile" );
    ButtonSaveProfile->setProperty( "HcButton", "true" );

    ButtonLoadProfile = new QPushButton( ProfileWidget );
    ButtonLoadProfile->setObjectName( "ButtonLoadProfile" );
    ButtonLoadProfile->setProperty( "HcButton", "true" );

    ListProfiles = new QListWidget( ProfileWidget );
    ListProfiles->setObjectName( "ListProfiles" );
    ListProfiles->setProperty( "HcListWidget", "half-dark" );
    ListProfiles->setContextMenuPolicy( Qt::CustomContextMenu );

    TextBuildLog = new QTextEdit( SplitterTopBottom );
    TextBuildLog->setObjectName( "TextBuildLog" );
    TextBuildLog->setReadOnly( true );

    SplitterLeftRight->addWidget( StackedBuilders );
    SplitterLeftRight->addWidget( ProfileWidget );
    SplitterTopBottom->addWidget( SplitterLeftRight );
    SplitterTopBottom->addWidget( TextBuildLog );

    gridLayout->addWidget( ButtonGenerate,    0, 0, 1, 1 );
    gridLayout->addWidget( ButtonSaveProfile, 1, 0, 1, 1 );
    gridLayout->addWidget( ButtonLoadProfile, 2, 0, 1, 1 );
    gridLayout->addWidget( ListProfiles,      3, 0, 1, 1 );

    gridLayout_2->addWidget( LabelPayload,      0, 0, 1, 1 );
    gridLayout_2->addWidget( ComboPayload,      0, 1, 1, 1 );
    gridLayout_2->addWidget( SplitterTopBottom, 1, 0, 1, 3 );
    gridLayout_2->addItem( horizontalSpacer, 0, 2, 1, 1 );

    SplitterTopBottom->setSizes( QList<int>() << 0 );
    SplitterLeftRight->setSizes( QList<int>() << 1374 << 650 );

    retranslateUi();

    setStyleSheet( HcApplication::StyleSheet() );
    resize( 900, 880 );

    connect( ButtonGenerate,    &QPushButton::clicked, this, &HcPayloadBuild::clickedGenerate    );
    connect( ButtonSaveProfile, &QPushButton::clicked, this, &HcPayloadBuild::clickedProfileSave );
    connect( ButtonLoadProfile, &QPushButton::clicked, this, &HcPayloadBuild::clickedProfileLoad );
    connect( ListProfiles,      &QListWidget::itemDoubleClicked, this, &HcPayloadBuild::itemSelectProfile );
    connect( ListProfiles,      &QListWidget::customContextMenuRequested, this, &HcPayloadBuild::contextMenuProfile );

    for ( auto& name : Havoc->Builders() ) {
        if ( Havoc->BuilderObject( name ).has_value() ) {
            AddBuilder( name, Havoc->BuilderObject( name ).value() );
        }
    }

    QMetaObject::connectSlotsByName( this );
}

HcPayloadBuild::~HcPayloadBuild() {
    for ( auto& builder : Builders ) {
        delete builder.widget;
    }
}

auto HcPayloadBuild::retranslateUi() -> void {
    setWindowTitle( "Payload Builder" );

    if ( !ComboPayload->count() ) {
        ComboPayload->addItem( "(no payload available)" );
    }

    LabelPayload->setText( "Payload:" );
    ButtonGenerate->setText( "Generate" );
    ButtonSaveProfile->setText( "Save Profile" );
    ButtonLoadProfile->setText( "Load Profile" );

    for ( int i = 0; i < ListProfiles->count(); ++i ) {
        auto item   = ListProfiles->item( i );
        auto widget = ListProfiles->itemWidget( item );

        if ( widget ) {
            delete widget;
        }

        delete item;
    }

    ListProfiles->clear();

    for ( const auto& entry : Havoc->ProfileQuery( "profile" ) ) {
        auto name    = std::string();
        auto type    = std::string();
        auto profile = json();

        if ( !entry.contains( "name" )    ||
             !entry.contains( "type" )    ||
             !entry.contains( "profile" )
        ) {
            spdlog::error( "failed to parse following profile: {}", toml::format( entry ) );
            continue;
        }

        name = toml::find<std::string>( entry, "name" );
        type = toml::find<std::string>( entry, "type" );

        try {
            if ( ( profile = json::parse( toml::find<std::string>( entry, "profile" ) ) ).is_discarded() ) {
                spdlog::debug( "profile from toml entry has been discarded" );
                return;
            }
        } catch ( std::exception& e ) {
            spdlog::error( "failed to parse profile from toml entry:\n{}", e.what() );
            return;
        }

        auto item   = new QListWidgetItem;
        auto widget = new HcProfileItem(
            QString::fromStdString( name ),
            QString::fromStdString( type ),
            profile,
            this
        );

        item->setSizeHint( widget->sizeHint() );
        widget->setFocusPolicy( Qt::NoFocus );

        ListProfiles->addItem( item );
        ListProfiles->setItemWidget( item, widget );
    }
}

auto HcPayloadBuild::AddBuilder(
    const std::string&  name,
    const py11::object& object
) -> void {
    auto objname = "HcPageBuilderBuilder" + QString( name.c_str() );
    auto builder = Builder {
        .name    = name,
        .widget  = new QWidget
    };

    builder.widget->setObjectName( objname );
    builder.widget->setStyleSheet( "#" + objname + "{ background: " + Havoc->Theme.getBackground().name() + "}" );

    try {
        HcPythonAcquire();

        builder.instance = object( U_PTR( builder.widget ), U_PTR( this ), name );
        builder.instance.attr( "_hc_main" )();
    } catch ( py11::error_already_set &eas ) {
        Helper::MessageBox(
            QMessageBox::Icon::Critical,
            "Builder python error",
            std::string( eas.what() )
        );
        ErrorReceived = true;
        return;
    }

    if ( Builders.empty() ) {
        ComboPayload->clear();
        ComboPayload->setEnabled( true );
    }

    ComboPayload->addItem( builder.name.c_str() );
    StackedBuilders->addWidget( builder.widget );

    Builders.push_back( builder );
}

auto HcPayloadBuild::generate(
    const std::string& type,
    const json&        profile
) -> std::optional<std::string> {
    auto file = std::string();

    return generate( type, profile, file );
}

auto HcPayloadBuild::generate(
    const std::string& type,
    const json&        profile,
    std::string&       filename,
    const bool         profile_load
) -> std::optional<std::string> {
    auto python_obj = std::optional<py11::object>();
    auto object     = json();
    auto context    = json();
    auto payload    = std::string();

    if ( Builders.empty() ) {
        throw std::runtime_error( "no builder registered" );
    }

    if ( ! ( python_obj = BuilderObject( type ) ).has_value() ) {
        throw std::runtime_error( "specified payload builder does not exist" );
    }

    //
    // generate the configuration for the payload
    //

    auto [status, response] = Havoc->ApiSend( "api/agent/build", {
        { "name",   type    },
        { "config", profile },
    } );

    if ( status != 200 ) {
        //
        // process the response and throw an error
        //

        if ( ( object = json::parse( response ) ).is_discarded() ) {
            throw std::runtime_error( "failed to parse json object: json has been discarded" );
        };

        if ( !object.contains( "error" ) || !object[ "error" ].is_string() ) {
            throw std::runtime_error( "invalid server response: invalid error" );
        };

        throw std::runtime_error( std::format(
            "failed to build payload: {}", object[ "error" ].get<std::string>()
        ) );
    }

    //
    // parse and sanity check the server json response
    //

    if ( ( object = json::parse( response ) ).is_discarded() ) {
        throw std::runtime_error( "failed to parse json object: json has been discarded" );
    };

    if ( !object.contains( "filename" ) || !object[ "filename" ].is_string() ) {
        throw std::runtime_error( "invalid server response: invalid filename" );
    };

    if ( !object.contains( "payload" ) || !object[ "payload" ].is_string() ) {
        throw std::runtime_error( "invalid server response: invalid payload" );
    };

    if ( !object.contains( "context" ) || !object[ "context" ].is_object() ) {
        throw std::runtime_error( "invalid server response: invalid context" );
    };

    //
    // parse the filename, context and decode
    // payload binary from json response object
    //

    filename = object.at( "filename" ).get<std::string>();
    context  = object.at( "context" ).get<json>();
    payload  = object.at( "payload" ).get<std::string>();
    payload  = QByteArray::fromBase64(
        QByteArray::fromStdString( payload )
    ).toStdString();

    //
    // process payload by passing it to python builder instance
    // we are also creating a scope for it so the gil can be released
    // at the end of the scope after finishing interacting with the
    // python builder instance
    //
    {
        HcPythonAcquire();

        if ( !profile.empty() && profile_load ) {
            //
            // we are now going to pass the profile to the builder
            // and the extensions so during payload_process will be
            // taking care of them accordingly
            //

            python_obj.value().attr( "profile_load" )(
               profile
           ).cast<void>();
        }

        //
        // do some post payload processing after retrieving
        // the payload from the remote server plus the entire
        // configuration we have to include into the payload
        //
        return python_obj.value().attr( "payload_process" )(
            py11::bytes( payload.data(), payload.size() ),
            context
        ).cast<std::string>();
    }

    return std::nullopt;
}

auto HcPayloadBuild::clickedGenerate(
    void
) -> void {
    auto data     = json();
    auto body     = json();
    auto config   = json();
    auto name     = ComboPayload->currentText().toStdString();
    auto object   = BuilderObject( name );
    auto filename = std::string();
    auto payload  = std::optional<std::string>();
    auto dialog   = QFileDialog();

    TextBuildLog->clear();
    if ( SplitterTopBottom->sizes()[ 0 ] == 0 ) {
        SplitterTopBottom->setSizes( QList<int>() << 400 << 200 );
    }

    if ( Builders.empty() ) {
        Helper::MessageBox(
            QMessageBox::Critical,
            "Payload Build Failure",
            "failed to build payload: no builder registered"
        );

        return;
    }

    //
    // check if the builder exists
    //
    if ( ! object.has_value() ) {
        Helper::MessageBox(
            QMessageBox::Critical,
            "Payload Build Failure",
            std::format( "specified payload builder does not exist: {}", name )
        );
        return;
    }

    const auto& builder = object.value();

    //
    // scoped sanity check and generation of payload
    // and interacting with the builder instance
    //
    {
        HcPythonAcquire();

        try {
            if ( ! builder.attr( "sanity_check" )().cast<bool>() ) {
                spdlog::debug( "sanity check failed. exit and dont send request" );

                Helper::MessageBox(
                    QMessageBox::Critical,
                    "Payload Build Failure",
                    std::format( "sanity check failed: {}", ComboPayload->currentText().toStdString() )
                );

                return;
            }

            //
            // generate a context or configuration
            // from the builder that we are going to
            // send to the server plugin
            //
            config = builder.attr( "generate" )();
        } catch ( py11::error_already_set &eas ) {
            spdlog::error( "failed to refresh builder \"{}\": \n{}", name, eas.what() );
            return;
        }
    }

    //
    // we are now going to send a request to our api
    // endpoint to receive a payload from the server
    //

    try {
        if ( ! ( payload = generate( name, config, filename, false ) ).has_value() ) {
            Helper::MessageBox(
                QMessageBox::Critical,
                "Payload Build Failure",
                "failed to generate payload"
            );

            return;
        }
    } catch ( std::exception& e ) {
        Helper::MessageBox( QMessageBox::Critical, "Payload Build Failure", e.what() );
        return;
    }

    dialog.setStyleSheet( HcApplication::StyleSheet() );
    dialog.setDirectory( QDir::homePath() );
    dialog.selectFile( QString::fromStdString( filename ) );
    dialog.setAcceptMode( QFileDialog::AcceptSave );

    if ( dialog.exec() == Accepted ) {
        auto path = dialog.selectedFiles().value( 0 );
        auto file = QFile();

        file.setFileName( path );
        if ( file.open( QIODevice::ReadWrite ) ) {
            file.write( QByteArray::fromStdString( payload.value() ) );

            Helper::MessageBox(
                QMessageBox::Information,
                "Payload Build",
                std::format( "saved payload under:\n\n{}", path.toStdString() )
            );
        } else {
            Helper::MessageBox(
                QMessageBox::Critical,
                "Payload Build Failure",
                std::format( "failed to write payload to \"{}\": {}", path.toStdString(), file.errorString().toStdString() )
            );
        }
    }

    return;
}

auto HcPayloadBuild::clickedProfileSave(
    void
) -> void {
    auto Dialog        = new QDialog();
    auto d_gridLayout  = new QGridLayout( Dialog );
    auto d_ProfileName = new HcLineEdit( Dialog );
    auto d_buttonBox   = new QDialogButtonBox( Dialog );
    auto payload_type  = ComboPayload->currentText().toStdString();
    auto profile       = json();

    if ( Builders.empty() ) {
        Helper::MessageBox(
            QMessageBox::Critical,
            "Payload profile failure",
            "failed to add profile: no builder registered"
        );

        return;
    }

    auto object = BuilderObject( payload_type );
    if ( !object.has_value() ) {
        Helper::MessageBox(
            QMessageBox::Critical,
            "Payload build failure",
            std::format( "specified payload builder does not exist: {}", payload_type )
        );
        return;
    }

    Dialog->setWindowTitle( QCoreApplication::translate( "Profile Name", "Dialog", nullptr ) );
    Dialog->setStyleSheet( HcApplication::StyleSheet() );
    Dialog->resize( 330, 70 );

    d_gridLayout->setObjectName( QString::fromUtf8( "gridLayout" ) );

    d_ProfileName->setObjectName( QString::fromUtf8( "d_ProfileName" ) );
    d_ProfileName->setLabelText( "Profile Name:" );

    d_buttonBox->setObjectName( QString::fromUtf8( "d_buttonBox" ) );
    d_buttonBox->setOrientation( Qt::Horizontal );
    d_buttonBox->setStandardButtons( QDialogButtonBox::Cancel | QDialogButtonBox::Ok );

    d_gridLayout->addWidget( d_ProfileName, 0, 0, 1, 1 );
    d_gridLayout->addWidget( d_buttonBox,   1, 0, 1, 1 );

    connect( d_buttonBox, &QDialogButtonBox::accepted, Dialog, &QDialog::accept );
    connect( d_buttonBox, &QDialogButtonBox::rejected, Dialog, &QDialog::reject );

    QMetaObject::connectSlotsByName( Dialog );

    if ( Dialog->exec() == Accepted ) {
        auto found = false;

        if ( d_ProfileName->Input->text().isEmpty() ) {
            Helper::MessageBox(
                QMessageBox::Critical,
                "Payload profile failure",
                "failed to add profile: no profile name specified"
            );

            return;
        }

        try {
            HcPythonAcquire();

            profile = object.value().attr( "profile_save" )().cast<json>();
        } catch ( std::exception& e ) {
            spdlog::error( "profile failure: {}", e.what() );

            Helper::MessageBox(
                QMessageBox::Critical,
                "Profile save failure",
                std::format( "failed to process profile data: \n{}", e.what() )
            );
            return;
        }

        for ( const auto& _profile : Havoc->ProfileQuery( "profile" ) ) {
            if ( !_profile.contains( "name" ) ) {
                continue;
            }

            auto name = toml::find<std::string>( _profile, "name" );
            if ( ! name.empty() ) {
                if ( name == d_ProfileName->Input->text().toStdString() ) {
                    Helper::MessageBox(
                        QMessageBox::Critical,
                        "Payload profile failure",
                        "failed to add profile: profile name already exists"
                    );

                    return;
                }
            }
        }

        //
        // insert the profile name and configuration
        // into the profile file
        //
        auto name = d_ProfileName->text().toStdString();
        Havoc->ProfileInsert( "profile", toml::table {
            { "name",    name           },
            { "type",    payload_type   },
            { "profile", profile.dump() },
        } );

        retranslateUi();
    }

    delete Dialog;
}

auto HcPayloadBuild::clickedProfileLoad(
    void
) -> void {
    auto dialog  = QFileDialog();
    auto path    = QString();
    auto file    = QFile();
    auto object  = json();
    auto name    = std::string();
    auto type    = std::string();
    auto profile = json();

    dialog.setStyleSheet( HcApplication::StyleSheet() );
    dialog.setDirectory( QDir::homePath() );
    dialog.setWindowTitle( "Load Profile" );
    if ( dialog.exec() == Rejected ) {
        return;
    }

    path = dialog.selectedUrls().value( 0 ).toLocalFile();
    if ( path.isNull() ) {
        Helper::MessageBox(
            QMessageBox::Critical,
            "Profile Loading Failure",
            std::format( "failed to load profile: path is empty" )
        );

        return;
    }

    file.setFileName( dialog.selectedUrls().value( 0 ).toLocalFile() );
    if ( !file.exists() ) {
        Helper::MessageBox(
            QMessageBox::Critical,
            "Profile Loading Failure",
            std::format( "failed to load profile:\n\n{}", path.toStdString() )
        );
        return;
    }

    if ( !file.open( QIODevice::ReadOnly | QIODevice::Text ) ) {
        Helper::MessageBox(
            QMessageBox::Critical,
            "Profile Loading Failure",
            std::format( "failed to open profile:\n\n{}", file.errorString().toStdString() )
        );
        return;
    }

    try {
        if ( ( object = json::parse( file.readAll().toStdString() ) ).is_discarded() ) {
            spdlog::debug( "specified profile has been discarded!!" );
            Helper::MessageBox(
                QMessageBox::Critical,
                "Profile Loading Failure",
                std::format( "profile has been discarded while parsing:\n\n{}", path.toStdString() )
            );
            return;
        }
    } catch ( std::exception& e ) {
        spdlog::error( "failed to parse profile:\n{}", e.what() );
        Helper::MessageBox(
            QMessageBox::Critical,
            "Profile Loading Failure",
            std::format( "failed to parse profile:\n\n{}", e.what() )
        );
        return;
    }

    //
    // parse json object and retrieve name, type and profile configuration
    //

    if ( object.contains( "name" ) && object[ "name" ].is_string() ) {
        name = object[ "name" ].get<std::string>();
    } else {
        Helper::MessageBox(
            QMessageBox::Critical,
            "Profile Loading Failure",
            "Invalid Profile: invalid name"
        );
        return;
    }

    if ( object.contains( "type" ) && object[ "type" ].is_string() ) {
        type = object[ "type" ].get<std::string>();
    } else {
        Helper::MessageBox(
            QMessageBox::Critical,
            "Profile Loading Failure",
            "Invalid Profile: invalid type"
        );
        return;
    }

    if ( object.contains( "profile" ) && object[ "profile" ].is_object() ) {
        profile = object[ "profile" ].get<json>();
    } else {
        Helper::MessageBox(
            QMessageBox::Critical,
            "Profile Loading Failure",
            "Invalid Profile: invalid profile configuration"
        );
        return;
    }

    //
    // sanity check if the profile has not been already registered
    //
    for ( const auto& _profile : Havoc->ProfileQuery( "profile" ) ) {
        if ( !_profile.contains( "name" ) ) {
            continue;
        }

        if ( name == toml::find<std::string>( _profile, "name" ) ) {
            Helper::MessageBox(
                QMessageBox::Critical,
                "Payload Loading Failure",
                "failed to load profile: profile name already exists"
            );

            return;
        }
    }

    //
    // insert the profile name and configuration
    // into the profile file
    //
    Havoc->ProfileInsert( "profile", toml::table {
        { "name",    name           },
        { "type",    type           },
        { "profile", profile.dump() },
    } );

    retranslateUi();
}

auto HcPayloadBuild::itemSelectProfile(
    QListWidgetItem *item
) -> void {
    if ( !item ) {
        return;
    }

    auto widget = dynamic_cast<HcProfileItem*>( ListProfiles->itemWidget( item ) );

    if ( Builders.empty() ) {
        Helper::MessageBox(
            QMessageBox::Critical,
            "Payload profile failure",
            "failed to add profile: no builder registered"
        );

        return;
    }

    auto object = BuilderObject( widget->type.toStdString() );
    if ( !object.has_value() ) {
        Helper::MessageBox(
            QMessageBox::Critical,
            "Payload build failure",
            std::format( "specified payload builder does not exist: {}", widget->type.toStdString() )
        );
        return;
    }

    try {
        HcPythonAcquire();
        object.value().attr( "profile_load" )( widget->profile ).cast<void>();
    } catch ( std::exception& e ) {
        spdlog::error( "payload profile loading failure: {}", e.what() );

        Helper::MessageBox(
            QMessageBox::Critical,
            "Profile Loading Failure",
            std::format( "failed to load profile data: \n{}", e.what() )
        );
        return;
    }
}

auto HcPayloadBuild::contextMenuProfile(
    const QPoint& pos
) -> void {
    auto menu = QMenu();
    auto item = ListProfiles->itemAt( pos );

    if ( !item ) {
        return;
    }

    menu.setStyleSheet( HcApplication::StyleSheet() );
    menu.addAction( "Remove" );
    menu.addAction( "Export" );

    auto widget = dynamic_cast<HcProfileItem*>( ListProfiles->itemWidget( item ) );
    auto action = menu.exec( ListProfiles->viewport()->mapToGlobal( pos ) );

    if ( !action ) {
        return;
    }

    if ( action->text() == "Remove" ) {
        int index = 0;
        for ( const auto& _profile : Havoc->ProfileQuery( "profile" ) ) {
            auto name = std::string();

            if ( !_profile.contains( "name" ) ) {
                continue;
            }

            name = toml::find<std::string>( _profile, "name" );
            if ( name == widget->name.toStdString() ) {
                Havoc->ProfileDelete( "profile", index );
                retranslateUi();
                break;
            }

            ++index;
        }
    } else if ( action->text() == "Export" ) {
        auto name    = std::string();
        auto type    = std::string();
        auto profile = json();
        auto found   = false;
        auto dialog  = QFileDialog();
        auto path    = QString();
        auto file    = QFile();

        for ( const auto& _profile : Havoc->ProfileQuery( "profile" ) ) {
            if ( !_profile.contains( "name" ) ||
                 !_profile.contains( "type" ) ||
                 !_profile.contains( "profile" )
            ) {
                continue;
            }

            name = toml::find<std::string>( _profile, "name" );
            type = toml::find<std::string>( _profile, "type" );

            if ( name != widget->name.toStdString() ) {
                continue;
            }

            try {
                if ( ( profile = json::parse( toml::find<std::string>( _profile, "profile" ) ) ).is_discarded() ) {
                    spdlog::debug( "profile from toml entry has been discarded" );
                    return;
                }
            } catch ( std::exception& e ) {
                spdlog::error( "failed to parse profile from toml entry:\n{}", e.what() );
                return;
            }

            found = true;
            break;
        }

        if ( !found ) {
            return;
        }

        dialog.setStyleSheet( HcApplication::StyleSheet() );
        dialog.setDirectory( QDir::homePath() );
        dialog.selectFile( QString::fromStdString( name ) + ".json" );
        dialog.setAcceptMode( QFileDialog::AcceptSave );
        dialog.setWindowTitle( "Save Profile" );

        if ( dialog.exec() == Accepted ) {
            path = dialog.selectedFiles().value( 0 );

            file.setFileName( path );

            if ( file.open( QIODevice::ReadWrite ) ) {
                file.write( QByteArray::fromStdString( json {
                    { "name",    name    },
                    { "type",    type    },
                    { "profile", profile },
                }.dump( 2 ) ) );

                Helper::MessageBox(
                    QMessageBox::Information,
                    "Payload build",
                    std::format( "saved payload profile under:\n\n{}", path.toStdString() )
                );
            } else {
                Helper::MessageBox(
                    QMessageBox::Critical,
                    "Payload build failure",
                    std::format( "Failed to write payload profile to \"{}\": {}", path.toStdString(), file.errorString().toStdString() )
                );
            }
        }
    }
}

auto HcPayloadBuild::EventBuildLog(
    const QString& log
) -> void {
    //
    // add support for coloring texts
    //
    TextBuildLog->append( log );
}

auto HcPayloadBuild::BuilderObject(
    const std::string& name
) -> std::optional<py11::object> {

    for ( int i = 0; i < Builders.size(); i++ ) {
        if ( Builders[ i ].name == name ) {
            return Builders[ i ].instance;
        }
    }

    return std::nullopt;
}

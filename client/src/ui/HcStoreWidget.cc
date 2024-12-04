#include <Havoc.h>
#include <ui/HcStoreWidget.h>

#include <QNetworkAccessManager>
#include <QMouseEvent>
#include <QHBoxLayout>

//
// a small QTextEdit class that handles remote resources such
// as http images and embeds them into the markdown/html renderer
//

class HcTextEdit : public QTextEdit {
public:
    explicit HcTextEdit( QWidget* parent = nullptr ) : QTextEdit( parent ) {}

    QVariant loadResource(
        int         type,
        const QUrl& name
    ) override {
        if ( type == QTextDocument::ImageResource ) {
            //
            // is remote/link image resource
            //
            if ( !name.isLocalFile() ) {
                spdlog::debug( "loadResource( {} )", name.toString().toStdString() );

                auto [status, response, _] = Havoc->RequestSend(
                    name.toString().toStdString(),
                    std::string(),
                    false,
                    "GET",
                    std::string(),
                    false
                );

                return QByteArray::fromStdString( response.value() );
            }
        }

        return QTextEdit::loadResource( type, name );
    }
};

HcMarketPluginItem::HcMarketPluginItem(
    const QString& name,
    const QString& description,
    const QString& group        = "local",
    bool           professional = false,
    QWidget*       parent       = nullptr
) : QWidget( parent ) {
    gridLayout = new QGridLayout( this );
    gridLayout->setObjectName( "gridLayout" );

    LabelName   = new QLabel( this );
    LabelName->setObjectName( "LabelName" );

    auto policy = QSizePolicy( QSizePolicy::Maximum, QSizePolicy::Maximum );
    policy.setHorizontalStretch( 0 );
    policy.setVerticalStretch( 0 );
    policy.setHeightForWidth( LabelName->sizePolicy().hasHeightForWidth() );

    LabelName->setSizePolicy( policy );

    policy     = QSizePolicy( QSizePolicy::Maximum, QSizePolicy::Minimum );
    LabelGroup = new QLabel( this );
    LabelGroup->setObjectName( "LabelGroup" );
    policy.setHorizontalStretch( 0 );
    policy.setVerticalStretch( 0 );
    policy.setHeightForWidth( LabelGroup->sizePolicy().hasHeightForWidth() );
    LabelGroup->setSizePolicy( policy );
    LabelGroup->setText( group );

    if ( group == "local" ) {
        LabelGroup->setProperty( "HcLabelDisplay", "tag" );
    } else {
        LabelGroup->setProperty( "HcLabelDisplay", professional ? "professional" : "cyan" );
    }

    LabelDescription = new QLabel( this );
    LabelDescription->setObjectName( "LabelDescription" );

    policy = QSizePolicy( QSizePolicy::Ignored, QSizePolicy::Preferred );
    policy.setHorizontalStretch( 0 );
    policy.setVerticalStretch( 0 );
    policy.setHeightForWidth( LabelDescription->sizePolicy().hasHeightForWidth() );
    LabelDescription->setSizePolicy( policy );

    InstallLayout = new QGridLayout();
    InstallLayout->setObjectName( u"InstallLayout" );

    Spacer = new QSpacerItem( 40, 20, QSizePolicy::Expanding, QSizePolicy::Minimum );

    gridLayout->addWidget( LabelName, 0, 0, 1, 1 );
    gridLayout->addWidget( LabelGroup, 0, 1, 1, 1 );
    gridLayout->addLayout( InstallLayout, 0, 7, 2, 1 );
    gridLayout->addItem( Spacer, 0, 2, 1, 5 );
    gridLayout->addWidget( LabelDescription, 1, 0, 1, 7 );
    gridLayout->setColumnStretch( 0, 1 );

    LabelDescription->setText( description );
    LabelName->setText( std::format( "<p><span style=\" font-size:12pt;\">{}</span></p>", name.toStdString() ).c_str() );

    QMetaObject::connectSlotsByName( this );
}

void HcMarketPluginItem::setInstalled() {
    if ( LabelInstalled ) {
        setNotInstalled();
    }

    auto policy    = QSizePolicy( QSizePolicy::Maximum, QSizePolicy::Maximum );
    LabelInstalled = new QLabel();
    LabelInstalled->setObjectName( "LabelInstalled" );
    LabelInstalled->setText( "Installed" );
    LabelInstalled->setProperty( "HcLabelDisplay", "green" );
    policy.setHorizontalStretch( 0 );
    policy.setVerticalStretch( 0 );
    policy.setHeightForWidth( LabelInstalled->sizePolicy().hasHeightForWidth() );
    LabelInstalled->setSizePolicy( policy );

    InstallLayout->addWidget( LabelInstalled, 0, 0, 1, 1 );
}

void HcMarketPluginItem::setNotInstalled() {
    if ( LabelInstalled ) {
        InstallLayout->removeWidget( LabelInstalled );
        delete LabelInstalled;
        LabelInstalled = nullptr;
    }
}

class HcListWidgetDeselect : public QListWidget {

public:
    explicit HcListWidgetDeselect( QWidget* parent ) : QListWidget( parent ) { }

    void mousePressEvent( QMouseEvent *event ) override {
        QListWidget::mousePressEvent( event );

        if ( ! indexAt( event->pos() ).isValid() ) {
            clearSelection();
        }
    }
};

HcStoreWidget::HcStoreWidget( QWidget* parent ) : QWidget( parent ) {
    setObjectName( QString::fromUtf8( "HcStoreWidget" ) );

    gridLayout_3 = new QGridLayout( this );
    gridLayout_3->setObjectName( "gridLayout_3" );
    gridLayout_3->setContentsMargins( 0, 0, 0, 0 );

    splitter = new QSplitter( this );
    splitter->setObjectName( "splitter" );
    splitter->setOrientation( Qt::Horizontal );

    MarketPlaceWidget = new QWidget( splitter );
    MarketPlaceWidget->setObjectName( "MarketPlaceWidget" );

    gridLayout = new QGridLayout( MarketPlaceWidget );
    gridLayout->setObjectName( "gridLayout" );

    MarketPlaceSearch = new QLineEdit( MarketPlaceWidget );
    MarketPlaceSearch->setObjectName( "MarketPlaceSearch" );
    MarketPlaceSearch->setProperty( "HcLineEdit", "dark" );

    MarketPlaceList = new HcListWidgetDeselect( MarketPlaceWidget );
    MarketPlaceList->setObjectName( "MarketPlaceList" );
    MarketPlaceList->setProperty( "HcListWidget", "dark" );
    MarketPlaceList->setFocusPolicy( Qt::NoFocus );

    PluginViewStack = new QStackedWidget( splitter );
    PluginViewStack->setObjectName( "PluginViewStack" );

    splitter->addWidget( MarketPlaceWidget );
    splitter->addWidget( PluginViewStack );

    gridLayout->addWidget( MarketPlaceSearch, 0, 0, 1, 1 );
    gridLayout->addWidget( MarketPlaceList, 1, 0, 1, 1 );

    gridLayout_3->addWidget( splitter, 0, 0, 1, 1 );

    PluginWorker.Thread = new QThread;
    PluginWorker.Worker = new HcStorePluginWorker;
    PluginWorker.Worker->moveToThread( PluginWorker.Thread );

    connect( PluginWorker.Thread, &QThread::started, PluginWorker.Worker, &HcStorePluginWorker::run );
    connect( this, &HcStoreWidget::RegisterRepository, PluginWorker.Worker, &HcStorePluginWorker::RegisterRepository );
    connect( this, &HcStoreWidget::RegisterPlugin, PluginWorker.Worker, &HcStorePluginWorker::RegisterPlugin );
    connect( this, &HcStoreWidget::PluginInstall, PluginWorker.Worker, &HcStorePluginWorker::PluginInstall );

    connect( PluginWorker.Worker, &HcStorePluginWorker::AddPlugin, this, &HcStoreWidget::AddPlugin );
    connect( PluginWorker.Worker, &HcStorePluginWorker::MessageBox, this, [&](
        QMessageBox::Icon  icon,
        const std::string& title,
        const std::string& content
    ) {
        Havoc->ui->MessageBox( icon, title, content );
    } );

    connect( PluginWorker.Worker, &HcStorePluginWorker::PluginIsInstalling, this, [&]( PluginView* plugin ) {
        plugin->ButtonInstall->setText( "Installing..." );
        plugin->ButtonInstall->setProperty( "HcButtonInstall", "installing" );
        plugin->ButtonInstall->setDisabled( true );
        plugin->ButtonInstall->style()->polish( plugin->ButtonInstall );
    } );

    connect( PluginWorker.Worker, &HcStorePluginWorker::PluginIsInstalled, this, [&]( PluginView* plugin ) {
        plugin->ButtonInstall->setEnabled( true );

        if ( plugin->plugin_dir.exists() ) {
            auto script = QFile( plugin->plugin_dir.path() + "/plugin.py" );

            ( ( HcMarketPluginItem* ) plugin->ListWidget )->setInstalled();

            plugin->ButtonInstall->setText( "Uninstall" );
            plugin->ButtonInstall->setProperty( "HcButtonInstall", "uninstall" );

            if ( script.exists() ) {
                try {
                    HcPythonAcquire();
                    Havoc->ui->PageScripts->LoadScript( script.fileName().toStdString() );
                } catch ( py11::error_already_set &eas ) {
                    Helper::MessageBox( QMessageBox::Critical, "plugin installation", std::format( "failed while loading {} plugin scripts:\n{}", plugin->name, eas.what() ) );
                    spdlog::error( "plugin error while loading {} python scripts:\n{}", plugin->name, eas.what() );
                }
            }
        } else {
            ( ( HcMarketPluginItem* ) plugin->ListWidget )->setNotInstalled();

            if ( ! plugin->repo.empty() ) {
                //
                // can be installed from a remote repository again
                //
                plugin->ButtonInstall->setText( "Install" );
                plugin->ButtonInstall->setProperty( "HcButtonInstall", "install" );
            } else {
                //
                // was locally added/installed
                //
                plugin->ButtonInstall->setEnabled( false );
                plugin->ButtonInstall->setText( "Removed" );
                plugin->ButtonInstall->setProperty( "HcButtonInstall", "installing" );
            }
        }
        plugin->ButtonInstall->style()->polish( plugin->ButtonInstall );
    } );

    connect( MarketPlaceSearch, &QLineEdit::textEdited, this, &HcStoreWidget::QueryPluginMarket );
    connect( MarketPlaceList, &QListWidget::itemClicked, this, [&]( QListWidgetItem* item ){
        PluginViewStack->setCurrentIndex( item->listWidget()->row( item ) );
    });

    QMetaObject::connectSlotsByName( this );
}

auto HcStoreWidget::AddPlugin(
    const std::string& parent,
    const std::string& repo,
    PluginView*        plugin
) -> void {
    auto project_url = repo;
    auto categories  = std::vector<std::string>();
    auto description = std::string();
    auto version     = std::string();
    auto license     = std::string();
    auto readme      = std::string();
    auto author_name = std::string();
    auto author_url  = std::string();

    //
    // parse the json object containing the plugin information
    // such as name, description, readme, categories, etc.
    //

    if ( plugin->object.contains( "description" ) && plugin->object[ "description" ].is_string() ) {
        description = plugin->object[ "description" ].get<std::string>();
    } else {
        spdlog::error( "AddPlugin error: failed to retrieve plugin description (either not found or not a string)" );
        plugin->mutex.unlock();
        return;
    }

    if ( plugin->object.contains( "readme" ) && plugin->object[ "readme" ].is_string() ) {
        readme = plugin->object[ "readme" ].get<std::string>();
    } else {
        spdlog::error( "AddPlugin error: failed to retrieve plugin readme (either not found or not a string)" );
        plugin->mutex.unlock();
        return;
    }

    if ( plugin->object.contains( "author" ) && plugin->object[ "author" ].is_object() ) {
        if ( plugin->object[ "author" ].get<json>().contains( "name" ) &&
             plugin->object[ "author" ].get<json>()[ "name" ].is_string()
        ) {
            author_name = plugin->object[ "author" ].get<json>()[ "name" ].get<std::string>();
        } else {
            spdlog::error( "AddPlugin error: failed to retrieve plugin author name (either not found or not a string)" );
            plugin->mutex.unlock();
            return;
        }

        if ( plugin->object[ "author" ].get<json>().contains( "url" ) &&
             plugin->object[ "author" ].get<json>()[ "url" ].is_string()
        ) {
            author_url = plugin->object[ "author" ].get<json>()[ "url" ].get<std::string>();
        } else {
            spdlog::error( "AddPlugin error: failed to retrieve plugin author url (either not found or not a string)" );
            plugin->mutex.unlock();
            return;
        }
    } else {
        spdlog::error( "AddPlugin error: failed to retrieve plugin author (either not found or not an object)" );
        plugin->mutex.unlock();
        return;
    }

    if ( plugin->object.contains( "categories" ) && plugin->object[ "categories" ].is_array() ) {
        for ( auto & category : plugin->object[ "categories" ].get<std::vector<json>>() ) {
            if ( ! category.is_string() ) {
                spdlog::warn( "AddPlugin warning: plugin category is not a string" );
                continue;
            }

            categories.push_back( category.get<std::string>() );
        }
    }

    //
    // create the Qt objects
    //

    plugin->Widget     = new QWidget( PluginViewStack );
    plugin->GridLayout = new QGridLayout( plugin->Widget );
    plugin->GridLayout->setObjectName( "PluginGridLayout" );

    plugin->LayoutCategories = new QHBoxLayout();
    plugin->LayoutCategories->setObjectName( "PluginLayoutCategories" );

    for ( auto& category : categories ) {
        auto tag = new QLabel( category.c_str() );

        tag->setProperty( "HcLabelDisplay", "tag" );
        tag->setSizePolicy( QSizePolicy::Fixed, QSizePolicy::Minimum );

        plugin->LayoutCategories->addWidget( tag );
    }

    plugin->ButtonInstall = new QPushButton( plugin->Widget );
    plugin->ButtonInstall->setObjectName( "PluginButtonInstall" );
    plugin->ButtonInstall->setText( "Install" );
    plugin->ButtonInstall->setProperty( "HcButtonInstall", "install" );
    plugin->ButtonInstall->setFixedWidth( 120 );

    plugin->LabelName = new QLabel( plugin->Widget );
    plugin->LabelName->setObjectName( "PluginLabelName" );
    plugin->LabelName->setOpenExternalLinks( true );

    plugin->TextReadme = new HcTextEdit( plugin->Widget );
    plugin->TextReadme->setObjectName( "PluginTextReadme" );
    plugin->TextReadme->setProperty( "HcConsole", "true" );
    plugin->TextReadme->setReadOnly( true );

    plugin->LabelDescription = new QLabel( plugin->Widget );
    plugin->LabelDescription->setObjectName( "PluginLabelDescription" );

    plugin->Spacers[ 0 ] = new QSpacerItem( 40, 20, QSizePolicy::Expanding, QSizePolicy::Minimum );
    plugin->Spacers[ 1 ] = new QSpacerItem( 40, 20, QSizePolicy::Expanding, QSizePolicy::Minimum );
    plugin->Spacers[ 2 ] = new QSpacerItem( 40, 20, QSizePolicy::Expanding, QSizePolicy::Minimum );

    plugin->LabelAuthor = new HcLabelNamed( "Author:", std::format( "<a href=\"{}\" style=\"color: #8BE9FD; text-decoration:none\"><span>{}</span></a>", author_url, author_name ).c_str(), plugin->Widget );
    plugin->LabelAuthor->Text->setObjectName( "PluginLabelAuthor" );
    plugin->LabelAuthor->Text->setProperty( "HcLabelLink", true );
    plugin->LabelAuthor->Text->setOpenExternalLinks( true );
    plugin->LabelAuthor->Text->setFixedHeight( 25 );
    plugin->LabelAuthor->Label->setFixedHeight( 25 );
    plugin->LabelAuthor->Label->setFixedWidth( 70 );

    plugin->GridLayout->addWidget( plugin->LabelName, 0, 0, 1, 1 );
    plugin->GridLayout->addItem( plugin->Spacers[ 0 ], 0, 1, 1, 5 );
    plugin->GridLayout->addWidget( plugin->LabelAuthor, 1, 0, 1, 1 );
    plugin->GridLayout->addLayout( plugin->LayoutCategories, 1, 1, 1, 1 );
    plugin->GridLayout->addItem( plugin->Spacers[ 1 ], 1, 2, 1, 4 );
    plugin->GridLayout->addWidget( plugin->LabelDescription, 2, 0, 1, 6 );
    plugin->GridLayout->addWidget( plugin->ButtonInstall, 3, 0, 1, 1 );
    plugin->GridLayout->addItem( plugin->Spacers[ 2 ], 3, 1, 1, 5 );
    plugin->GridLayout->addWidget( plugin->TextReadme, 4, 0, 1, 6 );

    //
    // display the retrieved plugin information
    //

    plugin->LabelName->setText( std::format( "<a href=\"{}\" style=\"color: #8BE9FD; text-decoration:none; font-size:18pt;\"><span>{}</span></a>", project_url, plugin->name ).c_str() );
    plugin->LabelDescription->setText( description.c_str() );
    plugin->TextReadme->setMarkdown( QString::fromStdString( readme ) );

    //
    // NOTE: forcefully trigger an loadResource call to
    //       retrieve and load the resources (images, etc.)
    //       from the external servers and repaint the UI
    //

    try {
        auto pixmap = QPixmap( plugin->TextReadme->size() );
        pixmap.fill( Qt::transparent );

        auto painter = QPainter( &pixmap );
        if ( !painter.isActive() ) {
            spdlog::warn( "painter could not be initialized!" );
            return;
        }

        plugin->TextReadme->render( &painter );
    } catch ( ... ) {
        //
        // NOTE: I don't really care about the exception.
        //       just don't crash the entire application
        //       because you failed to make my hacky fix run lol
        //
    }

    Havoc->ui->repaint();

    //
    // add plugin object to the vector array and to the stack widget
    //

    Plugins.push_back( plugin );
    PluginViewStack->addWidget( plugin->Widget );

    //
    // insert the plugin into the market list
    //
    auto group = std::string( "local" );
    if ( !repo.empty() ) {
        //
        // is remote repository plugin
        //
        group = parent.substr( parent.find_last_of( "/" ) + 1 );
    }

    AddPluginToMarketList( plugin, group );

    if ( ( plugin->plugin_dir.exists() ) ) {
        //
        // if the path/directory exists
        // then it means we installed it
        //
        dynamic_cast<HcMarketPluginItem*>( plugin->ListWidget )->setInstalled();
        plugin->ButtonInstall->setText( "Uninstall" );
        plugin->ButtonInstall->setProperty( "HcButtonInstall", "uninstall" );
        plugin->ButtonInstall->style()->polish( plugin->ButtonInstall );
    }

    connect( plugin->ButtonInstall, &QPushButton::clicked, this, [&]() {
        emit PluginInstall( Plugins.at( PluginViewStack->currentIndex() ) );
    } );

    //
    // each time a plugin gets added revert
    // back to the first plugin to be viewed
    //

    MarketPlaceList->setCurrentRow( 0 );
    PluginViewStack->setCurrentIndex( 0 );

    plugin->mutex.unlock();
}

auto HcStoreWidget::AddPluginToMarketList(
    PluginView*        plugin,
    const std::string& group
) -> void {
    plugin->ListItem   = new QListWidgetItem;
    plugin->ListWidget = new HcMarketPluginItem(
        plugin->object[ "name" ].get<std::string>().c_str(),
        plugin->object[ "description" ].get<std::string>().c_str(),
        group.c_str(),
        ! plugin->access_token.empty(),
        this
    );

    plugin->ListItem->setSizeHint( plugin->ListWidget->sizeHint() );

    MarketPlaceList->addItem( plugin->ListItem );
    MarketPlaceList->setItemWidget( plugin->ListItem, plugin->ListWidget );
}

auto HcStoreWidget::QueryPluginMarket(
    const QString& query
) -> void {
    //
    // TODO: do tokenized search and string search
    //       tokenized (max results / less accurate):
    //          "unguard-eat by 5pider" -> "unguard-eat" "by" "5pider"
    //          and each token will be compared against the entire pluging metadata
    //       complete string search (less results / max accurate):
    //          the string "unguard-eat by 5pider" will be searched across plugins
    //          if they contain the exact string or not
    //

    MarketPlaceList->clear();

    if ( query.isEmpty() ) {
        for ( auto& plugin : Plugins ) {
            AddPluginToMarketList( plugin );
        }
        return;
    }

    for ( auto& plugin : Plugins ) {
        if ( PluginQueryContainMeta( plugin, query ) ) {
            AddPluginToMarketList( plugin );
        }
    }
}

auto HcStoreWidget::PluginQueryContainMeta(
    const PluginView* plugin,
    const QString&    query
) -> bool {
    auto name        = plugin->object[ "name" ].get<std::string>();
    auto description = plugin->object[ "description" ].get<std::string>();
    auto author      = plugin->object[ "author" ][ "name" ].get<std::string>();
    auto search      = query.toStdString();

    if ( search.find( name ) != std::string::npos ) {
        return true;
    }

    if ( search.find( description ) != std::string::npos ) {
        return true;
    }

    if ( search.find( author ) != std::string::npos ) {
        return true;
    }

    if ( plugin->object.contains( "categories" ) && plugin->object[ "categories" ].is_array() ) {
        for ( auto & category : plugin->object[ "categories" ].get<std::vector<json>>() ) {
            if ( ! category.is_string() ) {
                continue;
            }

            if ( search.find( category ) != std::string::npos ) {
                return true;
            }
        }
    }

    return false;
}

auto HcStoreWidget::HttpGet(
    const std::string& url,
    const std::string& authorization
) -> std::optional<std::string> {
    auto base64  = std::string();
    auto headers = json::array();

    if ( ! authorization.empty() ) {
        base64  = QByteArray::fromStdString( authorization ).toBase64().toStdString();
        headers = {
            { { "Authorization", std::format( "Basic {}", base64 ) } },
        };
    }

    auto [status, response, _] = Havoc->RequestSend( url, "", false, "GET", "", false, headers );

    return status != 404 ? response : std::nullopt;
}

#include <Common.h>
#include <Havoc.h>

HcSessionGraph::HcSessionGraph(
    QWidget* parent
) : QGraphicsView( parent ) {
    _scene = new HcSessionGraphScene( 10, this );
    _scene->setItemIndexMethod( QGraphicsScene::BspTreeIndex );

    setDragMode( RubberBandDrag );
    setScene( _scene );
    setCacheMode( CacheBackground );
    setViewportUpdateMode( BoundingRectViewportUpdate );
    setTransformationAnchor( AnchorUnderMouse );
    setRenderHint( QPainter::Antialiasing );
    scaleView( 1 );

    box_layout = new QGridLayout( this );
    _settings  = new HcSessionGraphSetting( this );

    box_layout->setAlignment( Qt::AlignLeft | Qt::AlignTop );
    box_layout->addWidget( _settings );

    // server = new HcSessionGraphItem( nullptr, this );
    // _scene->addItem( server );
    // server->setPos( 100, 50 );
    //
    // _nodes.push_back( server );
}

HcSessionGraph::~HcSessionGraph() {
    delete _scene;
}

auto HcSessionGraph::scaleView(
    qreal scaleFactor
) -> void {
    const auto factor = transform().scale(
        scaleFactor,
        scaleFactor
    ).mapRect( QRectF( 0, 0, 1, 1 ) ).width();

    if ( factor < 1 || factor > 50 ) {
        return;
    }

    scale( scaleFactor, scaleFactor );
}

auto HcSessionGraph::addAgent(
    HcAgent *agent
) -> HcSessionGraphItem* {
    const auto item  = new HcSessionGraphItem( agent, this );

    if ( !server ) {
        server = new HcSessionGraphItem( nullptr, this );

        _scene->addItem( server );

        _nodes.push_back( server );
    }

    if ( agent->parent().empty() ) {
        item->setItemEdge( new HcSessionGraphEdge( server, item, Havoc->Theme.getGreen() ) );
        item->addParent( server );

        server->addPivot( item );
        _scene->addItem( item->itemEdge() );

        //
        // connect all pivots to itself
        for ( const auto n : _nodes ) {
            if ( isServer( n ) ) {
                continue;
            }

            if ( n->agent()->parent() == agent->uuid() ) {
                n->setItemEdge( new HcSessionGraphEdge( item, n, Havoc->Theme.getPurple() ) );
                n->addParent( item );

                item->addPivot( n );
                _scene->addItem( n->itemEdge() );
            }
        }
    } else {
        if ( const auto agent_parent = Havoc->Agent( agent->parent() ); agent_parent.has_value() ) {
            const auto node = agent_parent.value()->ui.node;

            item->setItemEdge( new HcSessionGraphEdge( node, item, Havoc->Theme.getPurple() ) );
            item->addParent( node );

            node->addPivot( item );

            _scene->addItem( item->itemEdge() );
        } else {
            spdlog::debug( "failed to add agent {}: parent has not been found or registered {}", agent->uuid(), agent->parent() );
        }
    }

    _scene->addItem( item );
    _nodes.push_back( item );

    HcGraphLayoutTree::draw( server );

    return item;
}

auto HcSessionGraph::removeAgent(
    HcAgent* agent
) -> void {
    spdlog::debug( "[HcSessionGraph::removeAgent] {}", agent->uuid() );

    //
    // remove the items from the graph
    _scene->removeItem( agent->ui.node );
    _scene->removeItem( agent->ui.node->itemEdge() );
    _scene->removeItem( agent->ui.node->itemInfo() );

    //
    // remove itself from the parent node
    if ( agent->ui.node->parent() ) {
        agent->ui.node->parent()->removePivot( agent->ui.node );
    }

    //
    // remove the node from the lists
    for ( int i = 0; i < _nodes.size(); i++ ) {
        if ( _nodes[ i ] == agent->ui.node ) {
            _nodes.erase( _nodes.begin() + i );
            break;
        }
    }

    //
    // remove pivots connections
    for ( const auto pivot : agent->ui.node->pivots() ) {
        _scene->removeItem( pivot->itemEdge() );
    }

    //
    // re-draw the tree layout
    HcGraphLayoutTree::draw( server );
}

auto HcSessionGraph::isServer(
    const HcSessionGraphItem* item
) const -> bool {
    return item == server;
}

auto HcSessionGraph::itemMoved() -> void
{
    if ( !timer_id ) {
        timer_id = startTimer( 40 );
    }
}

auto HcSessionGraph::nodes() -> std::vector<HcSessionGraphItem*>
{
    return _nodes;
}

auto HcSessionGraph::settings() -> HcSessionGraphSetting*
{
    return _settings;
}

auto HcSessionGraph::keyPressEvent(
    QKeyEvent* event
) -> void {
    switch ( event->key() ) {
        case Qt::Key_Plus: {
            zoomIn();
            break;
        }

        case Qt::Key_Minus: {
            zoomOut();
            break;
        }

        default: {
            QGraphicsView::keyPressEvent( event );
        }
    }
}

void HcSessionGraph::resizeEvent(
    QResizeEvent* event
) {
    _scene->setSceneRect( 0, 0, event->size().width(), event->size().height() );

    QGraphicsView::resizeEvent( event );
}

void HcSessionGraph::wheelEvent(
    QWheelEvent* event
) {
    if ( QApplication::keyboardModifiers() & Qt::ShiftModifier ) {
        horizontalScrollBar()->event( event );
    } else if ( QApplication::keyboardModifiers() & Qt::ControlModifier ) {
        scaleView( pow( 2., event->angleDelta().y() / 500.0 ) );
    } else {
        verticalScrollBar()->event( event );
    }

    event->ignore();
}

void HcSessionGraph::timerEvent(
    QTimerEvent* event
) {
    bool itemsMoved = false;

    for ( const auto item : nodes() ) {
        item->calculateForces();

        itemsMoved = item->advancePosition();

        item->adjust();
    }

    if ( ! itemsMoved ) {
        killTimer( timer_id );
        timer_id = 0;
    }
}

void HcSessionGraph::zoomIn() {
    scaleView( qreal( 1.2 ) );
}

void HcSessionGraph::zoomOut() {
    scaleView( 1 / qreal( 1.2 ) );
}

//
// HcGraphLayoutTree
//

double HcGraphLayoutTree::X_SEP = 220; // Horizontal separation between levels of the tree
double HcGraphLayoutTree::Y_SEP = 120; // Vertical separation between sibling nodes

auto HcGraphLayoutTree::initNode(
    HcSessionGraphItem* item
) -> void {
    item->Modifier = 0;
    item->Thread   = nullptr;
    item->Ancestor = item;

    for ( const auto w : item->pivots() ) {
        initNode( w );
    }
}

auto HcGraphLayoutTree::draw(
    HcSessionGraphItem* item
) -> void {
    initNode( item );
    firstWalk( item );
    secondWalk( item , 100, 0 );
}

auto HcGraphLayoutTree::firstWalk(
    HcSessionGraphItem* item
) -> void {
    if ( item->pivots().empty() ) {
        if ( item->parent() && item != item->parent()->pivots()[ 0 ] ) {
            const auto children = item->parent()->pivots();
            const auto sibling  = *std::prev( std::find( children.cbegin(), children.cend(), item ) );

            item->Prelim = sibling->Prelim + Y_SEP;
        } else {
            item->Prelim = 0;
        }
    } else {
        auto defaultAncestor = item->pivots()[ 0 ];

        for ( const auto w : item->pivots() ) {
            firstWalk( w );
            apportion( w, defaultAncestor );
        }

        executeShifts( item );

        const double midpoint = ( item->pivots()[ 0 ]->Prelim + item->pivots().back()->Prelim ) / 2;

        if ( item->parent() && item != item->parent()->pivots()[ 0 ] ) {
            const auto children = item->parent()->pivots();
            const auto sibling  = *std::prev( std::find( children.cbegin(), children.cend(), item ) );

            item->Prelim   = sibling->Prelim + Y_SEP;
            item->Modifier = item->Prelim - midpoint;
        } else {
            item->Prelim = midpoint;
        }
    }
}

auto HcGraphLayoutTree::apportion(
    HcSessionGraphItem*  item,
    HcSessionGraphItem*& defaultAncestor
) -> void {
    if ( item != item->parent()->pivots()[ 0 ] ) {
        const auto children = item->parent()->pivots();
        const auto sibling  = *std::prev( std::find( children.cbegin(), children.cend(), item ) );

        auto vip = item;
        auto vop = item;
        auto vim = sibling;
        auto vom = vip->parent()->pivots()[ 0 ];

        double sip = vip->Modifier;
        double sop = vop->Modifier;
        double sim = vim->Modifier;
        double som = vom->Modifier;

        while ( nextRight( vim ) && nextLeft( vip ) ) {
            vim = nextRight( vim );
            vip = nextLeft( vip );
            vom = nextLeft( vom );
            vop = nextRight( vop );

            vop->Ancestor = item;

            double shift = ( vim->Prelim + sim ) - ( vip->Prelim + sip ) + Y_SEP;

            if ( shift > 0 ) {
                moveSubtree( ancestor( vim, item, defaultAncestor ), item, shift );
                sip += shift;
                sop += shift;
            }

            sim += vim->Modifier;
            sip += vip->Modifier;
            som += vom->Modifier;
            sop += vop->Modifier;
        }

        if ( nextRight( vim ) && ! nextRight( vop ) ) {
            vop->Thread    = nextRight( vim );
            vop->Modifier += sim - sop;
        }

        if ( nextLeft( vip ) && ! nextLeft( vom ) ) {
            vom->Thread     = nextLeft( vip );
            vom->Modifier  += sip - som;
            defaultAncestor = item;
        }
    }
}

auto HcGraphLayoutTree::moveSubtree(
    HcSessionGraphItem* wm,
    HcSessionGraphItem* wp,
    double              shift
) -> void {
    auto children = wm->parent()->pivots();
    auto wmIndex  = std::distance( children.cbegin(), std::find( children.cbegin(), children.cend(), wm ) );
    auto wpIndex  = std::distance( children.cbegin(), std::find( children.cbegin(), children.cend(), wp ) );
    int  subtrees = wpIndex - wmIndex;

    if ( subtrees != 0 ) {
        wp->Change -= shift / subtrees;
        wp->Shift += shift;
        wm->Change += shift / subtrees;
        wp->Prelim += shift;
        wp->Modifier += shift;
    }
}

auto HcGraphLayoutTree::nextLeft(
    HcSessionGraphItem* v
) -> HcSessionGraphItem* {
    return ( ! v->pivots().empty() ) ? v->pivots()[ 0 ] : v->Thread;
}

auto HcGraphLayoutTree::nextRight(
    HcSessionGraphItem* v
) -> HcSessionGraphItem* {
    return ( ! v->pivots().empty() ) ? v->pivots().back() : v->Thread;
}

auto HcGraphLayoutTree::ancestor(
    HcSessionGraphItem*  vim,
    HcSessionGraphItem*  v,
    HcSessionGraphItem*& defaultAncestor
) -> HcSessionGraphItem * {
    return ( vim->Ancestor->parent() == v->parent() ) ? vim->Ancestor : defaultAncestor;
}

auto HcGraphLayoutTree::executeShifts(
    HcSessionGraphItem* item
) -> void {
    double shift = 0;
    double change = 0;

    for ( int i = item->pivots().size() - 1; i >= 0; i-- ) {
        auto w = item->pivots()[i];
        w->Prelim += shift;
        w->Modifier += shift;
        change += w->Change;
        shift += w->Shift + change;
    }
}

auto HcGraphLayoutTree::secondWalk(
    HcSessionGraphItem* item,
    double              m,
    double              depth
) -> void {
    item->setPos( ( depth * X_SEP ) + 100, item->Prelim + m );

    for ( const auto w : item->pivots() ) {
        secondWalk( w, m + item->Modifier, depth + 1 );
    }
}

//
// HcSessionGraphScene
//

HcSessionGraphScene::HcSessionGraphScene(
    int      grid_size,
    QObject* parent
) : QGraphicsScene( parent ), grid_size( grid_size ) {}

HcSessionGraphScene::~HcSessionGraphScene() = default;

auto HcSessionGraphScene::mouseMoveEvent(
    QGraphicsSceneMouseEvent* event
) -> void {
    QGraphicsScene::mouseMoveEvent( event );

    if ( const auto item = mouseGrabberItem() ) {
        const auto pos = item->pos();

        const auto snappedX = round( pos.x() / grid_size ) * grid_size;
        const auto snappedY = round( pos.y() / grid_size ) * grid_size;

        item->setPos( snappedX, snappedY );
    }
}

auto HcSessionGraphScene::contextMenuEvent(
    QGraphicsSceneContextMenuEvent *event
) -> void {
    auto sessions   = selectedItems();
    auto menu       = QMenu();
    auto actions    = Havoc->Actions( HcApplication::ActionObject::ActionAgent );
    auto agent_type = std::string();

    //
    // retrieve the list of selected current
    // items from the graph scene
    //
    if ( sessions.empty() ) {
        //
        // instead we are just going to retrieve
        // the current hovered over items
        //
        if ( ( sessions = items( event->scenePos() ) ).empty() ) {
            //
            // no items have been selected
            //
            return QGraphicsScene::contextMenuEvent( event );
        };
    }

    //
    // since there are item selected we
    // can dispatch and handle the event
    //
    menu.setStyleSheet( HcApplication::StyleSheet() );
    if ( sessions.count() > 1 ) {
        menu.addAction( QIcon( ":/icons/16px-agent-console" ), "Interact" );
        menu.addSeparator();
        menu.addAction( QIcon( ":/icons/16px-blind-white" ), "Hide" );
        menu.addAction( QIcon( ":/icons/16px-remove" ), "Remove" );
    } else {
        //
        // if a single selected agent item then try
        // to add the registered actions as well
        //
        menu.addAction( QIcon( ":/icons/16px-agent-console" ), "Interact" );
        menu.addSeparator();

        for ( const auto& _session : sessions ) {
            if ( ( _session->type() == HcSessionGraphItem::Type ) ) {
                if ( const auto session = static_cast<HcSessionGraphItem*>( _session );
                     session->agent()
                ) {
                    agent_type = session->agent()->type();
                    break;
                }
            }
        }

        //
        // add the registered agent type actions
        //
        for ( const auto action : actions ) {
            if ( action->agent.type == agent_type ) {
                if ( action->icon.isNull() ) {
                    menu.addAction( action->name.c_str() );
                } else {
                    menu.addAction( action->icon, action->name.c_str() );
                }
            }
        }

        menu.addSeparator();
        menu.addAction( QIcon( ":/icons/16px-blind-white" ), "Hide" );
        menu.addAction( QIcon( ":/icons/16px-remove" ), "Remove" );
    }

    //
    // check if only the server item has been selected,
    // if it is the only selected item and abort and do
    // not show the menu
    //
    if ( sessions.count() == 1 ) {
        auto session = sessions.at( 0 );
        if ( session->type() == HcSessionGraphItem::Type ) {
            if ( ! ( ( HcSessionGraphItem* ) session )->agent() ) {
                return;
            }
        } else {
            return;
        }
    }

    //
    // show menu if everything went well
    //
    const auto action = menu.exec( event->screenPos() );
    if ( ! action ) {
        return;
    }

    //
    // iterate over all selected sessions
    //
    for ( const auto& _session : sessions ) {
        if ( ( _session->type() == HcSessionGraphItem::Type ) ) {
            const auto session = dynamic_cast<HcSessionGraphItem*>( _session );

            if ( session->agent() ) {
                spdlog::debug( "session uuid: {}", session->agent()->uuid() );

                if ( action->text().compare( "Interact" ) == 0 ) {
                    Havoc->ui->PageAgent->spawnAgentConsole( session->agent()->uuid() );
                } else if ( action->text().compare( "Remove" ) == 0 ) {
                    session->agent()->remove();
                } else if ( action->text().compare( "Hide" ) == 0 ) {
                    // session->agent()->hide();
                    session->agent()->setHidden( true );
                } else {
                    for ( const auto agent_action : actions ) {
                        if ( agent_action->name       == action->text().toStdString() &&
                             agent_action->agent.type == session->agent()->type()
                        ) {
                            const auto agent = Havoc->Agent( session->agent()->uuid() );
                            if ( agent.has_value() &&
                                 agent.value()->interface().has_value()
                            ) {
                                if ( agent_action->callback ) {
                                    reinterpret_cast<HcFnCallbackCtx<std::string>>( agent_action->callback )(
                                        agent.value()->uuid()
                                    );
                                } else {
                                    try {
                                        HcPythonAcquire();

                                        agent_action->callback_py( agent.value()->interface().value() );
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
    }
}

//
// HcSessionGraphItemInfo
//

HcSessionGraphItemInfo::HcSessionGraphItemInfo(
    const QString& text_top,
    const QString& text_bottom
) : text_top( text_top ), text_bottom( text_bottom ) {

}

HcSessionGraphItemInfo::~HcSessionGraphItemInfo() = default;

auto HcSessionGraphItemInfo::boundingRect() const -> QRectF {
    return QRectF( -40, -50, text_bottom.length() * 8, 45 );
}

auto HcSessionGraphItemInfo::paint(
    QPainter*                       painter,
    const QStyleOptionGraphicsItem* option,
    QWidget*                        widget
) -> void {
    painter->setPen( Havoc->Theme.getForeground() );
    painter->drawText( boundingRect(), Qt::AlignCenter | Qt::AlignTop,    text_top    );
    painter->drawText( boundingRect(), Qt::AlignCenter | Qt::AlignBottom, text_bottom );
}

//
// HcSessionGraphItem
//

HcSessionGraphItem::HcSessionGraphItem(
    HcAgent*        agent,
    HcSessionGraph* graph
) : _agent( agent ), _graph( graph ) {
    setFlag( ItemIsMovable );
    setFlag( ItemSendsGeometryChanges );
    setFlag( ItemIsSelectable );

    setCacheMode( DeviceCoordinateCache );
    setZValue( -1 );

    rect = QRectF( -40, -50, 80, 80 );

    if ( agent ) {
        info = new HcSessionGraphItemInfo( agent->uuid().c_str(), std::format(
            "{} @ {}\\{}",
            agent->ui.table.Username->text().toStdString(),
            agent->ui.table.ProcessName->text().toStdString(),
            agent->ui.table.ProcessId->text().toStdString()
        ).c_str() );
        _graph->scene()->addItem( info );
    }
}

HcSessionGraphItem::~HcSessionGraphItem() {
    scene()->removeItem( info );
};

auto HcSessionGraphItem::graph(
    void
) const -> HcSessionGraph* {
    return _graph;
}

auto HcSessionGraphItem::agent(
    void
) const -> HcAgent* {
    return _agent;
}

auto HcSessionGraphItem::addPivot(
    HcSessionGraphItem *agent
) -> void {
    _children.push_back( agent );
}

auto HcSessionGraphItem::removePivot(
    const HcSessionGraphItem* agent
) -> void {
    //
    // remove the agent entry from
    // the children/pivots list
    //
    for ( int i = 0; i < _children.size(); i++ ) {
        if ( _children[ i ] == agent ) {
            _children.erase( _children.begin() + i );
            break;
        }
    }
}

auto HcSessionGraphItem::pivots(
    void
) const -> std::vector<HcSessionGraphItem*> {
    return _children;
}

auto HcSessionGraphItem::addParent(
    HcSessionGraphItem* parent
) -> void {
    _parent = parent;
}

auto HcSessionGraphItem::parent(
    void
) const -> HcSessionGraphItem* {
    return _parent;
}

auto HcSessionGraphItem::addEdge(
    HcSessionGraphEdge* edge
) -> void {
    edges.push_back( edge );
    edge->adjust();
}

auto HcSessionGraphItem::adjust() -> void
{
    for ( const auto _edge : edges ) {
        _edge->adjust();
    }
}

auto HcSessionGraphItem::setItemEdge(
    HcSessionGraphEdge* edge
) -> void {
    this->edge = edge;
}

auto HcSessionGraphItem::itemEdge(
    void
) -> HcSessionGraphEdge* {
    return edge;
}

auto HcSessionGraphItem::itemInfo() -> HcSessionGraphItemInfo*
{
    return info;
}

auto HcSessionGraphItem::boundingRect(
    void
) const -> QRectF {
    return rect;
}

auto HcSessionGraphItem::paint(
    QPainter* painter,
    const QStyleOptionGraphicsItem* option,
    QWidget* widget
) -> void {
    if ( graph()->isServer( this ) ) {
        //
        // check if the current graph item is the root item
        // in this scene (aka havoc server)
        //
        painter->drawImage( rect, QImage( ":/graph/server" ) );
    } else {
        // TODO: reintroduce this once we allow pivot/direct based icons
        //
        // if ( !graph()->isServer( parent() ) ) {
        //     //
        //     // check if the parent is not the server. if no then
        //     // it means that the current item is a pivot session
        //     //
        // } else {
        //     //
        //     // it's a direct session connection
        //     // via no pivoting agent
        //     //
        // }

        painter->drawImage( rect, _agent->image() );
    }

    //
    // if the item has been selected then draw dashed boarder
    // around the item to indicate it has been selected
    //
    if ( isSelected() ) {
        painter->setPen( QPen( QBrush( Havoc->Theme.getOrange() ), 1, Qt::DashLine ) );
        painter->drawRect( boundingRect() );
    }
}

auto HcSessionGraphItem::itemChange(
    GraphicsItemChange change,
    const QVariant&    value
) -> QVariant {

    if ( change == ItemPositionChange ) {
        adjust();
        graph()->itemMoved();

        if ( info ) {
            info->setPos( QPointF(
                value.toPointF().x() + ( boundingRect().width() - info->boundingRect().width() ) / 2,
                ( value.toPointF().y() + boundingRect().height() ) - 10
            ) );
        }
    }

    return QGraphicsItem::itemChange( change, value );
}

auto HcSessionGraphItem::shape() -> QPainterPath
{
    auto path = QPainterPath();

    path.addEllipse( rect );

    return path;
}

auto HcSessionGraphItem::calculateForces(
    void
) -> void {
    auto xvel = qreal( 0 );
    auto yvel = qreal( 0 );

    if ( ! scene() || scene()->mouseGrabberItem() == this ) {
        _pos = pos();
        return;
    }

    if ( qAbs( xvel ) < 0.1 && qAbs( yvel ) < 0.1 ) {
        xvel = yvel = 0;
    }

    auto sceneRect = scene()->sceneRect();

    _pos = pos() + QPointF( xvel, yvel );
    _pos.setX( qMin( qMax( _pos.x(), sceneRect.left() + 10 ), sceneRect.right()  - 10 ) );
    _pos.setY( qMin( qMax( _pos.y(), sceneRect.top()  + 10 ), sceneRect.bottom() - 10 ) );
}

auto HcSessionGraphItem::advancePosition(
    void
) -> bool {
    if ( _pos == pos() ) {
        return false;
    }

    setPos( _pos );

    return true;
}

auto HcSessionGraphItem::mousePressEvent(
    QGraphicsSceneMouseEvent* event
) -> void {
    update();
    graph()->itemMoved();
    QGraphicsItem::mousePressEvent( event );
}

auto HcSessionGraphItem::mouseReleaseEvent(
    QGraphicsSceneMouseEvent* event
) -> void {
    update();
    graph()->itemMoved();
    QGraphicsItem::mouseReleaseEvent( event );
}

auto HcSessionGraphItem::mouseMoveEvent(
    QGraphicsSceneMouseEvent* event
) -> void {
    update();
    graph()->itemMoved();
    QGraphicsItem::mouseMoveEvent( event );
}

//
// HcSessionGraphEdge
//

HcSessionGraphEdge::HcSessionGraphEdge(
    HcSessionGraphItem* source,
    HcSessionGraphItem* destination,
    QColor              color
) : _source( source ), _destination( destination ), color( color ) {
    setAcceptedMouseButtons( Qt::NoButton );

    source->addEdge( this );
    destination->addEdge( this );

    pulsate.timer = new QTimer();
    pulsate.color = color;
    _signals      = new HcGraphItemSignal( this );

    QObject::connect( pulsate.timer, &QTimer::timeout, _signals, &HcGraphItemSignal::updatePulsation );

    adjust();
}

HcSessionGraphEdge::~HcSessionGraphEdge() {
    delete _signals;
}

auto HcSessionGraphEdge::source()      const -> HcSessionGraphItem* { return _source;      }
auto HcSessionGraphEdge::destination() const -> HcSessionGraphItem* { return _destination; }

auto HcSessionGraphEdge::adjust() -> void
{
    if ( ! source() || ! destination() ) {
        return;
    }

    auto line   = QLineF( mapFromItem( source(), 0, 0 ), mapFromItem( destination(), 0, -30 ) );
    auto length = line.length();

    prepareGeometryChange();

    if ( length > qreal( 20. ) ) {
        auto edgeSpace  = 50;
        auto edgeOffset = QPointF( ( ( line.dx() * edgeSpace ) / length ), ( ( line.dy() * edgeSpace ) / length ) - 10 );

        sourcePoint = line.p1() + edgeOffset;
        destPoint   = line.p2() - edgeOffset;
    } else {
        sourcePoint = destPoint = line.p1();
    }
}

auto HcSessionGraphEdge::setColor(
    const QColor& color
) -> void {
    this->color = color;
}

auto HcSessionGraphEdge::setStatus(
    const QString& status,
    const QColor&  color
) -> void {
    this->status       = status;
    this->status_color = color;
}

auto HcSessionGraphEdge::startPulsation() -> void
{
    if ( !_source->graph()->settings()->pulsation() ) {
        //
        // if pulsation has been disabled then we won't be displaying it
        return;
    }

    pulsate.active = true;
    pulsate.step   = 0;

    pulsate.timer->start( 50 );

    QTimer::singleShot( 1000, [ this ] ( ) {
        pulsate.active = false;
        pulsate.timer->stop();
        update();
    } );
}

QRectF HcSessionGraphEdge::boundingRect() const {
    const auto width = qreal( 1 );
    const auto extra = qreal( width + arrowSize ) / 2.0;

    if ( ! source() || ! destination() ) {
        return QRectF();
    }

    return QRectF( sourcePoint, QSizeF(
        destPoint.x() - sourcePoint.x(),
        destPoint.y() - sourcePoint.y())
    ).normalized().adjusted( -extra, -extra, extra, extra );
}

void HcSessionGraphEdge::paint(
    QPainter*                       painter,
    const QStyleOptionGraphicsItem* option,
    QWidget*                        widget
) {
    auto line = QLineF( sourcePoint, destPoint );

    if ( ! source() || ! destination() ) {
        return;
    }

    if ( qFuzzyCompare( line.length(), qreal( 0. ) ) ) {
        return;
    }

    //
    // TODO: make the connections dashed if the session is beaconing
    //       back, while make it straight lines if the connections are
    //       interactive and activate pulsating
    //

    //
    // calculate the pulsating effect (e.g., changing color intensity)
    //
    auto current_color = pulsate.active ? pulsate.color : color;
    if ( pulsate.active ) {
        current_color.setAlpha(
            100 + ( 105 * std::sin( M_PI * pulsate.step / 10 ) )
        );
    }

    auto angle         = std::atan2( -line.dy(), line.dx() );
    auto sourceArrowP1 = sourcePoint + QPointF( sin( angle + M_PI / 3 ) * arrowSize, cos( angle + M_PI / 3 ) * arrowSize );
    auto sourceArrowP2 = sourcePoint + QPointF( sin( angle + M_PI - M_PI / 3 ) * arrowSize, cos( angle + M_PI - M_PI / 3 ) * arrowSize );
    auto destArrowP1   = destPoint   + QPointF( sin( angle - M_PI / 3 ) * arrowSize, cos( angle - M_PI / 3 ) * arrowSize );
    auto destArrowP2   = destPoint   + QPointF( sin( angle - M_PI + M_PI / 3 ) * arrowSize, cos( angle - M_PI + M_PI / 3 ) * arrowSize );

    if ( source()->agent() == nullptr ) {
        //
        // direct connection
        //
        painter->setPen( QPen( current_color, 2, Qt::DashLine, Qt::RoundCap, Qt::RoundJoin ) );
        painter->drawLine( line );
        painter->setBrush( current_color );
        painter->drawPolygon( QPolygonF() << line.p1() << sourceArrowP1 << sourceArrowP2 );
    } else {
        //
        // pivot connection
        //

        // adjust the text position so it is centered on the rotated line
        const auto metrics = QFontMetrics( painter->font() );
        const auto textPos = QPointF( -metrics.horizontalAdvance( status ) / 2, metrics.height() / 2 );

        painter->setPen( QPen( current_color, 2, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin ) );
        painter->setBrush( current_color  );
        painter->drawPolygon( QPolygonF() << line.p2() << destArrowP1 << destArrowP2 );

        if ( status.isEmpty() ) {
            //
            // there is no status to display
            //
            painter->drawLine( line );
            return;
        }

        // increase the font size of the status text
        auto font = painter->font();
        font.setPointSizeF( font.pointSizeF() * 1.1 );
        painter->setFont( font );

        // calculate two partial lines, leaving a gap for the text
        auto gapLength      = metrics.horizontalAdvance( status ) + 10;
        auto lineBeforeText = QLineF( line.p1(), line.pointAt( ( line.length() - gapLength ) / ( 2 * line.length() ) ) );
        auto lineAfterText  = QLineF( line.pointAt( 1 - ( line.length() - gapLength ) / ( 2 * line.length() ) ), line.p2() );

        painter->drawLine( lineBeforeText );
        painter->drawLine( lineAfterText  );

        // save the painter's state before rotating
        painter->save();

        // translate to the midpoint of the line
        painter->translate( line.pointAt( 0.5 ) );

        // rotate the painter by the angle of the line (in degrees)
        auto angleDegrees = ( -angle * 180 / M_PI );

        // if the angle makes the text upside down, rotate it by 180 degrees
        if ( angleDegrees > 90 || angleDegrees < -90 ) {
            angleDegrees += 180;
        }

        painter->rotate( angleDegrees );

        // draw the text
        painter->setPen( QPen( status_color ) ); // set the pen color for the text
        painter->drawText( textPos, status.toUpper() );    // draw the rotated text

        // restore the painter's state
        painter->restore();
    }
}

//
// HcGraphItemSignal
//

HcGraphItemSignal::HcGraphItemSignal(
    HcSessionGraphEdge* edge
) : edge( edge ) {}

HcGraphItemSignal::~HcGraphItemSignal() = default;

auto HcGraphItemSignal::updatePulsation() const -> void {
    edge->pulsate.step = ( edge->pulsate.step + 1 ) % 10;
    edge->update();
}

//
// HcSessionGraphSetting
//

HcSessionGraphSetting::HcSessionGraphSetting(
    QWidget* parent
) : QToolButton( parent ) {
    menu.setStyleSheet( HcApplication::StyleSheet() );

    _pulsation = menu.addAction( "show pulsation" );
    _pulsation->setCheckable( true );
    _pulsation->setChecked( true );

    setAttribute( Qt::WA_TranslucentBackground );
    setStyleSheet( "background: transparent;" );

    setMenu( &menu );
    setToolButtonStyle( Qt::ToolButtonIconOnly );
    setIcon( QIcon( ":/icons/32px-align" ) );
    setProperty( "HcButton", "true" );
    setPopupMode( InstantPopup );
    setLayoutDirection( Qt::LeftToRight );
    setMaximumWidth( 90 );
}

HcSessionGraphSetting::~HcSessionGraphSetting() = default;

auto HcSessionGraphSetting::pulsation(
    void
) -> bool {
    return _pulsation->isChecked();
}

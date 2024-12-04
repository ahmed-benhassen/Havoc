#ifndef HCSESSIONGRAPH_H
#define HCSESSIONGRAPH_H

#include <core/HcAgent.h>

#include <QGraphicsItem>
#include <QGraphicsView>
#include <QGraphicsProxyWidget>
#include <QGraphicsSceneContextMenuEvent>
#include <QPainterPath>
#include <QKeyEvent>
#include <QScrollBar>
#include <QRect>

class HcSessionGraphSetting;
class HcSessionGraphItem;
class HcSessionGraphItemInfo;
class HcSessionGraphEdge;
class HcSessionGraph;
class HcGraphItemSignal;

//
// HcSessionGraphItem
//

class HcSessionGraphItemInfo final : public QGraphicsItem
{
    QString text_top    = {};
    QString text_bottom = {};
public:
    explicit HcSessionGraphItemInfo(
        const QString& text_top,
        const QString& text_bottom
    );
    ~HcSessionGraphItemInfo();

    auto boundingRect(
        void
    ) const -> QRectF override;

protected:
    auto paint(
        QPainter*                       painter,
        const QStyleOptionGraphicsItem* option,
        QWidget*                        widget
    ) -> void override;
};

class HcSessionGraphItem final : public QGraphicsItem
{
    HcAgent*                           _agent    = {};
    HcSessionGraphItem*                _parent   = {};
    HcSessionGraph*                    _graph    = {};
    QPointF                            _pos      = {};
    std::vector<HcSessionGraphItem*>   _children = {};
    HcSessionGraphEdge*                edge      = {};
    QRectF                             rect      = {};
    std::vector<HcSessionGraphEdge*>   edges     = {};
    HcSessionGraphItemInfo*            info      = {};

public:
    HcSessionGraphItem* Thread   = nullptr; // For extreme left or right nodes, used to provide a successor node in a contour.
    HcSessionGraphItem* Ancestor = this;    // During the tree layout, it points to the node's ancestor that is used to determine how far apart different subtrees should be.
    double              Prelim   = 0;       // Preliminary y-coordinate calculated during the first tree traversal.
    double              Modifier = 0;       // Amount to adjust a node's y-coordinate, based on the positions of its descendants.
    double              Shift    = 0;       // Amount to move subtrees apart to avoid overlaps.
    double              Change   = 0;       // Rate of change in shift amount, used to evenly distribute shifts among siblings.

    explicit HcSessionGraphItem(
        HcAgent*        agent,
        HcSessionGraph* graph
    );
    ~HcSessionGraphItem() override;

    auto graph(
        void
    ) const -> HcSessionGraph*;

    auto agent(
        void
    ) const -> HcAgent*;

    auto addPivot(
        HcSessionGraphItem* agent
    ) -> void;

    auto removePivot(
        const HcSessionGraphItem* agent
    ) -> void;

    auto pivots(
        void
    ) const -> std::vector<HcSessionGraphItem*>;

    auto addParent(
        HcSessionGraphItem* parent
    ) -> void;

    auto parent(
        void
    ) const -> HcSessionGraphItem*;

    auto addEdge(
        HcSessionGraphEdge* edge
    ) -> void;

    auto adjust() -> void;

    auto setItemEdge(
        HcSessionGraphEdge* edge
    ) -> void;

    auto itemEdge() -> HcSessionGraphEdge*;
    auto itemInfo() -> HcSessionGraphItemInfo*;

    auto calculateForces() -> void;
    auto advancePosition() -> bool;

    enum { Type = UserType + 1 };
    int type() const override { return Type; }

protected:
    auto boundingRect(
        void
    ) const -> QRectF override;

    auto paint(
        QPainter* painter,
        const QStyleOptionGraphicsItem* option,
        QWidget* widget
    ) -> void override;

    auto itemChange(
        GraphicsItemChange change,
        const QVariant&    value
    ) -> QVariant override;

    auto shape() -> QPainterPath;
    auto mousePressEvent( QGraphicsSceneMouseEvent* event ) -> void override;
    auto mouseReleaseEvent( QGraphicsSceneMouseEvent* event ) -> void override;
    auto mouseMoveEvent( QGraphicsSceneMouseEvent* event ) -> void override;
};

//
// HcSessionGraphEdge
//

class HcSessionGraphEdge final : public QGraphicsItem
{
    HcSessionGraphItem* _source      = {};
    HcSessionGraphItem* _destination = {};
    QColor              color        = {};
    QString             status       = {};
    QColor              status_color = {};
    QPointF             sourcePoint  = {};
    QPointF             destPoint    = {};
    qreal               arrowSize    = 10;
    HcGraphItemSignal*  _signals     = {};

public:
    struct {
        QColor  color;
        QTimer* timer;
        bool    active;
        int     step;
    } pulsate;

    explicit HcSessionGraphEdge(
        HcSessionGraphItem* source,
        HcSessionGraphItem* destination,
        QColor              color
    );
    ~HcSessionGraphEdge();

    auto source() const -> HcSessionGraphItem*;
    auto destination() const -> HcSessionGraphItem*;

    auto adjust() -> void;
    auto setColor(
        const QColor& color
    ) -> void;

    auto setStatus(
        const QString& status,
        const QColor&  color
    ) -> void;

    enum { Type = UserType + 2 };
    int type() const override { return Type; }

    auto startPulsation() -> void;

protected:
    QRectF boundingRect() const override;
    void paint( QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget ) override;
};

//
// HcGraphItemSignal
//

class HcGraphItemSignal final : public QObject
{
    Q_OBJECT

    HcSessionGraphEdge* edge = {};

public:
    explicit HcGraphItemSignal(
        HcSessionGraphEdge* edge
    );
    ~HcGraphItemSignal();

public Q_SLOTS:
    auto updatePulsation() const -> void;
};

//
// HcSessionGraphSetting
//

class HcSessionGraphSetting final : public QToolButton
{
    Q_OBJECT

    QMenu    menu       = QMenu();
    QAction* _pulsation = {};

public:
    explicit HcSessionGraphSetting( QWidget* parent = nullptr );
    ~HcSessionGraphSetting() override;

    auto pulsation() -> bool;
};

//
// HcSessionGraphInfo
//

class HcSessionGraphInfo final : public QWidget {

};

//
// HcGraphLayoutTree
//

class HcGraphLayoutTree {

public:
    static double X_SEP;
    static double Y_SEP;

    static auto initNode(
        HcSessionGraphItem* item
    ) -> void;

    static auto draw(
        HcSessionGraphItem* item
    ) -> void;

    static auto firstWalk(
        HcSessionGraphItem* item
    ) -> void;

    static auto apportion(
        HcSessionGraphItem*  item,
        HcSessionGraphItem*& defaultAncestor
    ) -> void;

    static auto moveSubtree(
        HcSessionGraphItem* wm,
        HcSessionGraphItem* wp,
        double shift
    ) -> void;

    static auto nextLeft(
        HcSessionGraphItem* v
    ) -> HcSessionGraphItem*;

    static auto nextRight(
        HcSessionGraphItem* v
    ) -> HcSessionGraphItem*;

    static auto ancestor(
        HcSessionGraphItem* vim,
        HcSessionGraphItem* v,
        HcSessionGraphItem*& defaultAncestor
    ) -> HcSessionGraphItem*;

    static auto executeShifts(
        HcSessionGraphItem* item
    ) -> void;

    static auto secondWalk(
        HcSessionGraphItem* item,
        double m,
        double depth
    ) -> void;
};

//
// HcSessionGraph
//

class HcSessionGraphScene final : public QGraphicsScene
{
    Q_OBJECT

    int grid_size = 0;

public:
    explicit HcSessionGraphScene(
        int      grid_size = 50,
        QObject* parent    = nullptr
    );

    ~HcSessionGraphScene() override;

private:
    auto mouseMoveEvent( QGraphicsSceneMouseEvent* event ) -> void override;
    auto contextMenuEvent( QGraphicsSceneContextMenuEvent *event ) -> void override;

};

class HcSessionGraph final : public QGraphicsView
{
    Q_OBJECT

    std::vector<HcSessionGraphItem*> _nodes     = {};
    HcSessionGraphSetting*           _settings  = {};
    HcSessionGraphScene*             _scene      = {};
    HcSessionGraphItem*              server     = {};
    QGridLayout*                     box_layout = {};
    int                              timer_id   = 0;

public:
    explicit HcSessionGraph( QWidget *parent = nullptr );
    ~HcSessionGraph() override;

    auto scaleView(
        qreal scaleFactor
    ) -> void;

    auto addAgent(
        HcAgent* agent
    ) -> HcSessionGraphItem*;

    auto removeAgent(
        HcAgent* agent
    ) -> void;

    auto isServer(
        const HcSessionGraphItem* item
    ) const -> bool;

    auto itemMoved() -> void;

    auto nodes() -> std::vector<HcSessionGraphItem*>;
    auto settings() -> HcSessionGraphSetting*;
protected:
    void keyPressEvent( QKeyEvent* event ) override;
    void resizeEvent( QResizeEvent* event ) override;
    void wheelEvent( QWheelEvent* event ) override;
    void timerEvent( QTimerEvent* event ) override;

public Q_SLOTS:
    auto zoomIn() -> void;
    auto zoomOut() -> void;
};

#endif //HCSESSIONGRAPH_H

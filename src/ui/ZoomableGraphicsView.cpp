#include "ZoomableGraphicsView.h"

#include <QDragEnterEvent>
#include <QDragMoveEvent>
#include <QDropEvent>
#include <QGraphicsPixmapItem>
#include <QMimeData>
#include <QPainter>
#include <QScrollBar>
#include <QUrl>

ZoomableGraphicsView::ZoomableGraphicsView(QWidget *parent)
    : QGraphicsView(parent) {
    setAcceptDrops(true);
    setDragMode(QGraphicsView::ScrollHandDrag);
    setRenderHint(QPainter::SmoothPixmapTransform, true);
    setAlignment(Qt::AlignCenter);
    setRenderHint(QPainter::Antialiasing, true);
    setRenderHint(QPainter::TextAntialiasing, true);
    setViewportUpdateMode(QGraphicsView::FullViewportUpdate);
    setCacheMode(QGraphicsView::CacheBackground);
    setOptimizationFlag(QGraphicsView::DontAdjustForAntialiasing, true);
}

void ZoomableGraphicsView::drawBackground(QPainter *painter, const QRectF &rect) {
    QGraphicsView::drawBackground(painter, rect);

    if (!scene())
        return;

    QGraphicsPixmapItem *pixmapItem = nullptr;
    for (QGraphicsItem *item : scene()->items()) {
        if (auto *pItem = qgraphicsitem_cast<QGraphicsPixmapItem *>(item)) {
            if (pItem->pixmap().hasAlphaChannel()) {
                pixmapItem = pItem;
                break;
            }
        }
    }

    if (!pixmapItem)
        return;

    qreal scale = transform().m11();
    if (scale <= 0)
        scale = 1.0;
    int gridSize = qBound(4, int(20 / scale), 64);

    QRectF imageRect = pixmapItem->sceneBoundingRect();
    QRectF intersectRect = rect.intersected(imageRect);

    if (intersectRect.isEmpty())
        return;

    painter->setRenderHint(QPainter::Antialiasing, false);

    QColor light(240, 240, 240);
    QColor dark(200, 200, 200);

    int xStart = qFloor(intersectRect.x() / gridSize) * gridSize;
    int yStart = qFloor(intersectRect.y() / gridSize) * gridSize;

    for (int y = yStart; y < qCeil(intersectRect.bottom()); y += gridSize) {
        for (int x = xStart; x < qCeil(intersectRect.right()); x += gridSize) {
            QRectF cell(x, y, gridSize, gridSize);
            QRectF drawRect = cell.intersected(intersectRect);
            if (drawRect.isEmpty())
                continue;

            bool isLight = ((x / gridSize) + (y / gridSize)) % 2 == 0;
            painter->fillRect(drawRect, isLight ? light : dark);
        }
    }
}

void ZoomableGraphicsView::wheelEvent(QWheelEvent *event) {
    if (event->modifiers() & Qt::ControlModifier) {
        double zoomInFactor = 1.15;
        double zoomOutFactor = 1.0 / zoomInFactor;
        double scaleFactor = (event->angleDelta().y() > 0) ? zoomInFactor : zoomOutFactor;

        QPointF mousePos = event->position();
        QPointF scenePos = mapToScene(mousePos.toPoint());

        scale(scaleFactor, scaleFactor);

        QPointF newScenePos = mapToScene(mousePos.toPoint());
        QPointF delta = newScenePos - scenePos;
        setTransformationAnchor(QGraphicsView::NoAnchor);
        translate(delta.x(), delta.y());

        event->accept();
    } else {
        int delta = static_cast<int>(event->angleDelta().y() * 0.5);
        verticalScrollBar()->setValue(verticalScrollBar()->value() - delta);
        event->accept();
    }
}

void ZoomableGraphicsView::dragEnterEvent(QDragEnterEvent *event) {
    if (event->mimeData()->hasUrls()) {
        event->acceptProposedAction();
    } else {
        event->ignore();
    }
}

void ZoomableGraphicsView::dragMoveEvent(QDragMoveEvent *event) {
    if (event->mimeData()->hasUrls()) {
        event->acceptProposedAction();
    } else {
        event->ignore();
    }
}

void ZoomableGraphicsView::dropEvent(QDropEvent *event) {
    if (event->mimeData()->hasUrls()) {
        QStringList paths;
        const QList<QUrl> urls = event->mimeData()->urls();
        for (const QUrl &url : urls) {
            paths.append(url.toLocalFile());
        }
        if (!paths.isEmpty()) {
            emit filesDropped(paths);
            event->acceptProposedAction();
            return;
        }
    }
    event->ignore();
}

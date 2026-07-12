#pragma once

#include <QGraphicsView>

class ZoomableGraphicsView : public QGraphicsView {
    Q_OBJECT

public:
    explicit ZoomableGraphicsView(QWidget *parent = nullptr);

signals:
    void filesDropped(const QStringList &paths);

protected:
    void drawBackground(QPainter *painter, const QRectF &rect) override;
    void wheelEvent(QWheelEvent *event) override;
    void dragEnterEvent(QDragEnterEvent *event) override;
    void dragMoveEvent(QDragMoveEvent *event) override;
    void dropEvent(QDropEvent *event) override;
};

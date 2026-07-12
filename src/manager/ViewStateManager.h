#pragma once

#include "common/ImageModel.h"
#include <QObject>

class ViewStateManager : public QObject {
    Q_OBJECT

public:
    explicit ViewStateManager(QObject *parent = nullptr);

    ViewState viewState() const;
    void reset();

    void zoomIn();
    void zoomOut();
    void actualSize();
    void fitToWindow();
    void toggleFitActualSize();
    void rotate(int angle);
    void mirror();

    void setFitToWindow(bool fit);
    void setScaleFactor(double scale);

signals:
    void viewStateChanged(const ViewState &state);

private:
    ViewState m_state;
};

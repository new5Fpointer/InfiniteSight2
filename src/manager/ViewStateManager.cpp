#include "ViewStateManager.h"

ViewStateManager::ViewStateManager(QObject *parent)
    : QObject(parent) {
}

ViewState ViewStateManager::viewState() const {
    return m_state;
}

void ViewStateManager::reset() {
    m_state.scaleFactor = 1.0;
    m_state.isFitToWindow = true;
    m_state.rotation = 0;
    m_state.mirrored = false;
    emit viewStateChanged(m_state);
}

void ViewStateManager::zoomIn() {
    m_state.scaleFactor *= 1.2;
    m_state.isFitToWindow = false;
    emit viewStateChanged(m_state);
}

void ViewStateManager::zoomOut() {
    m_state.scaleFactor *= 0.8;
    m_state.isFitToWindow = false;
    emit viewStateChanged(m_state);
}

void ViewStateManager::actualSize() {
    m_state.scaleFactor = 1.0;
    m_state.isFitToWindow = false;
    emit viewStateChanged(m_state);
}

void ViewStateManager::fitToWindow() {
    m_state.scaleFactor = 1.0;
    m_state.isFitToWindow = true;
    emit viewStateChanged(m_state);
}

void ViewStateManager::toggleFitActualSize() {
    if (m_state.isFitToWindow) {
        actualSize();
    } else {
        fitToWindow();
    }
}

void ViewStateManager::rotate(int angle) {
    m_state.rotation = (m_state.rotation + angle) % 360;
    if (m_state.rotation < 0)
        m_state.rotation += 360;
    emit viewStateChanged(m_state);
}

void ViewStateManager::mirror() {
    m_state.mirrored = !m_state.mirrored;
    emit viewStateChanged(m_state);
}

void ViewStateManager::setFitToWindow(bool fit) {
    m_state.isFitToWindow = fit;
    emit viewStateChanged(m_state);
}

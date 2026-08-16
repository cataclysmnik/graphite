#pragma once

#include <QWidget>
#include <QLabel>
#include <QSpinBox>
#include <QTimer>
#include <QFrame>

namespace dsp {
    class AudioEngine;
}

namespace gui {

class TunerDial : public QWidget
{
    Q_OBJECT
public:
    explicit TunerDial(QWidget* parent = nullptr);
    void setCents(double cents, bool hasSignal);

protected:
    void paintEvent(QPaintEvent* event) override;

private slots:
    void updatePhysics();

private:
    double m_cents { 0.0 };
    double m_smoothedCents { 0.0 };
    bool m_hasSignal { false };
    QTimer m_physicsTimer;
};

class GuitarTunerWidget : public QWidget
{
    Q_OBJECT
public:
    explicit GuitarTunerWidget(dsp::AudioEngine* engine, QWidget* parent = nullptr);

private slots:
    void processTuner();
    void onA4Changed(int val);

private:
    void setupUi();
    void showInactive();

    dsp::AudioEngine* m_engine;
    double m_refA4 { 440.0 };
    
    QTimer m_timer;
    
    QSpinBox* m_spinA4;
    QFrame* m_noteCard;
    QLabel* m_lblNote;
    QLabel* m_lblCents;
    QLabel* m_lblFrequency;
    TunerDial* m_dial;
};

} // namespace gui

#pragma once

#include <QWidget>

namespace dsp {
    class AudioEngine;
}

namespace gui {

class SignalFlow : public QWidget
{
    Q_OBJECT
public:
    explicit SignalFlow(dsp::AudioEngine* engine, QWidget* parent = nullptr);

protected:
    void paintEvent(QPaintEvent* event) override;

private:
    dsp::AudioEngine* m_engine;
};

} // namespace gui

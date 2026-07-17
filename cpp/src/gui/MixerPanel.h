#pragma once

#include <QScrollArea>

namespace dsp {
    class AudioEngine;
}

namespace gui {

class MixerPanel : public QScrollArea
{
    Q_OBJECT
public:
    explicit MixerPanel(dsp::AudioEngine* engine, QWidget* parent = nullptr);
    const std::vector<class MixerStrip*>& getMixerStrips() const { return m_mixerStrips; }
    void reorderStrips(int fromIndex, int toIndex);

private:
    dsp::AudioEngine* m_engine;
    std::vector<class MixerStrip*> m_mixerStrips;
};

} // namespace gui

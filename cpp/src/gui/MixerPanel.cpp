#include "MixerPanel.h"
#include "MixerStrip.h"
#include "../audio/AudioEngine.h"
#include <QHBoxLayout>
#include <QWidget>

namespace gui {

MixerPanel::MixerPanel(dsp::AudioEngine* engine, QWidget* parent)
    : QScrollArea(parent), m_engine(engine)
{
    setWidgetResizable(true);
    setFrameShape(QFrame::NoFrame);
    
    QWidget* container = new QWidget();
    QHBoxLayout* layout = new QHBoxLayout(container);
    layout->setContentsMargins(5, 5, 5, 5);
    layout->setSpacing(5);
    
    // Add Master track at the end (or beginning, depending on preference)
    MixerStrip* masterStrip = new MixerStrip(-1, "Master", m_engine, container);
    layout->addWidget(masterStrip);
    m_mixerStrips.push_back(masterStrip);
    
    // Add default tracks
    const char* trackNames[] = {"Lead Guitar", "Rhythm Guitar", "Bass", "Drums"};
    for (int i = 0; i < 4; ++i) {
        MixerStrip* strip = new MixerStrip(i, trackNames[i], m_engine, container);
        layout->addWidget(strip);
        m_mixerStrips.push_back(strip);
    }
    
    layout->addStretch(); // Push to left
    
    setWidget(container);
}

} // namespace gui

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

void MixerPanel::reorderStrips(int fromIndex, int toIndex)
{
    int stripFrom = fromIndex + 1; // 0 is Master
    int stripTo = toIndex + 1;
    
    if (stripFrom >= m_mixerStrips.size() || stripTo >= m_mixerStrips.size()) return;
    
    auto strip = m_mixerStrips[stripFrom];
    m_mixerStrips.erase(m_mixerStrips.begin() + stripFrom);
    m_mixerStrips.insert(m_mixerStrips.begin() + stripTo, strip);
    
    QHBoxLayout* layout = qobject_cast<QHBoxLayout*>(widget()->layout());
    if (layout) {
        layout->removeWidget(strip);
        layout->insertWidget(stripTo, strip);
    }
    
    // Update strip indices
    for (int i = 1; i < m_mixerStrips.size(); ++i) {
        m_mixerStrips[i]->setTrackIndex(i - 1);
    }
}

} // namespace gui

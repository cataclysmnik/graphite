#pragma once

#include <QWidget>
#include <QScrollArea>
#include <QTimer>

namespace dsp {
    class AudioEngine;
}

namespace gui {

class TimeRulerWidget : public QWidget
{
    Q_OBJECT
public:
    explicit TimeRulerWidget(dsp::AudioEngine* engine, QWidget* parent = nullptr);
    
    void setScrollOffset(int offset);
    void setZoom(double pixelsPerSecond);

protected:
    void paintEvent(QPaintEvent* event) override;
    
private:
    dsp::AudioEngine* m_engine;
    int m_scrollOffset { 0 };
    double m_pixelsPerSecond { 50.0 };
};


class TimelineLanesWidget : public QWidget
{
    Q_OBJECT
public:
    explicit TimelineLanesWidget(dsp::AudioEngine* engine, QWidget* parent = nullptr);

    void setZoom(double pixelsPerSecond);
    
protected:
    void paintEvent(QPaintEvent* event) override;

private slots:
    void onPlayheadTimerTick();

private:
    dsp::AudioEngine* m_engine;
    double m_pixelsPerSecond { 50.0 };
    QTimer m_playheadTimer;
    
    // Cache the playhead position to only update when it moves
    double m_lastPlayheadTime { 0.0 };
};


class TimelineContainer : public QScrollArea
{
    Q_OBJECT
public:
    explicit TimelineContainer(dsp::AudioEngine* engine, QWidget* parent = nullptr);

private slots:
    void onHorizontalScroll(int value);

private:
    TimeRulerWidget* m_ruler;
    TimelineLanesWidget* m_lanes;
};

} // namespace gui

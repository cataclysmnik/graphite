#pragma once

#include <QWidget>
#include <QTimer>

namespace gui {

class LevelMeter : public QWidget {
    Q_OBJECT
public:
    explicit LevelMeter(Qt::Orientation orientation = Qt::Vertical, QWidget* parent = nullptr);
    ~LevelMeter() override = default;

    void setLevel(float peak);

protected:
    void paintEvent(QPaintEvent* event) override;

private slots:
    void decay();

private:
    Qt::Orientation m_orientation;
    float m_currentPeak = 0.0f;
    float m_peakHold = 0.0f;
    bool m_clipActive = false;
    int m_clipFrames = 0;
    
    QTimer* m_decayTimer;
};

} // namespace gui

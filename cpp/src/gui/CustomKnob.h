#pragma once

#include <QDial>
#include <QPaintEvent>

namespace gui {

class CustomKnob : public QDial {
    Q_OBJECT
public:
    explicit CustomKnob(QWidget* parent = nullptr);
    ~CustomKnob() override = default;

protected:
    void paintEvent(QPaintEvent* event) override;
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
    void enterEvent(QEnterEvent* event) override;
#else
    void enterEvent(QEvent* event) override;
#endif
    void leaveEvent(QEvent* event) override;

private:
    bool m_isHovered = false;
};

} // namespace gui

#pragma once

#include <QDialog>
#include <QString>
#include <QMessageBox>
#include <QPoint>
#include <QMouseEvent>
#include <QWidget>

namespace gui {

class CustomMessageBox : public QDialog {
    Q_OBJECT
public:
    enum Icon {
        NoIcon,
        Information,
        Warning,
        Critical,
        Question
    };

    static QMessageBox::StandardButton question(QWidget* parent, const QString& title, const QString& text, QMessageBox::StandardButtons buttons = QMessageBox::Yes | QMessageBox::No, QMessageBox::StandardButton defaultButton = QMessageBox::NoButton);
    
    static QMessageBox::StandardButton critical(QWidget* parent, const QString& title, const QString& text, QMessageBox::StandardButtons buttons = QMessageBox::Ok, QMessageBox::StandardButton defaultButton = QMessageBox::NoButton);
    
    static QMessageBox::StandardButton information(QWidget* parent, const QString& title, const QString& text, QMessageBox::StandardButtons buttons = QMessageBox::Ok, QMessageBox::StandardButton defaultButton = QMessageBox::NoButton);

    explicit CustomMessageBox(QWidget* parent, const QString& title, const QString& text, Icon icon, QMessageBox::StandardButtons buttons, QMessageBox::StandardButton defaultButton);

protected:
    void mousePressEvent(QMouseEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;

private:
    void addButtons(QMessageBox::StandardButtons buttons, QMessageBox::StandardButton defaultButton);
    
    QPoint m_dragPosition;
    QMessageBox::StandardButton m_clickedButton = QMessageBox::NoButton;
};

} // namespace gui

#include "CustomMessageBox.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>

namespace gui {

CustomMessageBox::CustomMessageBox(QWidget* parent, const QString& title, const QString& text, Icon icon, QMessageBox::StandardButtons buttons, QMessageBox::StandardButton defaultButton)
    : QDialog(parent)
{
    setWindowTitle(title);
    setWindowFlags(Qt::Dialog | Qt::FramelessWindowHint);
    
    // Style and sizing
    setMinimumWidth(350);
    
    QVBoxLayout* outerLayout = new QVBoxLayout(this);
    outerLayout->setContentsMargins(0, 0, 0, 0);
    outerLayout->setSpacing(0);
    
    // Title Strip
    QWidget* titleStrip = new QWidget(this);
    titleStrip->setObjectName("DialogTitleStrip");
    titleStrip->setFixedHeight(32);
    titleStrip->setStyleSheet("QWidget#DialogTitleStrip { background-color: #1a1a1a; } QLabel { color: #aaaaaa; font-weight: bold; }");
    
    QHBoxLayout* titleLayout = new QHBoxLayout(titleStrip);
    titleLayout->setContentsMargins(12, 0, 0, 0);
    titleLayout->setSpacing(0);
    
    QLabel* titleLabel = new QLabel(title.toUpper(), titleStrip);
    titleLayout->addWidget(titleLabel);
    titleLayout->addStretch();
    
    outerLayout->addWidget(titleStrip);
    
    // Content area
    QWidget* content = new QWidget(this);
    content->setStyleSheet("QWidget { background-color: #2b2b2b; color: #e0e0e0; font-size: 13px; }");
    outerLayout->addWidget(content, 1);
    
    QVBoxLayout* mainLayout = new QVBoxLayout(content);
    mainLayout->setContentsMargins(20, 20, 20, 20);
    mainLayout->setSpacing(20);
    
    QLabel* textLabel = new QLabel(text, this);
    textLabel->setWordWrap(true);
    mainLayout->addWidget(textLabel);
    
    // Buttons layout
    QHBoxLayout* btnLayout = new QHBoxLayout();
    btnLayout->addStretch();
    mainLayout->addLayout(btnLayout);
    
    // Helper to add a button
    auto addButton = [&](QMessageBox::StandardButton btnType, const QString& btnText) {
        QPushButton* btn = new QPushButton(btnText, this);
        btn->setFixedSize(80, 28);
        btn->setCursor(Qt::PointingHandCursor);
        btn->setFocusPolicy(Qt::NoFocus);
        
        QString defaultStyle = "QPushButton { background-color: #383838; border: 1px solid #555; color: #eee; border-radius: 4px; } QPushButton:hover { background-color: #4a4a4a; }";
        QString accentStyle = "QPushButton { background-color: #2b5c8f; border: 1px solid #3c78b5; color: #fff; border-radius: 4px; } QPushButton:hover { background-color: #356fa8; }";
        
        if (btnType == defaultButton) {
            btn->setStyleSheet(accentStyle);
        } else {
            btn->setStyleSheet(defaultStyle);
        }
        
        connect(btn, &QPushButton::clicked, this, [this, btnType]() {
            m_clickedButton = btnType;
            accept();
        });
        btnLayout->addWidget(btn);
    };
    
    if (buttons & QMessageBox::Yes) addButton(QMessageBox::Yes, "Yes");
    if (buttons & QMessageBox::No) addButton(QMessageBox::No, "No");
    if (buttons & QMessageBox::Ok) addButton(QMessageBox::Ok, "OK");
    if (buttons & QMessageBox::Cancel) addButton(QMessageBox::Cancel, "Cancel");
}

QMessageBox::StandardButton CustomMessageBox::question(QWidget* parent, const QString& title, const QString& text, QMessageBox::StandardButtons buttons, QMessageBox::StandardButton defaultButton)
{
    CustomMessageBox box(parent, title, text, Question, buttons, defaultButton);
    box.exec();
    return box.m_clickedButton;
}

QMessageBox::StandardButton CustomMessageBox::critical(QWidget* parent, const QString& title, const QString& text, QMessageBox::StandardButtons buttons, QMessageBox::StandardButton defaultButton)
{
    CustomMessageBox box(parent, title, text, Critical, buttons, defaultButton);
    box.exec();
    return box.m_clickedButton;
}

QMessageBox::StandardButton CustomMessageBox::information(QWidget* parent, const QString& title, const QString& text, QMessageBox::StandardButtons buttons, QMessageBox::StandardButton defaultButton)
{
    CustomMessageBox box(parent, title, text, Information, buttons, defaultButton);
    box.exec();
    return box.m_clickedButton;
}

void CustomMessageBox::mousePressEvent(QMouseEvent* event)
{
    if (event->button() == Qt::LeftButton) {
        m_dragPosition = event->globalPosition().toPoint() - frameGeometry().topLeft();
        event->accept();
    }
}

void CustomMessageBox::mouseMoveEvent(QMouseEvent* event)
{
    if (event->buttons() & Qt::LeftButton) {
        move(event->globalPosition().toPoint() - m_dragPosition);
        event->accept();
    }
}

} // namespace gui

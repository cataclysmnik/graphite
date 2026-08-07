#include "LoadingPopup.h"
#include <QVBoxLayout>
#include <QHBoxLayout>

namespace gui {

LoadingPopup::LoadingPopup(const QString& message, QWidget* parent)
    : QDialog(parent), m_wasCancelled(false)
{
    setWindowTitle("Please Wait");
    setFixedSize(320, 140);
    setModal(false); // Non-modal: we show() it and pump events manually
    setObjectName("LoadingPopup");
    
    setWindowFlags(Qt::Dialog | Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint);
    
    QVBoxLayout* layout = new QVBoxLayout(this);
    layout->setContentsMargins(20, 20, 20, 15);
    layout->setSpacing(12);
    
    m_lblMessage = new QLabel(message.toUpper(), this);
    m_lblMessage->setAlignment(Qt::AlignCenter);
    m_lblMessage->setWordWrap(true);
    m_lblMessage->setObjectName("LoadingMessage");
    layout->addWidget(m_lblMessage);
    
    // Indeterminate progress bar - since there's no accurate per-plugin progress
    // available without JUCE threading, just show a busy indicator
    m_progressBar = new QProgressBar(this);
    m_progressBar->setRange(0, 0); // Marquee / indeterminate
    m_progressBar->setTextVisible(false);
    m_progressBar->setFixedHeight(4);
    m_progressBar->setObjectName("LoadingProgress");
    layout->addWidget(m_progressBar);
    
    setStyleSheet(R"(
        QDialog#LoadingPopup {
            background-color: #0b0b0c;
            border: 1px solid #222225;
        }
        QLabel#LoadingMessage {
            color: #ffffff;
            font-family: "Consolas", monospace;
            font-size: 10px;
            font-weight: bold;
            letter-spacing: 0.5px;
            line-height: 14px;
        }
        QProgressBar#LoadingProgress {
            background-color: #151518;
            border: none;
        }
        QProgressBar#LoadingProgress::chunk {
            background-color: #ff0033;
        }
    )");
}

void LoadingPopup::setProgress(float /*progress*/)
{
    // No-op: progress bar is indeterminate since VST loading duration is unpredictable
}

void LoadingPopup::onWorkFinished()
{
    accept();
}

void LoadingPopup::onCancelClicked()
{
    m_wasCancelled = true;
    reject();
}

} // namespace gui

#pragma once

#include <QDialog>
#include <QLabel>
#include <QProgressBar>

namespace gui {

class LoadingPopup : public QDialog
{
    Q_OBJECT
public:
    explicit LoadingPopup(const QString& message = "LOADING...", QWidget* parent = nullptr);

    bool wasCancelled() const { return m_wasCancelled; }

public slots:
    void setProgress(float progress); // No-op - indeterminate bar
    void onWorkFinished();

private slots:
    void onCancelClicked();

private:
    bool m_wasCancelled;
    QLabel* m_lblMessage;
    QProgressBar* m_progressBar;
};

} // namespace gui

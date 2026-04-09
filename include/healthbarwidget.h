#pragma once
#include <QWidget>
#include <QColor>

class QPropertyAnimation;

class HealthBarWidget : public QWidget {
    Q_OBJECT
    Q_PROPERTY(float barPercent READ barPercent WRITE setBarPercent)
public:
    explicit HealthBarWidget(QWidget* parent = nullptr);

    // Call this once after construction to lock the bar to one color
    // (used for the SP bar — always blue regardless of value)
    void setFixedBarColor(const QColor& color);

    float barPercent() const { return m_percent; }
    void  setBarPercent(float p);
    void  animateTo(float targetPercent);

protected:
    void paintEvent(QPaintEvent*) override;
    QSize sizeHint() const override { return {200, 18}; }

private:
    float   m_percent    = 1.0f;
    bool    m_fixedColor = false;
    QColor  m_color      = QColor("#44BB44");
    QPropertyAnimation* m_anim;
};

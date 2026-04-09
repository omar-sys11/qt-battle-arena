#include "healthbarwidget.h"
#include <QPainter>
#include <QPropertyAnimation>
#include <algorithm>

HealthBarWidget::HealthBarWidget(QWidget* parent)
    : QWidget(parent)
{
    m_anim = new QPropertyAnimation(this, "barPercent", this);
    m_anim->setDuration(400);
    setMinimumSize(160, 14);
}

void HealthBarWidget::setFixedBarColor(const QColor& color)
{
    m_fixedColor = true;
    m_color      = color;
    update();
}

void HealthBarWidget::setBarPercent(float p)
{
    m_percent = std::clamp(p, 0.0f, 1.0f);
    update();
}

void HealthBarWidget::animateTo(float target)
{
    m_anim->stop();
    m_anim->setStartValue(m_percent);
    m_anim->setEndValue(std::clamp(target, 0.0f, 1.0f));
    m_anim->start();
}

void HealthBarWidget::paintEvent(QPaintEvent*)
{
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing, false);

    const int w = width();
    const int h = height();

    p.fillRect(0, 0, w, h, QColor("#F0E8D0"));          // border
    p.fillRect(2, 2, w-4, h-4, QColor("#0D0D1A"));      // background

    int fillW = static_cast<int>((w - 4) * m_percent);
    if (fillW > 0) {
        QColor barColor;
        if (m_fixedColor) {
            barColor = m_color;                          // SP bar: always blue
        } else {
            barColor = (m_percent > 0.5f) ? QColor("#44BB44")
                     : (m_percent > 0.25f)? QColor("#FFBB00")
                                          : QColor("#EE3333");
        }
        p.fillRect(2, 2, fillW, h-4, barColor);
    }
}

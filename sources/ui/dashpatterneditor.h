/* SPDX-License-Identifier: GPL-2.0-or-later */
#ifndef DASH_PATTERN_EDITOR_H
#define DASH_PATTERN_EDITOR_H

#include "../conductordashpattern.h"
#include <QDoubleSpinBox>
#include <QFormLayout>
#include <QGroupBox>
#include <QLabel>
#include <QPainter>
#include <QPixmap>
#include <array>
#include <functional>

// Shared, compact controls for the symbol editor. Field-specific notifications
// let a multi-selection retain the other lengths of each selected primitive.
class DashPatternEditor : public QGroupBox
{
public:
    enum Field { Enabled, Dash, Gap, Dot, DotGap };
    std::function<void(Field, const ConductorDashPattern &)> edited;

    explicit DashPatternEditor(QWidget *parent = nullptr) : QGroupBox(parent)
    {
        setTitle(tr("Custom dash pattern"));
        setObjectName(QStringLiteral("symbolCustomDashPattern"));
        setCheckable(true);
        auto *form = new QFormLayout(this);
        const QStringList labels {tr("Dash length:"), tr("Gap after dash:"),
                                  tr("Dot / short dash:"), tr("Gap after dot:")};
        for (int i = 0; i < 4; ++i) {
            auto *spin = new QDoubleSpinBox(this);
            spin->setObjectName(QStringLiteral("patternLength%1").arg(i));
            spin->setRange(ConductorDashPattern::minimum, ConductorDashPattern::maximum);
            spin->setDecimals(2);
            spin->setSingleStep(0.5);
            spin->setSuffix(tr(" du"));
            spin->setKeyboardTracking(false);
            m_lengths[i] = spin;
            m_labels[i] = new QLabel(labels[i], this);
            form->addRow(m_labels[i], spin);
            connect(spin, qOverload<double>(&QDoubleSpinBox::valueChanged), this,
                    [this, i](double) {
                if (m_loading) return;
                updatePreview();
                if (edited) edited(static_cast<Field>(i + 1), pattern());
            });
        }
        auto *units = new QLabel(tr("Drawing units; independent of line thickness."), this);
        units->setWordWrap(true);
        form->addRow(units);
        m_preview = new QLabel(this);
        m_preview->setAlignment(Qt::AlignCenter);
        form->addRow(m_preview);
        connect(this, &QGroupBox::toggled, this, [this](bool) {
            if (m_loading) return;
            updateEnabled();
            updatePreview();
            if (edited) edited(Enabled, pattern());
        });
        setPattern(ConductorDashPattern());
    }

    ConductorDashPattern pattern() const
    {
        ConductorDashPattern p;
        p.enabled = isChecked();
        p.dash = m_lengths[0]->value(); p.gap = m_lengths[1]->value();
        p.dot = m_lengths[2]->value(); p.dotGap = m_lengths[3]->value();
        return p;
    }

    void setPattern(const ConductorDashPattern &pattern)
    {
        m_loading = true;
        setChecked(pattern.enabled);
        const double values[] {pattern.dash, pattern.gap, pattern.dot, pattern.dotGap};
        for (int i = 0; i < 4; ++i) m_lengths[i]->setValue(values[i]);
        m_loading = false;
        updateEnabled();
        updatePreview();
    }

    void setLineStyle(Qt::PenStyle style)
    {
        m_style = style;
        setEnabled(style == Qt::DashLine || style == Qt::DashDotLine);
        updateEnabled();
        updatePreview();
    }

    static ConductorDashPattern changedField(ConductorDashPattern old,
                                             const ConductorDashPattern &value, Field field)
    {
        switch (field) {
        case Enabled: old.enabled = value.enabled; break;
        case Dash: old.dash = value.dash; break;
        case Gap: old.gap = value.gap; break;
        case Dot: old.dot = value.dot; break;
        case DotGap: old.dotGap = value.dotGap; break;
        }
        return old;
    }

private:
    void updateEnabled()
    {
        for (int i = 0; i < 4; ++i) {
            const bool enabled = isEnabled() && isChecked()
                    && (i < 2 || m_style == Qt::DashDotLine);
            m_lengths[i]->setEnabled(enabled);
            m_labels[i]->setEnabled(enabled);
        }
    }
    void updatePreview()
    {
        QPixmap pixmap(220, 28);
        pixmap.fill(palette().base().color());
        QPainter painter(&pixmap);
        QPen pen(palette().text().color(), 0.7);
        pattern().apply(pen, m_style);
        painter.setPen(pen);
        painter.drawLine(QPointF(8, 14), QPointF(212, 14));
        painter.end();
        m_preview->setPixmap(pixmap);
    }
    bool m_loading = false;
    Qt::PenStyle m_style = Qt::DashDotLine;
    std::array<QDoubleSpinBox *, 4> m_lengths;
    std::array<QLabel *, 4> m_labels;
    QLabel *m_preview;
};
#endif

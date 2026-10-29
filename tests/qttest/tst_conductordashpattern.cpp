// SPDX-License-Identifier: GPL-2.0-or-later
#include "conductorproperties.h"
#include <QtTest>
#include <QImage>
#include <QPainter>
#include <QPainterPathStroker>
#include <QTemporaryDir>
#include <limits>

class TestConductorDashPattern : public QObject
{
    Q_OBJECT
private slots:
    void legacyDefaults()
    {
        ConductorProperties cp;
        for (auto style : {Qt::SolidLine, Qt::DashLine, Qt::DashDotLine}) {
            QPen expected(Qt::black); expected.setStyle(style);
            QPen actual(Qt::black);
            QVERIFY(!cp.m_dash_pattern.apply(actual, style));
            QCOMPARE(actual, expected);
        }
        QDomDocument doc;
        auto e = doc.createElement("conductor");
        cp.toXml(e);
        QVERIFY(!e.hasAttribute("custom-dash-pattern"));
        QVERIFY(!e.hasAttribute("pattern-dash"));
        ConductorProperties reloaded;
        reloaded.fromXml(e);
        QVERIFY(reloaded.m_dash_pattern == ConductorDashPattern{});
    }

    void lengthsIndependentOfWidth()
    {
        ConductorDashPattern p {true, 7.5, 3.5, 0.8, 2.0};
        for (double width : {0.0, 0.4, 1.0, 2.0, 5.0}) {
            for (auto style : {Qt::DashLine, Qt::DashDotLine}) {
                QPen pen(Qt::black); pen.setWidthF(width);
                QVERIFY(p.apply(pen, style));
                QCOMPARE(pen.style(), Qt::CustomDashLine);
                QCOMPARE(pen.capStyle(), Qt::FlatCap);
                auto values = pen.dashPattern();
                double scale = width == 0 ? 1.0 : width;
                QCOMPARE(values.size(), style == Qt::DashLine ? 2 : 4);
                QVERIFY(qAbs(values[0] * scale - p.dash) < 1e-10);
                QVERIFY(qAbs(values[1] * scale - p.gap) < 1e-10);
                if (style == Qt::DashDotLine) {
                    QVERIFY(qAbs(values[2] * scale - p.dot) < 1e-10);
                    QVERIFY(qAbs(values[3] * scale - p.dotGap) < 1e-10);
                }
            }
        }
        QPen solid;
        QVERIFY(!p.apply(solid, Qt::SolidLine));
        QCOMPARE(solid.style(), Qt::SolidLine);
    }

    void xmlAndSettingsRoundTrip()
    {
        ConductorProperties cp;
        cp.style = Qt::DashDotLine;
        cp.m_dash_pattern = {true, 8.25, 3.75, 0.6, 2.25};
        QDomDocument doc;
        auto e = doc.createElement("conductor"); cp.toXml(e);
        ConductorProperties reloaded; reloaded.fromXml(e);
        QCOMPARE(reloaded.style, cp.style);
        QVERIFY(reloaded.m_dash_pattern == cp.m_dash_pattern);
        QTemporaryDir dir; QVERIFY(dir.isValid());
        const QString path = dir.filePath("settings.ini");
        { QSettings out(path, QSettings::IniFormat);
          cp.toSettings(out, "test/"); out.sync();
          QCOMPARE(out.status(), QSettings::NoError); }
        QSettings in(path, QSettings::IniFormat);
        reloaded = ConductorProperties{};
        reloaded.fromSettings(in, "test/");
        QVERIFY(reloaded.m_dash_pattern == cp.m_dash_pattern);
        // Disabling the option retains the user's custom values.
        cp.m_dash_pattern.enabled = false;
        cp.toXml(e); reloaded.fromXml(e);
        QVERIFY(reloaded.m_dash_pattern == cp.m_dash_pattern);
    }

    void rejectInvalidValues()
    {
        for (double invalid : {0.0, -1.0, 1001.0,
             std::numeric_limits<double>::infinity(),
             std::numeric_limits<double>::quiet_NaN()}) {
            ConductorDashPattern p; p.enabled = true; p.gap = invalid;
            QVERIFY(!p.valid());
            QPen pen; QVERIFY(!p.apply(pen, Qt::DashLine));
            QCOMPARE(pen.style(), Qt::DashLine);
        }
        for (QString invalid : {"0", "-4", "1001", "nan", "inf", "wrong"}) {
            QDomDocument doc; auto e = doc.createElement("conductor");
            e.setAttribute("custom-dash-pattern", "true");
            e.setAttribute("pattern-dot", invalid);
            ConductorProperties cp; cp.fromXml(e);
            QVERIFY(cp.m_dash_pattern == ConductorDashPattern{});
        }
    }

    void multiEditPreservesOtherFields()
    {
        ConductorProperties a, b, before, after;
        a.m_dash_pattern = {true, 8, 4, 1, 3};
        b.m_dash_pattern = {true, 12, 4, 2, 3};
        before.applyForEqualAttributes({a, b});
        QCOMPARE(before.m_dash_pattern.gap, 4.0);
        QCOMPARE(before.m_dash_pattern.dotGap, 3.0);
        after = before; after.m_dash_pattern.gap = 5.5;
        a.applyChanges(before, after); b.applyChanges(before, after);
        QCOMPARE(a.m_dash_pattern.gap, 5.5);
        QCOMPARE(b.m_dash_pattern.gap, 5.5);
        QCOMPARE(a.m_dash_pattern.dash, 8.0);
        QCOMPARE(b.m_dash_pattern.dash, 12.0);
        QCOMPARE(a.m_dash_pattern.dot, 1.0);
        QCOMPARE(b.m_dash_pattern.dot, 2.0);
        ConductorProperties single; single.applyForEqualAttributes({a});
        QVERIFY(single.m_dash_pattern == a.m_dash_pattern);
        QVERIFY(a != b);
        auto restored = a; restored.applyChanges(after, before);
        QCOMPARE(restored.m_dash_pattern.gap, 4.0);
        QCOMPARE(restored.m_dash_pattern.dash, 8.0);
    }

    void rasterGapsAndSecondaryColour()
    {
        for (auto style : {Qt::DashLine, Qt::DashDotLine}) {
            for (bool vertical : {false, true}) {
                for (double width : {1.0, 2.0}) {
                    QImage image(300, 300, QImage::Format_ARGB32_Premultiplied);
                    image.fill(Qt::white);
                    QPainter painter(&image); painter.scale(10, 10);
                    QPainterPath path; path.moveTo(2, 2);
                    path.lineTo(vertical ? QPointF(2, 25) : QPointF(25, 2));
                    ConductorDashPattern pattern {true, 6, 4, 1, 4};
                    QPen primary(Qt::green); primary.setWidthF(width);
                    QVERIFY(pattern.apply(primary, style));
                    painter.setPen(primary); painter.drawPath(path);
                    QPainterPathStroker mask;
                    mask.setWidth(primary.widthF());
                    mask.setCapStyle(primary.capStyle());
                    mask.setJoinStyle(primary.joinStyle());
                    mask.setDashPattern(primary.dashPattern());
                    painter.setClipPath(mask.createStroke(path), Qt::IntersectClip);
                    QPen secondary = primary; secondary.setColor(Qt::yellow);
                    secondary.setDashPattern({1, 2});
                    painter.setPen(secondary); painter.drawPath(path); painter.end();
                    auto colourAt = [&](double distance) {
                        int along = qRound(20 + distance * 10);
                        return vertical ? image.pixelColor(20, along) : image.pixelColor(along, 20);
                    };
                    QVERIFY(colourAt(2) != QColor(Qt::white));
                    QCOMPARE(colourAt(8), QColor(Qt::white));
                    QVERIFY(colourAt(10.4) != QColor(Qt::white));
                    if (style == Qt::DashDotLine)
                        QCOMPARE(colourAt(13), QColor(Qt::white));
                }
            }
        }
    }
};

QTEST_GUILESS_MAIN(TestConductorDashPattern)
#include "tst_conductordashpattern.moc"

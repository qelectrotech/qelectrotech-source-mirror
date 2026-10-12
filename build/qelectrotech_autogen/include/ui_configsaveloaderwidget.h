/********************************************************************************
** Form generated from reading UI file 'configsaveloaderwidget.ui'
**
** Created by: Qt User Interface Compiler version 6.11.0
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef UI_CONFIGSAVELOADERWIDGET_H
#define UI_CONFIGSAVELOADERWIDGET_H

#include <QtCore/QVariant>
#include <QtGui/QIcon>
#include <QtWidgets/QApplication>
#include <QtWidgets/QComboBox>
#include <QtWidgets/QGridLayout>
#include <QtWidgets/QGroupBox>
#include <QtWidgets/QLineEdit>
#include <QtWidgets/QPushButton>

QT_BEGIN_NAMESPACE

class Ui_ConfigSaveLoaderWidget
{
public:
    QGridLayout *gridLayout;
    QLineEdit *m_line_edit;
    QComboBox *m_combo_box;
    QPushButton *m_load_pb;
    QPushButton *m_save_pb;

    void setupUi(QGroupBox *ConfigSaveLoaderWidget)
    {
        if (ConfigSaveLoaderWidget->objectName().isEmpty())
            ConfigSaveLoaderWidget->setObjectName("ConfigSaveLoaderWidget");
        ConfigSaveLoaderWidget->resize(400, 300);
        gridLayout = new QGridLayout(ConfigSaveLoaderWidget);
        gridLayout->setObjectName("gridLayout");
        m_line_edit = new QLineEdit(ConfigSaveLoaderWidget);
        m_line_edit->setObjectName("m_line_edit");

        gridLayout->addWidget(m_line_edit, 1, 0, 1, 1);

        m_combo_box = new QComboBox(ConfigSaveLoaderWidget);
        m_combo_box->setObjectName("m_combo_box");

        gridLayout->addWidget(m_combo_box, 0, 0, 1, 1);

        m_load_pb = new QPushButton(ConfigSaveLoaderWidget);
        m_load_pb->setObjectName("m_load_pb");
        QIcon icon;
        icon.addFile(QString::fromUtf8(":/ico/16x16/folder-open.png"), QSize(), QIcon::Mode::Normal, QIcon::State::Off);
        m_load_pb->setIcon(icon);

        gridLayout->addWidget(m_load_pb, 0, 1, 1, 1);

        m_save_pb = new QPushButton(ConfigSaveLoaderWidget);
        m_save_pb->setObjectName("m_save_pb");
        QIcon icon1;
        icon1.addFile(QString::fromUtf8(":/ico/16x16/document-save.png"), QSize(), QIcon::Mode::Normal, QIcon::State::Off);
        m_save_pb->setIcon(icon1);

        gridLayout->addWidget(m_save_pb, 1, 1, 1, 1);

        QWidget::setTabOrder(m_combo_box, m_load_pb);
        QWidget::setTabOrder(m_load_pb, m_line_edit);
        QWidget::setTabOrder(m_line_edit, m_save_pb);

        retranslateUi(ConfigSaveLoaderWidget);

        QMetaObject::connectSlotsByName(ConfigSaveLoaderWidget);
    } // setupUi

    void retranslateUi(QGroupBox *ConfigSaveLoaderWidget)
    {
        ConfigSaveLoaderWidget->setWindowTitle(QCoreApplication::translate("ConfigSaveLoaderWidget", "GroupBox", nullptr));
        ConfigSaveLoaderWidget->setTitle(QCoreApplication::translate("ConfigSaveLoaderWidget", "Configuration", nullptr));
        m_load_pb->setText(QString());
        m_save_pb->setText(QString());
    } // retranslateUi

};

namespace Ui {
    class ConfigSaveLoaderWidget: public Ui_ConfigSaveLoaderWidget {};
} // namespace Ui

QT_END_NAMESPACE

#endif // UI_CONFIGSAVELOADERWIDGET_H

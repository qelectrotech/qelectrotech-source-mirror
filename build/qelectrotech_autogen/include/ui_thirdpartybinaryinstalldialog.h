/********************************************************************************
** Form generated from reading UI file 'thirdpartybinaryinstalldialog.ui'
**
** Created by: Qt User Interface Compiler version 6.11.0
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef UI_THIRDPARTYBINARYINSTALLDIALOG_H
#define UI_THIRDPARTYBINARYINSTALLDIALOG_H

#include <QtCore/QVariant>
#include <QtGui/QIcon>
#include <QtWidgets/QAbstractButton>
#include <QtWidgets/QApplication>
#include <QtWidgets/QDialog>
#include <QtWidgets/QDialogButtonBox>
#include <QtWidgets/QGridLayout>
#include <QtWidgets/QLabel>
#include <QtWidgets/QPushButton>

QT_BEGIN_NAMESPACE

class Ui_ThirdPartyBinaryInstallDialog
{
public:
    QGridLayout *gridLayout;
    QDialogButtonBox *m_button_box;
    QPushButton *m_download_pb;
    QPushButton *m_install_dir_pb;
    QLabel *m_label;

    void setupUi(QDialog *ThirdPartyBinaryInstallDialog)
    {
        if (ThirdPartyBinaryInstallDialog->objectName().isEmpty())
            ThirdPartyBinaryInstallDialog->setObjectName("ThirdPartyBinaryInstallDialog");
        ThirdPartyBinaryInstallDialog->resize(373, 64);
        ThirdPartyBinaryInstallDialog->setModal(true);
        gridLayout = new QGridLayout(ThirdPartyBinaryInstallDialog);
        gridLayout->setObjectName("gridLayout");
        m_button_box = new QDialogButtonBox(ThirdPartyBinaryInstallDialog);
        m_button_box->setObjectName("m_button_box");
        m_button_box->setOrientation(Qt::Horizontal);
        m_button_box->setStandardButtons(QDialogButtonBox::Ok);

        gridLayout->addWidget(m_button_box, 1, 2, 1, 1);

        m_download_pb = new QPushButton(ThirdPartyBinaryInstallDialog);
        m_download_pb->setObjectName("m_download_pb");
        QIcon icon;
        icon.addFile(QString::fromUtf8(":/ico/16x16/edit-download.png"), QSize(), QIcon::Mode::Normal, QIcon::State::Off);
        m_download_pb->setIcon(icon);

        gridLayout->addWidget(m_download_pb, 1, 0, 1, 1);

        m_install_dir_pb = new QPushButton(ThirdPartyBinaryInstallDialog);
        m_install_dir_pb->setObjectName("m_install_dir_pb");
        QIcon icon1;
        icon1.addFile(QString::fromUtf8(":/ico/16x16/folder-open.png"), QSize(), QIcon::Mode::Normal, QIcon::State::Off);
        m_install_dir_pb->setIcon(icon1);

        gridLayout->addWidget(m_install_dir_pb, 1, 1, 1, 1);

        m_label = new QLabel(ThirdPartyBinaryInstallDialog);
        m_label->setObjectName("m_label");
        m_label->setAlignment(Qt::AlignLeading|Qt::AlignLeft|Qt::AlignTop);
        m_label->setMargin(5);

        gridLayout->addWidget(m_label, 0, 0, 1, 3);


        retranslateUi(ThirdPartyBinaryInstallDialog);
        QObject::connect(m_button_box, &QDialogButtonBox::accepted, ThirdPartyBinaryInstallDialog, qOverload<>(&QDialog::accept));
        QObject::connect(m_button_box, &QDialogButtonBox::rejected, ThirdPartyBinaryInstallDialog, qOverload<>(&QDialog::reject));

        QMetaObject::connectSlotsByName(ThirdPartyBinaryInstallDialog);
    } // setupUi

    void retranslateUi(QDialog *ThirdPartyBinaryInstallDialog)
    {
        ThirdPartyBinaryInstallDialog->setWindowTitle(QCoreApplication::translate("ThirdPartyBinaryInstallDialog", "Logiciel tiers requis", nullptr));
        m_download_pb->setText(QCoreApplication::translate("ThirdPartyBinaryInstallDialog", "T\303\251lechargement", nullptr));
        m_install_dir_pb->setText(QCoreApplication::translate("ThirdPartyBinaryInstallDialog", "Dossier installation", nullptr));
        m_label->setText(QString());
    } // retranslateUi

};

namespace Ui {
    class ThirdPartyBinaryInstallDialog: public Ui_ThirdPartyBinaryInstallDialog {};
} // namespace Ui

QT_END_NAMESPACE

#endif // UI_THIRDPARTYBINARYINSTALLDIALOG_H

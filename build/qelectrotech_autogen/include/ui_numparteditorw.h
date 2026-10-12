/********************************************************************************
** Form generated from reading UI file 'numparteditorw.ui'
**
** Created by: Qt User Interface Compiler version 6.11.0
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef UI_NUMPARTEDITORW_H
#define UI_NUMPARTEDITORW_H

#include <QtCore/QVariant>
#include <QtWidgets/QApplication>
#include <QtWidgets/QComboBox>
#include <QtWidgets/QHBoxLayout>
#include <QtWidgets/QLineEdit>
#include <QtWidgets/QSpinBox>
#include <QtWidgets/QWidget>

QT_BEGIN_NAMESPACE

class Ui_NumPartEditorW
{
public:
    QHBoxLayout *horizontalLayout_2;
    QComboBox *type_cb;
    QLineEdit *value_field;
    QSpinBox *increase_spinBox;
    QSpinBox *modulus_spinBox;
    QLineEdit *format_le;

    void setupUi(QWidget *NumPartEditorW)
    {
        if (NumPartEditorW->objectName().isEmpty())
            NumPartEditorW->setObjectName("NumPartEditorW");
        NumPartEditorW->resize(368, 33);
        horizontalLayout_2 = new QHBoxLayout(NumPartEditorW);
        horizontalLayout_2->setSpacing(3);
        horizontalLayout_2->setObjectName("horizontalLayout_2");
        horizontalLayout_2->setContentsMargins(0, 2, 0, 2);
        type_cb = new QComboBox(NumPartEditorW);
        type_cb->setObjectName("type_cb");
        QSizePolicy sizePolicy(QSizePolicy::Policy::MinimumExpanding, QSizePolicy::Policy::Fixed);
        sizePolicy.setHorizontalStretch(0);
        sizePolicy.setVerticalStretch(0);
        sizePolicy.setHeightForWidth(type_cb->sizePolicy().hasHeightForWidth());
        type_cb->setSizePolicy(sizePolicy);
        type_cb->setEditable(true);

        horizontalLayout_2->addWidget(type_cb);

        value_field = new QLineEdit(NumPartEditorW);
        value_field->setObjectName("value_field");
        sizePolicy.setHeightForWidth(value_field->sizePolicy().hasHeightForWidth());
        value_field->setSizePolicy(sizePolicy);
        value_field->setClearButtonEnabled(true);

        horizontalLayout_2->addWidget(value_field);

        increase_spinBox = new QSpinBox(NumPartEditorW);
        increase_spinBox->setObjectName("increase_spinBox");
        increase_spinBox->setEnabled(true);
        sizePolicy.setHeightForWidth(increase_spinBox->sizePolicy().hasHeightForWidth());
        increase_spinBox->setSizePolicy(sizePolicy);
        increase_spinBox->setMinimumSize(QSize(0, 0));
        increase_spinBox->setWrapping(false);
        increase_spinBox->setAlignment(Qt::AlignCenter);
        increase_spinBox->setAccelerated(true);
        increase_spinBox->setMinimum(0);

        horizontalLayout_2->addWidget(increase_spinBox);

        modulus_spinBox = new QSpinBox(NumPartEditorW);
        modulus_spinBox->setObjectName("modulus_spinBox");
        modulus_spinBox->setEnabled(false);
        sizePolicy.setHeightForWidth(modulus_spinBox->sizePolicy().hasHeightForWidth());
        modulus_spinBox->setSizePolicy(sizePolicy);
        modulus_spinBox->setAlignment(Qt::AlignCenter);
        modulus_spinBox->setAccelerated(true);
        modulus_spinBox->setMinimum(0);
        modulus_spinBox->setMaximum(99999);

        horizontalLayout_2->addWidget(modulus_spinBox);

        format_le = new QLineEdit(NumPartEditorW);
        format_le->setObjectName("format_le");
        format_le->setEnabled(false);
        format_le->setMaximumSize(QSize(70, 16777215));
        format_le->setAlignment(Qt::AlignCenter);

        horizontalLayout_2->addWidget(format_le);


        retranslateUi(NumPartEditorW);

        QMetaObject::connectSlotsByName(NumPartEditorW);
    } // setupUi

    void retranslateUi(QWidget *NumPartEditorW)
    {
        NumPartEditorW->setWindowTitle(QCoreApplication::translate("NumPartEditorW", "Form", nullptr));
#if QT_CONFIG(tooltip)
        increase_spinBox->setToolTip(QCoreApplication::translate("NumPartEditorW", "Mettre \303\240 0 pour un chiffre qui n'avance que par le report d'un chiffre cyclique suivant (ex: le \"0\" de \"0.7\")", nullptr));
#endif // QT_CONFIG(tooltip)
        increase_spinBox->setSpecialValueText(QString());
        increase_spinBox->setPrefix(QString());
#if QT_CONFIG(tooltip)
        modulus_spinBox->setToolTip(QCoreApplication::translate("NumPartEditorW", "Valeur \303\240 laquelle ce chiffre revient \303\240 0 en incr\303\251mentant le chiffre pr\303\251c\303\251dent (0 = pas de cycle)", nullptr));
#endif // QT_CONFIG(tooltip)
        modulus_spinBox->setPrefix(QCoreApplication::translate("NumPartEditorW", "mod. ", nullptr));
#if QT_CONFIG(tooltip)
        format_le->setToolTip(QCoreApplication::translate("NumPartEditorW", "Format d'affichage : une suite de z\303\251ros donne le nombre minimum de chiffres (00 = 07, 000 = 007). Vide = largeur naturelle du type.", nullptr));
#endif // QT_CONFIG(tooltip)
        format_le->setPlaceholderText(QCoreApplication::translate("NumPartEditorW", "0", nullptr));
    } // retranslateUi

};

namespace Ui {
    class NumPartEditorW: public Ui_NumPartEditorW {};
} // namespace Ui

QT_END_NAMESPACE

#endif // UI_NUMPARTEDITORW_H

/********************************************************************************
** Form generated from reading UI file 'diagrameditorhandlersizewidget.ui'
**
** Created by: Qt User Interface Compiler version 6.11.0
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef UI_DIAGRAMEDITORHANDLERSIZEWIDGET_H
#define UI_DIAGRAMEDITORHANDLERSIZEWIDGET_H

#include <QtCore/QVariant>
#include <QtWidgets/QApplication>
#include <QtWidgets/QComboBox>
#include <QtWidgets/QHBoxLayout>
#include <QtWidgets/QLabel>
#include <QtWidgets/QWidget>

QT_BEGIN_NAMESPACE

class Ui_DiagramEditorHandlerSizeWidget
{
public:
    QHBoxLayout *horizontalLayout;
    QLabel *label;
    QComboBox *comboBox;

    void setupUi(QWidget *DiagramEditorHandlerSizeWidget)
    {
        if (DiagramEditorHandlerSizeWidget->objectName().isEmpty())
            DiagramEditorHandlerSizeWidget->setObjectName("DiagramEditorHandlerSizeWidget");
        DiagramEditorHandlerSizeWidget->resize(471, 62);
        horizontalLayout = new QHBoxLayout(DiagramEditorHandlerSizeWidget);
        horizontalLayout->setObjectName("horizontalLayout");
        label = new QLabel(DiagramEditorHandlerSizeWidget);
        label->setObjectName("label");

        horizontalLayout->addWidget(label);

        comboBox = new QComboBox(DiagramEditorHandlerSizeWidget);
        comboBox->addItem(QString());
        comboBox->addItem(QString());
        comboBox->addItem(QString());
        comboBox->setObjectName("comboBox");

        horizontalLayout->addWidget(comboBox);


        retranslateUi(DiagramEditorHandlerSizeWidget);

        QMetaObject::connectSlotsByName(DiagramEditorHandlerSizeWidget);
    } // setupUi

    void retranslateUi(QWidget *DiagramEditorHandlerSizeWidget)
    {
        DiagramEditorHandlerSizeWidget->setWindowTitle(QCoreApplication::translate("DiagramEditorHandlerSizeWidget", "Form", nullptr));
        label->setText(QCoreApplication::translate("DiagramEditorHandlerSizeWidget", "Poign\303\251es :", nullptr));
        comboBox->setItemText(0, QCoreApplication::translate("DiagramEditorHandlerSizeWidget", "x 1", nullptr));
        comboBox->setItemText(1, QCoreApplication::translate("DiagramEditorHandlerSizeWidget", "x 2", nullptr));
        comboBox->setItemText(2, QCoreApplication::translate("DiagramEditorHandlerSizeWidget", "x 3", nullptr));

    } // retranslateUi

};

namespace Ui {
    class DiagramEditorHandlerSizeWidget: public Ui_DiagramEditorHandlerSizeWidget {};
} // namespace Ui

QT_END_NAMESPACE

#endif // UI_DIAGRAMEDITORHANDLERSIZEWIDGET_H

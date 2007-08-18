#include "textfieldeditor.h"
#include "parttextfield.h"

/**
	Constructeur
	@param part Champ de texte a editer
	@param parent QWidget parent
*/
TextFieldEditor::TextFieldEditor(PartTextField *textfield, QWidget *parent) : QWidget(parent) {
	part = textfield;
	
	qle_x     = new QLineEdit();
	qle_y     = new QLineEdit();
	qle_text  = new QLineEdit();
	font_size = new QSpinBox();
	font_size -> setRange(0, 144);
	rotate    = new QCheckBox(tr("Maintenir horizontal malgr\351\n les rotations de l'\351l\351ment"));
	rotate -> setChecked(true);
	
	QVBoxLayout *main_layout = new QVBoxLayout();
	main_layout -> addWidget(new QLabel(tr("Postion : ")));
	
	QHBoxLayout *position = new QHBoxLayout();
	position -> addWidget(new QLabel(tr("x : ")));
	position -> addWidget(qle_x                 );
	position -> addWidget(new QLabel(tr("y : ")));
	position -> addWidget(qle_y                 );
	main_layout -> addLayout(position);
	
	QHBoxLayout *fs = new QHBoxLayout();
	fs -> addWidget(new QLabel(tr("Taille : ")));
	fs -> addWidget(font_size);
	main_layout -> addLayout(fs);
	
	QHBoxLayout *t = new QHBoxLayout();
	t -> addWidget(new QLabel(tr("Texte par d\351faut : ")));
	t -> addWidget(qle_text);
	main_layout -> addLayout(t);
	
	QHBoxLayout *r = new QHBoxLayout();
	r -> addWidget(rotate);
	main_layout -> addLayout(r);
	
	main_layout -> addStretch();
	setLayout(main_layout);
	
	connect(qle_x,     SIGNAL(textEdited(const QString &)), this, SLOT(updateTextField()));
	connect(qle_y,     SIGNAL(textEdited(const QString &)), this, SLOT(updateTextField()));
	connect(qle_text,  SIGNAL(textEdited(const QString &)), this, SLOT(updateTextField()));
	connect(font_size, SIGNAL(valueChanged(int)),           this, SLOT(updateTextField()));
	connect(rotate,    SIGNAL(stateChanged(int)),           this, SLOT(updateTextField()));
}

/**
	Destructeur
*/
TextFieldEditor::~TextFieldEditor() {
	qDebug() << "~TextFieldEditor()";
}

/**
	Met a jour le champ de texte a partir des donnees du formulaire
*/
void TextFieldEditor::updateTextField() {
	part -> can_check_changes = false;
	part -> setFont(QFont(part -> font().family(), font_size -> value()));
	part -> setPlainText(qle_text -> text());
	part -> setPos(qle_x -> text().toDouble(), qle_y -> text().toDouble());
	part -> setFollowParentRotations(!rotate -> isChecked());
	part -> can_check_changes = true;
}

/**
	Met a jour le formulaire a partir du champ de texte
*/
void TextFieldEditor::updateForm() {
	qle_x -> setText(QString("%1").arg(part -> pos().x()));
	qle_y -> setText(QString("%1").arg(part -> pos().y()));
	qle_text -> setText(part -> toPlainText());
	font_size -> setValue(part -> font().pointSize());
	rotate -> setChecked(!part -> followParentRotations());
}

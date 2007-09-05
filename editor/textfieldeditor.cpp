#include "textfieldeditor.h"
#include "parttextfield.h"

/**
	Constructeur
	@param part Champ de texte a editer
	@param parent QWidget parent
*/
TextFieldEditor::TextFieldEditor(QETElementEditor *editor, PartTextField *textfield, QWidget *parent) : ElementItemEditor(editor, parent) {
	part = textfield;
	
	qle_x     = new QLineEdit();
	qle_y     = new QLineEdit();
	qle_text  = new QLineEdit();
	font_size = new QSpinBox();
	font_size -> setRange(0, 144);
	rotate    = new QCheckBox(tr("Maintenir horizontal malgr\351\n les rotations de l'\351l\351ment"));
	rotate -> setChecked(true);
	
	QVBoxLayout *main_layout = new QVBoxLayout();
	main_layout -> addWidget(new QLabel(tr("Position : ")));
	
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
	updateForm();
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
	part -> setProperty("size", font_size -> value());
	part -> setPlainText(qle_text -> text());
	part -> setPos(qle_x -> text().toDouble(), qle_y -> text().toDouble());
	part -> setFollowParentRotations(!rotate -> isChecked());
}

void TextFieldEditor::updateTextFieldX() { addChangePartCommand(tr("abscisse"),        part, "x",      qle_x -> text().toDouble()); updateForm(); }
void TextFieldEditor::updateTextFieldY() { addChangePartCommand(tr("ordonn\351e"),     part, "y",      qle_y -> text().toDouble()); updateForm(); }
void TextFieldEditor::updateTextFieldT() { addChangePartCommand(tr("texte"),           part, "text",   qle_text -> text());         }
void TextFieldEditor::updateTextFieldS() { addChangePartCommand(tr("taille"),          part, "size",   font_size -> value());       }
void TextFieldEditor::updateTextFieldR() { addChangePartCommand(tr("propri\351t\351"), part, "rotate", !rotate -> isChecked());     }

/**
	Met a jour le formulaire a partir du champ de texte
*/
void TextFieldEditor::updateForm() {
	activeConnections(false);
	qle_x     -> setText(part -> property("x").toString());
	qle_y     -> setText(part -> property("y").toString());
	qle_text  -> setText(part -> property("text").toString());
	font_size -> setValue(part -> property("size").toInt());
	rotate  -> setChecked(!part -> property("rotate").toBool());
	activeConnections(true);
}

void TextFieldEditor::activeConnections(bool active) {
	if (active) {
		connect(qle_x,     SIGNAL(editingFinished()), this, SLOT(updateTextFieldX()));
		connect(qle_y,     SIGNAL(editingFinished()), this, SLOT(updateTextFieldY()));
		connect(qle_text,  SIGNAL(editingFinished()), this, SLOT(updateTextFieldT()));
		connect(font_size, SIGNAL(editingFinished()), this, SLOT(updateTextFieldS()));
		connect(rotate,    SIGNAL(stateChanged(int)), this, SLOT(updateTextFieldR()));
	} else {
		disconnect(qle_x,     SIGNAL(editingFinished()), this, SLOT(updateTextFieldX()));
		disconnect(qle_y,     SIGNAL(editingFinished()), this, SLOT(updateTextFieldY()));
		disconnect(qle_text,  SIGNAL(editingFinished()), this, SLOT(updateTextFieldT()));
		disconnect(font_size, SIGNAL(editingFinished()), this, SLOT(updateTextFieldS()));
		disconnect(rotate,    SIGNAL(stateChanged(int)), this, SLOT(updateTextFieldR()));
	}
}

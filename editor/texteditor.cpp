#include "texteditor.h"
#include "parttext.h"

/**
	Constructeur
	@param editor L'editeur d'element concerne
	@param text Champ de texte a editer
	@param parent QWidget parent de ce widget
*/
TextEditor::TextEditor(QETElementEditor *editor, PartText *text, QWidget *parent) : ElementItemEditor(editor, parent) {
	part = text;
	
	qle_x     = new QLineEdit();
	qle_y     = new QLineEdit();
	qle_text  = new QLineEdit();
	font_size = new QSpinBox();
	font_size -> setRange(0, 144);
	
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
	t -> addWidget(new QLabel(tr("Texte : ")));
	t -> addWidget(qle_text);
	main_layout -> addLayout(t);
	main_layout -> addStretch();
	setLayout(main_layout);
	
	
	
	updateForm();
}

/**
	Destructeur
*/
TextEditor::~TextEditor() {
}

/**
	Met a jour le champ de texte a partir des donnees du formulaire
*/
void TextEditor::updateText() {
	part -> setProperty("size", font_size -> value());
	part -> setPlainText(qle_text -> text());
	part -> setPos(qle_x -> text().toDouble(), qle_y -> text().toDouble());
}

void TextEditor::updateTextX() { addChangePartCommand(tr("abscisse"),    part, "x",    qle_x -> text().toDouble()); updateForm(); }
void TextEditor::updateTextY() { addChangePartCommand(tr("ordonn\351e"), part, "y",    qle_y -> text().toDouble()); updateForm(); }
void TextEditor::updateTextT() { addChangePartCommand(tr("texte"),       part, "text", qle_text -> text());         }
void TextEditor::updateTextS() { addChangePartCommand(tr("taille"),      part, "size", font_size -> value());       }

/**
	Met a jour le formulaire a partir du champ de texte
*/
void TextEditor::updateForm() {
	activeConnections(false);
	qle_x     -> setText(part -> property("x").toString());
	qle_y     -> setText(part -> property("y").toString());
	qle_text  -> setText(part -> property("text").toString());
	font_size -> setValue(part -> property("size").toInt());
	activeConnections(true);
}

void TextEditor::activeConnections(bool active) {
	if (active) {
		connect(qle_x,     SIGNAL(editingFinished()), this, SLOT(updateTextX()));
		connect(qle_y,     SIGNAL(editingFinished()), this, SLOT(updateTextY()));
		connect(qle_text,  SIGNAL(editingFinished()), this, SLOT(updateTextT()));
		connect(font_size, SIGNAL(editingFinished()), this, SLOT(updateTextS()));
	} else {
		disconnect(qle_x,     SIGNAL(editingFinished()), this, SLOT(updateTextX()));
		disconnect(qle_y,     SIGNAL(editingFinished()), this, SLOT(updateTextY()));
		disconnect(qle_text,  SIGNAL(editingFinished()), this, SLOT(updateTextT()));
		disconnect(font_size, SIGNAL(editingFinished()), this, SLOT(updateTextS()));
	}
}

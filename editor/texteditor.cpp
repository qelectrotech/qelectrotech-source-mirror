#include "texteditor.h"
#include "parttext.h"

/**
	Constructeur
	@param term Champ de texte a editer
	@param parent QWidget parent de ce widget
*/
TextEditor::TextEditor(PartText *text, QWidget *parent) : QWidget(parent) {
	part = text;
	
	qle_x     = new QLineEdit();
	qle_y     = new QLineEdit();
	qle_text  = new QLineEdit();
	font_size = new QSpinBox();
	font_size -> setRange(0, 144);
	
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
	t -> addWidget(new QLabel(tr("Texte : ")));
	t -> addWidget(qle_text);
	main_layout -> addLayout(t);
	main_layout -> addStretch();
	setLayout(main_layout);
	
	connect(qle_x,     SIGNAL(textEdited(const QString &)), this, SLOT(updateText()));
	connect(qle_y,     SIGNAL(textEdited(const QString &)), this, SLOT(updateText()));
	connect(qle_text,  SIGNAL(textEdited(const QString &)), this, SLOT(updateText()));
	connect(font_size, SIGNAL(valueChanged(int)),           this, SLOT(updateText()));
	
	//updateForm();
}

/**
	Destructeur
*/
TextEditor::~TextEditor() {
	qDebug() << "~TextEditor()";
}

void TextEditor::updateText() {
	part -> can_check_changes = false;
	part -> setFont(QFont(part -> font().family(), font_size -> value()));
	part -> setPlainText(qle_text -> text());
	part -> setPos(qle_x -> text().toDouble(), qle_y -> text().toDouble());
	part ->  can_check_changes = true;
}

void TextEditor::updateForm() {
	qle_x -> setText(QString("%1").arg(part -> pos().x()));
	qle_y -> setText(QString("%1").arg(part -> pos().y()));
	qle_text -> setText(part -> toPlainText());
	font_size -> setValue(part -> font().pointSize());
}

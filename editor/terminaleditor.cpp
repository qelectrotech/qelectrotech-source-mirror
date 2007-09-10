#include "terminaleditor.h"
#include "partterminal.h"

/**
	Constructeur
	@param term Borne a editer
	@param parent QWidget parent de ce widget
*/
TerminalEditor::TerminalEditor(QETElementEditor *editor, PartTerminal *term, QWidget *parent) : ElementItemEditor(editor, parent) {
	part = term;
	
	qle_x = new QLineEdit();
	qle_y = new QLineEdit();
	
	orientation = new QComboBox();
	orientation -> addItem(QIcon(":/ico/north.png"), tr("Nord"),  QET::North);
	orientation -> addItem(QIcon(":/ico/east.png"),  tr("Est"),   QET::East);
	orientation -> addItem(QIcon(":/ico/south.png"), tr("Sud"),   QET::South);
	orientation -> addItem(QIcon(":/ico/west.png"),  tr("Ouest"), QET::West);
	
	QVBoxLayout *main_layout = new QVBoxLayout();
	main_layout -> addWidget(new QLabel(tr("Position : ")));
	
	QHBoxLayout *position = new QHBoxLayout();
	position -> addWidget(new QLabel(tr("x : ")));
	position -> addWidget(qle_x                 );
	position -> addWidget(new QLabel(tr("y : ")));
	position -> addWidget(qle_y                 );
	main_layout -> addLayout(position);
	
	QHBoxLayout *ori = new QHBoxLayout();
	ori -> addWidget(new QLabel(tr("Orientation : ")));
	ori -> addWidget(orientation                     );
	main_layout -> addLayout(ori);
	main_layout -> addStretch();
	setLayout(main_layout);
	
	activeConnections(true);
	updateForm();
}

/**
	Destructeur
*/
TerminalEditor::~TerminalEditor() {
};

void TerminalEditor::updateTerminal() {
	part -> setPos(qle_x -> text().toDouble(), qle_y -> text().toDouble());
	part -> setOrientation(
		static_cast<QET::Orientation>(
			orientation -> itemData(
				orientation -> currentIndex()
			).toInt()
		)
	);
}

void TerminalEditor::updateTerminalX() { addChangePartCommand(tr("abscisse"),    part, "x",           qle_x -> text().toDouble()); updateForm(); }
void TerminalEditor::updateTerminalY() { addChangePartCommand(tr("ordonn\351e"), part, "y",           qle_y -> text().toDouble()); updateForm(); }
void TerminalEditor::updateTerminalO() { addChangePartCommand(tr("orientation"), part, "orientation", orientation -> itemData(orientation -> currentIndex()).toInt()); }

void TerminalEditor::updateForm() {
	activeConnections(false);
	qle_x -> setText(part -> property("x").toString());
	qle_y -> setText(part -> property("y").toString());
	orientation -> setCurrentIndex(static_cast<int>(part -> orientation()));
	activeConnections(true);
}

void TerminalEditor::activeConnections(bool active) {
	if (active) {
		connect(qle_x,       SIGNAL(editingFinished()), this, SLOT(updateTerminalX()));
		connect(qle_y,       SIGNAL(editingFinished()), this, SLOT(updateTerminalY()));
		connect(orientation, SIGNAL(activated(int)),    this, SLOT(updateTerminalO()));
	} else {
		disconnect(qle_x,       SIGNAL(editingFinished()), this, SLOT(updateTerminalX()));
		disconnect(qle_y,       SIGNAL(editingFinished()), this, SLOT(updateTerminalY()));
		disconnect(orientation, SIGNAL(activated(int)),    this, SLOT(updateTerminalO()));
	}
}

#include "terminaleditor.h"
#include "partterminal.h"

/**
	Constructeur
	@param term Borne a editer
	@param parent QWidget parent de ce widget
*/
TerminalEditor::TerminalEditor(PartTerminal *term, QWidget *parent) : QWidget(parent) {
	part = term;
	
	qle_x = new QLineEdit();
	qle_y = new QLineEdit();
	
	orientation = new QComboBox();
	orientation -> addItem(QIcon(":/ico/north.png"), tr("Nord"),  QET::North);
	orientation -> addItem(QIcon(":/ico/east.png"),  tr("Est"),   QET::East);
	orientation -> addItem(QIcon(":/ico/south.png"), tr("Sud"),   QET::South);
	orientation -> addItem(QIcon(":/ico/west.png"),  tr("Ouest"), QET::West);
	
	QVBoxLayout *main_layout = new QVBoxLayout();
	main_layout -> addWidget(new QLabel(tr("Postion : ")));
	
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
	
	connect(qle_x,       SIGNAL(textEdited(const QString &)), this, SLOT(updateTerminal()));
	connect(qle_y,       SIGNAL(textEdited(const QString &)), this, SLOT(updateTerminal()));
	connect(orientation, SIGNAL(activated(int)),              this, SLOT(updateTerminal()));
	
	updateForm();
}

/**
	Destructeur
*/
TerminalEditor::~TerminalEditor() {
	qDebug() << "~TerminalEditor()";
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

void TerminalEditor::updateForm() {
	qle_x -> setText(QString("%1").arg(part -> pos().x()));
	qle_y -> setText(QString("%1").arg(part -> pos().y()));
	orientation -> setCurrentIndex(static_cast<int>(part -> orientation()));
}

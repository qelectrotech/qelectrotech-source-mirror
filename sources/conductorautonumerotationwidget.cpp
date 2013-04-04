#include "conductorautonumerotationwidget.h"
#include "conductorproperties.h"
#include "diagramcommands.h"
#include "diagram.h"

ConductorAutoNumerotationWidget::ConductorAutoNumerotationWidget(Conductor *c, QSet<Conductor *> cl, QWidget *parent) :
	conductor_(c),
	c_list(cl),
	QDialog (parent)
{
#ifdef Q_WS_MAC
	setWindowFlags(Qt::Sheet);
#endif
	buildInterface();
}

void ConductorAutoNumerotationWidget::buildInterface() {
	QVBoxLayout *mainlayout = new QVBoxLayout;
	QGroupBox *potential_groupbox = new QGroupBox(tr("Textes de potentiel"), this);
	QVBoxLayout *vlayout = new QVBoxLayout;

	QLabel *label= new QLabel(tr("Les textes de ce potentiel \351lectrique ne sont pas identiques.\n"
								 "Appliquer un texte \340 l'ensemble de ces conducteurs?"), this);
	vlayout -> addWidget(label);

	//map the signal for each radio button create in buildRadioList
	sm_ = new QSignalMapper(this);
	connect(sm_, SIGNAL(mapped(QString)), this, SLOT(setText(QString)));
	vlayout -> addLayout(buildRadioList());

	potential_groupbox -> setLayout(vlayout);
	mainlayout -> addWidget(potential_groupbox);

	QDialogButtonBox *dbb = new QDialogButtonBox(QDialogButtonBox::Cancel | QDialogButtonBox::Yes, Qt::Horizontal, this);
	connect(dbb, SIGNAL(rejected()),
			this, SLOT(reject()));
	connect(dbb, SIGNAL(accepted()),
			this, SLOT(accept()));

	mainlayout->addWidget(dbb);
	setLayout(mainlayout);
}

/**
 * @brief ConductorAutoNumerotationWidget::buildRadioList
 *construit toute la partie de l'interface contenant les boutons radio permetant le choix du texte a appliquer
 * @return un layout contenant les boutons radio
 */
QVBoxLayout* ConductorAutoNumerotationWidget::buildRadioList() {
	QVBoxLayout *radioLayout = new QVBoxLayout;
	QHBoxLayout *otherLayout = new QHBoxLayout;

	//create a new radio button for each text of @conductorList
	QMultiMap<int, QString> conductorlist = conductorsTextToMap(c_list);
	for (QMultiMap<int, QString>::ConstIterator it = conductorlist.constEnd()-1; it != conductorlist.constBegin()-1; --it) {
		QRadioButton *rb= new QRadioButton(it.value() + tr("  : est pr\351sent ") + QString::number(it.key()) + tr(" fois."), this);
		if (it == conductorlist.constEnd()-1) {
			rb -> setChecked(true);
			text_ = it.value();
		}
		//connect the button to mapper @sm_
		connect(rb, SIGNAL(clicked()),
				sm_, SLOT(map()));
		sm_ -> setMapping(rb, it.value());
		radioLayout -> addWidget(rb);
	}

	//create the "other" radio button and is text field
	QRadioButton *other= new QRadioButton(tr("Autre"), this);
	text_field = new QLineEdit(this);
	text_field -> setEnabled(false);
	connect(other, SIGNAL(toggled(bool)), text_field, SLOT(setEnabled(bool)));
	otherLayout -> addWidget(other);
	otherLayout -> addWidget(text_field);
	radioLayout -> addLayout(otherLayout);
	return radioLayout;
}

/**
 * @param csl liste des conducteurs a analyser
 * @return QMultiMap avec le nombre de conducteurs possedant le même texte en clee et le texte en question comme valeur
 */
QMultiMap <int, QString> ConductorAutoNumerotationWidget::conductorsTextToMap(QSet<Conductor *> csl) {
	QStringList textList;
	foreach(Conductor *c, csl) textList << c -> text();

	QMultiMap<int, QString> conductorlist;
	while (!textList.size() == 0) {
		QString t = textList.at(0);
		int n = textList.count(t);
		textList.removeAll(t);
		conductorlist.insert(n, t);
	}
	return conductorlist;
}

/**
 * @brief ConductorAutoNumerotationWidget::applyText
 *applique le texte selectionne @text_ a tout les conducteur de @c_list et a @conducteur_
 */
void ConductorAutoNumerotationWidget::applyText() {
	QList <ConductorProperties> old_properties, new_properties;
	ConductorProperties cp;
	foreach (Conductor *c, c_list) {
		old_properties << c -> properties();
		cp = c -> properties();
		cp.text = text_;
		c -> setProperties(cp);
		new_properties << c -> properties();
		c -> setText(text_);
	}
	// initialise l'objet UndoCommand correspondant
	ChangeSeveralConductorsPropertiesCommand *cscpc = new ChangeSeveralConductorsPropertiesCommand(c_list);
	cscpc -> setOldSettings(old_properties);
	cscpc -> setNewSettings(new_properties);
	conductor_ -> diagram() -> undoStack().push(cscpc);

	cp = conductor_ -> properties();
	cp.text = text_;
	conductor_ -> setProperties(cp);
	conductor_ -> setText(text_);
}

/**
 * @brief ConductorAutoNumerotationWidget::setText
 * enregistre le texte @t passé en parametre
 */
void ConductorAutoNumerotationWidget::setText(QString t) {
	text_ = t;
}

/**
 * @brief ConductorAutoNumerotationWidget::accept
 *action executé lors de l'appuis sur le bouton 'oui'
 */
void ConductorAutoNumerotationWidget::accept() {
	if (text_field -> isEnabled()) {
		text_ = text_field -> text();
		applyText();
		}
	else
		applyText();
	close();
}

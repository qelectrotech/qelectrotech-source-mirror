#include "treecoloranimation.h"

/**
	Constructor
	@param items List of items whose background color will be animated.
	@param parent Parent QObject
*/
TreeColorAnimation::TreeColorAnimation(const QList<QTreeWidgetItem *> &items, QObject *parent) : 
	QVariantAnimation(parent),
	items_(items)
{
}

/**
	Destructor
*/
TreeColorAnimation::~TreeColorAnimation() {
}

/**
	@return the list of items whose background color will be animated.
*/
QList<QTreeWidgetItem *> TreeColorAnimation::items() const {
	return(items_);
}

/**
	Apply the provided color to animated items.
	@param color Color to be applied on animated items.
*/
void TreeColorAnimation::updateCurrentValue(const QVariant &color) {
	foreach (QTreeWidgetItem *item, items_) {
		item -> setBackgroundColor(0, color.value<QColor>());
	}
}

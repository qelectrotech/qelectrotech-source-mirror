#include "customelementpart.h"
#include "customelement.h"

QPicture *CustomElementPart::getCustomElementQPicture(CustomElement &ce) const {
	return(&(ce.dessin));
}

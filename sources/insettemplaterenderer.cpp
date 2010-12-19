#include "insettemplaterenderer.h"
#include "insettemplate.h"

/**
	Constructor
	@param parnet Parent QObject of this renderer
*/
InsetTemplateRenderer::InsetTemplateRenderer(QObject *parent) :
	QObject(parent),
	inset_template_(0),
	last_known_inset_width_(-1)
{
}

/**
	Destructor
*/
InsetTemplateRenderer::~InsetTemplateRenderer() {
}

/**
	@return the inset template used for the rendering
*/
const InsetTemplate *InsetTemplateRenderer::insetTemplate() const {
	return(inset_template_);
}

/**
	@param inset_template Inset template to render.
*/
void InsetTemplateRenderer::setInsetTemplate(const InsetTemplate *inset_template) {
	if (inset_template != inset_template_) {
		inset_template_ = inset_template;
		invalidateRenderedTemplate();
	}
}

/**
	@param context Diagram Context to use when rendering the inset
*/
void InsetTemplateRenderer::setContext(const DiagramContext &context) {
	context_ = context;
	invalidateRenderedTemplate();
}

/**
	@return the height of the rendered template, or -1 if no template has been
	set for this renderer.
	@see InsetTemplate::height()
*/
int InsetTemplateRenderer::height() const {
	if (!inset_template_) return(-1);
	return(inset_template_ -> height());
}

/**
	Render the inset.
	@param provided_painter QPainter to use to render the inset.
	@param inset_width The total width of the inset to render
*/
void InsetTemplateRenderer::render(QPainter *provided_painter, int inset_width) {
	if (!inset_template_) return;
	
	// Do we really need to calculate all this again?
	if (inset_width != last_known_inset_width_ || rendered_template_.isNull()) {
		renderToQPicture(inset_width);
	}
	
	provided_painter -> save();
	rendered_template_.play(provided_painter);
	provided_painter -> restore();
}

/**
	Renders the inset to the internal QPicture
	@param inset_width Width of the inset to render
*/
void InsetTemplateRenderer::renderToQPicture(int inset_width) {
	if (!inset_template_) return;
	
	// we render the template on our internal QPicture
	QPainter painter(&rendered_template_);
	
	inset_template_ -> render(painter, context_, inset_width);
	
	// memorize the last known width
	last_known_inset_width_ = inset_width;
}

/**
	Invalidates the previous rendering of the template by resetting the internal
	QPicture.
*/
void InsetTemplateRenderer::invalidateRenderedTemplate() {
	rendered_template_ = QPicture();
}

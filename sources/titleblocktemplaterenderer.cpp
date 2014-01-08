#include "titleblocktemplaterenderer.h"
#include "titleblocktemplate.h"

/**
	Constructor
	@param parnet Parent QObject of this renderer
*/
TitleBlockTemplateRenderer::TitleBlockTemplateRenderer(QObject *parent) :
	QObject(parent),
	titleblock_template_(0),
	use_cache_(true),
	last_known_titleblock_width_(-1)
{
}

/**
	Destructor
*/
TitleBlockTemplateRenderer::~TitleBlockTemplateRenderer() {
}

/**
	@return the titleblock template used for the rendering
*/
const TitleBlockTemplate *TitleBlockTemplateRenderer::titleBlockTemplate() const {
	return(titleblock_template_);
}

/**
	@param titleblock_template TitleBlock template to render.
*/
void TitleBlockTemplateRenderer::setTitleBlockTemplate(const TitleBlockTemplate *titleblock_template) {
	if (titleblock_template != titleblock_template_) {
		titleblock_template_ = titleblock_template;
		invalidateRenderedTemplate();
	}
}

/**
	@param context Diagram Context to use when rendering the titleblock
*/
void TitleBlockTemplateRenderer::setContext(const DiagramContext &context) {
	context_ = context;
	invalidateRenderedTemplate();
}

/**
	@return the height of the rendered template, or -1 if no template has been
	set for this renderer.
	@see TitleBlockTemplate::height()
*/
int TitleBlockTemplateRenderer::height() const {
	if (!titleblock_template_) return(-1);
	return(titleblock_template_ -> height());
}

/**
	Render the titleblock.
	@param provided_painter QPainter to use to render the titleblock.
	@param titleblock_width The total width of the titleblock to render
*/
void TitleBlockTemplateRenderer::render(QPainter *provided_painter, int titleblock_width) {
	if (!titleblock_template_) return;
	
	if (use_cache_) {
		// Do we really need to calculate all this again?
		if (titleblock_width != last_known_titleblock_width_ || rendered_template_.isNull()) {
			renderToQPicture(titleblock_width);
		}
		
		provided_painter -> save();
		rendered_template_.play(provided_painter);
		provided_painter -> restore();
	} else {
		titleblock_template_ -> render(*provided_painter, context_, titleblock_width);
	}
}


void TitleBlockTemplateRenderer::renderDxf(QRectF &title_block_rect, int titleblock_width, QString &file_path, int color) {
	if (!titleblock_template_) return;
	titleblock_template_ -> renderDxf(title_block_rect, context_, titleblock_width, file_path, color);
}

/**
	Renders the titleblock to the internal QPicture
	@param titleblock_width Width of the titleblock to render
*/
void TitleBlockTemplateRenderer::renderToQPicture(int titleblock_width) {
	if (!titleblock_template_) return;
	
	// we render the template on our internal QPicture
	QPainter painter(&rendered_template_);
	
	titleblock_template_ -> render(painter, context_, titleblock_width);
	
	// memorize the last known width
	last_known_titleblock_width_ = titleblock_width;
}

/**
	Invalidates the previous rendering of the template by resetting the internal
	QPicture.
*/
void TitleBlockTemplateRenderer::invalidateRenderedTemplate() {
	rendered_template_ = QPicture();
}

/**
	@param use_cache true for this renderer to use its QPicture-based cache,
	false otherwise.
*/
void TitleBlockTemplateRenderer::setUseCache(bool use_cache) {
	use_cache_ = use_cache;
}

/**
	@return true if this renderer uses its QPicture-based cache, false
	otherwise.
*/
bool TitleBlockTemplateRenderer::useCache() const {
	return(use_cache_);
}


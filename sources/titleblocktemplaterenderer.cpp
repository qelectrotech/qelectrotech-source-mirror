#include "titleblocktemplaterenderer.h"
#include "titleblocktemplate.h"

/**
	Constructor
	@param parnet Parent QObject of this renderer
*/
TitleBlockTemplateRenderer::TitleBlockTemplateRenderer(QObject *parent) :
	QObject(parent),
	m_titleblock_template(nullptr),
	m_use_cache(true),
	m_last_known_titleblock_width(-1)
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
	return(m_titleblock_template);
}

/**
	@param titleblock_template TitleBlock template to render.
*/
void TitleBlockTemplateRenderer::setTitleBlockTemplate(const TitleBlockTemplate *titleblock_template) {
	if (titleblock_template != m_titleblock_template) {
		m_titleblock_template = titleblock_template;
		invalidateRenderedTemplate();
	}
}

/**
 * @brief TitleBlockTemplateRenderer::setContext
 * @param context : Context to use when rendering the titleblock
 */
void TitleBlockTemplateRenderer::setContext(const DiagramContext &context) {
	m_context = context;
	invalidateRenderedTemplate();
}

/**
 * @brief TitleBlockTemplateRenderer::context
 * @return the current diagram context use when render the titleblock
 */
DiagramContext TitleBlockTemplateRenderer::context() const {
	return  m_context;
}

/**
	@return the height of the rendered template, or -1 if no template has been
	set for this renderer.
	@see TitleBlockTemplate::height()
*/
int TitleBlockTemplateRenderer::height() const {
	if (!m_titleblock_template) return(-1);
	return(m_titleblock_template -> height());
}

/**
	Render the titleblock.
	@param provided_painter QPainter to use to render the titleblock.
	@param titleblock_width The total width of the titleblock to render
*/
void TitleBlockTemplateRenderer::render(QPainter *provided_painter, int titleblock_width) {
	if (!m_titleblock_template) return;
	
	if (m_use_cache) {
		// Do we really need to calculate all this again?
		if (titleblock_width != m_last_known_titleblock_width || m_rendered_template.isNull()) {
			renderToQPicture(titleblock_width);
		}
		
		provided_painter -> save();
		m_rendered_template.play(provided_painter);
		provided_painter -> restore();
	} else {
		m_titleblock_template -> render(*provided_painter, m_context, titleblock_width);
	}
}


void TitleBlockTemplateRenderer::renderDxf(QRectF &title_block_rect, int titleblock_width, QString &file_path, int color) {
	if (!m_titleblock_template) return;
	m_titleblock_template -> renderDxf(title_block_rect, m_context, titleblock_width, file_path, color);
}

/**
	Renders the titleblock to the internal QPicture
	@param titleblock_width Width of the titleblock to render
*/
void TitleBlockTemplateRenderer::renderToQPicture(int titleblock_width) {
	if (!m_titleblock_template) return;
	
	// we render the template on our internal QPicture
	QPainter painter(&m_rendered_template);
	
	m_titleblock_template -> render(painter, m_context, titleblock_width);
	
	// memorize the last known width
	m_last_known_titleblock_width = titleblock_width;
}

/**
	Invalidates the previous rendering of the template by resetting the internal
	QPicture.
*/
void TitleBlockTemplateRenderer::invalidateRenderedTemplate() {
	m_rendered_template = QPicture();
}

/**
	@param use_cache true for this renderer to use its QPicture-based cache,
	false otherwise.
*/
void TitleBlockTemplateRenderer::setUseCache(bool use_cache) {
	m_use_cache = use_cache;
}

/**
	@return true if this renderer uses its QPicture-based cache, false
	otherwise.
*/
bool TitleBlockTemplateRenderer::useCache() const {
	return(m_use_cache);
}


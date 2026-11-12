#include "titleblocktemplaterenderer.h"
#include "diagram.h"
#include "properties/projectlanguages.h"
#include "titleblocktemplate.h"

/**
	@brief TitleBlockTemplateRenderer::TitleBlockTemplateRenderer
	Constructor
	@param parent : Parent QObject of this renderer
*/
TitleBlockTemplateRenderer::TitleBlockTemplateRenderer(QObject *parent) :
	QObject(parent),
	m_titleblock_template(nullptr)
{
}

/**
	@brief TitleBlockTemplateRenderer::~TitleBlockTemplateRenderer
	Destructor
*/
TitleBlockTemplateRenderer::~TitleBlockTemplateRenderer()
{
}

/**
	@brief TitleBlockTemplateRenderer::titleBlockTemplate
	@return the titleblock template used for the rendering
*/
const TitleBlockTemplate *TitleBlockTemplateRenderer::titleBlockTemplate() const
{
	return(m_titleblock_template);
}

/**
	@brief TitleBlockTemplateRenderer::setTitleBlockTemplate
	@param titleblock_template : TitleBlock template to render.
*/
void TitleBlockTemplateRenderer::setTitleBlockTemplate(
		const TitleBlockTemplate *titleblock_template) {
	if (titleblock_template != m_titleblock_template) {
		m_titleblock_template = titleblock_template;
	}
		// Always record anew: a template removed from the project and
		// loaded again may come back at the address of the former one
	invalidateCache();
}

/**
	@brief TitleBlockTemplateRenderer::setContext
	@param context : Context to use when rendering the titleblock
*/
void TitleBlockTemplateRenderer::setContext(const DiagramContext &context) {
	m_context = context;
}

/**
	@brief TitleBlockTemplateRenderer::context
	@return the current diagram context use when render the titleblock
*/
DiagramContext TitleBlockTemplateRenderer::context() const
{
	return  m_context;
}

/**
	@brief TitleBlockTemplateRenderer::setLanguages
	@param languages : the project's texts in other languages, which must
	outlive this renderer; nullptr to show the context as it is
*/
void TitleBlockTemplateRenderer::setLanguages(const ProjectLanguages *languages)
{
	m_languages = languages;
}

/**
	@brief TitleBlockTemplateRenderer::shownContext
	@return the context to draw: its texts in the project's active language.
	context() keeps the words as written, for the project database.
*/
DiagramContext TitleBlockTemplateRenderer::shownContext() const
{
	if (!m_languages || m_languages->isEmpty()) return m_context;
	DiagramContext shown;
	for (const QString &key : m_context.keys()) {
		const QVariant value = m_context[key];
		shown.addValue(key,
					   value.typeId() == QMetaType::QString
						   ? QVariant(m_languages->translate(value.toString()))
						   : value,
					   m_context.keyMustShow(key));
	}
	return shown;
}

/**
	@brief TitleBlockTemplateRenderer::height
	@return the height of the rendered template,
	or -1 if no template has been
	set for this renderer.
	@see TitleBlockTemplate::height()
*/
int TitleBlockTemplateRenderer::height() const
{
	if (!m_titleblock_template) return(-1);
	return(m_titleblock_template -> height());
}

/**
	@brief TitleBlockTemplateRenderer::render
	Render the titleblock.
	@param provided_painter : QPainter to use to render the titleblock.
	@param titleblock_width : The total width of the titleblock to render
*/
void TitleBlockTemplateRenderer::render(QPainter *provided_painter,
					int titleblock_width) {
	if (!m_titleblock_template) return;

	const DiagramContext context = shownContext();

		// A print, a PDF, an image or an OpenGL viewport: drawn anew.
		// Only a widget, painted again at every move of the mouse, plays
		// the recording (working the cells and their texts out takes most
		// of the time of a repaint of a folio). A screen whose dpi is not
		// the one of the recording is drawn anew too, so that the texts
		// keep their size.
	const QPaintDevice *device = provided_painter -> device();
	if (!device
			|| device -> devType() != QInternal::Widget
			|| device -> logicalDpiX() != m_cache.logicalDpiX()
			|| device -> logicalDpiY() != m_cache.logicalDpiY())
	{
		m_titleblock_template -> render(*provided_painter,
						context,
						titleblock_width);
		return;
	}

	if (!m_cache_valid
			|| m_cache_template   != m_titleblock_template
			|| m_cache_width      != titleblock_width
			|| m_cache_background != Diagram::background_color
			|| m_cache_pen        != provided_painter -> pen().color()
			|| m_cache_font       != provided_painter -> font()
			|| m_cache_hints      != provided_painter -> renderHints()
			|| m_cache_context    != context)
	{
		m_cache = QPicture();
		QPainter recorder(&m_cache);
		recorder.setPen(provided_painter -> pen());
		recorder.setFont(provided_painter -> font());
		recorder.setRenderHints(provided_painter -> renderHints());
		m_titleblock_template -> render(recorder, context, titleblock_width);
		recorder.end();

		m_cache_valid      = true;
		m_cache_template   = m_titleblock_template;
		m_cache_width      = titleblock_width;
		m_cache_background = Diagram::background_color;
		m_cache_pen        = provided_painter -> pen().color();
		m_cache_font       = provided_painter -> font();
		m_cache_hints      = provided_painter -> renderHints();
		m_cache_context    = context;
	}
	m_cache.play(provided_painter);
}

/**
	@brief TitleBlockTemplateRenderer::invalidateCache
	Forget the title block recorded for the screen: its template changed
	without changing its address, so the next paint records it again.
*/
void TitleBlockTemplateRenderer::invalidateCache()
{
	m_cache_valid = false;
}

/**
	@brief TitleBlockTemplateRenderer::renderDxf
	@param title_block_rect
	@param titleblock_width
	@param file_path
	@param color
	@param dxf_transform : see TitleBlockTemplate::renderDxf()
*/
void TitleBlockTemplateRenderer::renderDxf(QRectF &title_block_rect,
					   int titleblock_width,
					   QString &file_path,
					   int color,
					   const QTransform &dxf_transform) {
	if (!m_titleblock_template) return;
	m_titleblock_template -> renderDxf(title_block_rect,
					   shownContext(),
					   titleblock_width,
					   file_path, color,
					   dxf_transform);
}


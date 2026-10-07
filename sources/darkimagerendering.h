// SPDX-License-Identifier: GPL-2.0-or-later
#ifndef DARK_IMAGE_RENDERING_H
#define DARK_IMAGE_RENDERING_H

#include <QPainter>
#include <QPixmap>
#include <functional>

namespace DarkImageRendering {
// View-local raster composition boundary. Qt still paints the scene in
// its native order, with its own transforms, clipping and opacity.
// A different painter (printing, export, another view or an item cache)
// cannot consume this display-only context.
struct Context {
	QPainter *painter;
	bool invertScene;
	std::function<void(bool, const QRect &)> flush;
};
inline thread_local Context *current = nullptr;
class Scope {
	Context *previous;
public:
	explicit Scope(Context *context) : previous(current) { current = context; }
	~Scope() { current = previous; }
	Scope(const Scope &) = delete;
	Scope &operator=(const Scope &) = delete;
};
inline void paintPixmap(QPainter *painter, const QPixmap &pixmap, bool adapt)
{
	Context *context = current;
	const bool boundary = context && context->painter == painter && adapt != context->invertScene;
	if (boundary) context->flush(context->invertScene, QRect());
	painter->drawPixmap(pixmap.rect(), pixmap);
	if (boundary) {
		// Only the raster was drawn since the preceding flush. Avoid a
		// whole-viewport copy for a small logo; include filtering/AA edges.
		const QRect area = painter->worldTransform().mapRect(QRectF(pixmap.rect())).toAlignedRect().adjusted(-2,-2,2,2);
		context->flush(adapt, area);
	}
}
}
#endif

#ifndef BORDERINSET_H
	#define BORDERINSET_H
	#include <QObject>
	#include <QRectF>
	#include <QPainter>
	#include <QDate>
	/**
		Cette classe represente l'ensemble bordure + cartouche qui encadre le
		schema electrique.
	*/
	class BorderInset : public QObject {
		Q_OBJECT
		public:
		BorderInset(QObject * = 0);
		~BorderInset();
		void draw(QPainter *, qreal = 0.0, qreal = 0.0);
		
		// methodes d'acces en lecture aux dimensions
		int     nbColumn()            const { return(nb_columns);                    }
		qreal   columnsWidth()        const { return(columns_width);                 }
		qreal   columnsHeaderHeight() const { return(columns_header_height);         }
		qreal   columnsHeight()       const { return(columns_height);                }
		qreal   borderWidth()         const { return(nb_columns * columns_width);    }
		qreal   borderHeight()        const { return(columns_height + inset_height); }
		qreal   insetWidth()          const { return(inset_width);                   }
		qreal   insetHeight()         const { return(inset_height);                  }
		
		// methodes d'acces en lecture aux informations du cartouche
		QString author()              const { return(bi_author);             }
		QDate   date()                const { return(bi_date);               }
		QString title()               const { return(bi_title);              }
		QString folio()               const { return(bi_folio);              }
		QString fileName()            const { return(bi_filename);           }
		
		// methodes d'acces en lecture aux options
		bool    insetIsDisplayed()    const { return(display_inset);         }
		bool    columnsAreDisplayed() const { return(display_columns);       }
		bool    borderIsDisplayed()   const { return(display_border);        }
		
		// methodes d'acces en ecriture aux dimensions
		void addColumn             ();
		void removeColumn          ();
		void setColumnsWidth       (const qreal &);
		void setColumnsHeaderHeight(const qreal &);
		void setColumnsHeight      (const qreal &);
		void setInsetWidth         (const qreal &);
		void setInsetHeight        (const qreal &);
		
		// methodes d'acces en ecriture aux informations du cartouche
		void setAuthor             (const QString &author)   { bi_author       = author;   }
		void setDate               (const QDate   &date)     { bi_date         = date;     }
		void setTitle              (const QString &title)    { bi_title        = title;    }
		void setFolio              (const QString &folio)    { bi_folio        = folio;    }
		void setFileName           (const QString &filename) { bi_filename     = filename; }
		
		// methodes d'acces en ecriture aux options
		void displayInset          (bool di)                 { display_inset   = di;       }
		void displayColumns        (bool dc)                 { display_columns = dc;       }
		void displayBorder         (bool db)                 { display_border  = db;       }
		
		private:
		// informations du cartouche
		QString bi_author;
		QDate   bi_date;
		QString bi_title;
		QString bi_folio;       // vraiment necessaire ce truc ?
		QString bi_filename;    // meme remarque
		
		// dimensions du cadre et du cartouche
		int    nb_columns;
		qreal  columns_width;
		qreal  columns_header_height;
		qreal  columns_height;
		qreal  inset_width;
		qreal  inset_height;
		
		// rectangles utilises pour le dessin
		QRectF  border;
		QRectF  inset;
		QRectF  inset_author;
		QRectF  inset_date;
		QRectF  inset_title;
		QRectF  inset_file;
		QRectF  inset_folio;
		
		// booleens pour les options de dessin
		bool display_inset;
		bool display_columns;
		bool display_border;
		
		void updateRectangles();
	};
#endif

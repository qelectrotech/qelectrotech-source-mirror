/*
	Copyright 2006-2008 Xavier Guerrin
	This file is part of QElectroTech.
	
	QElectroTech is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation, either version 2 of the License, or
	(at your option) any later version.
	
	QElectroTech is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.
	
	You should have received a copy of the GNU General Public License
	along with QElectroTech.  If not, see <http://www.gnu.org/licenses/>.
*/
#ifndef BORDERINSET_H
#define BORDERINSET_H
#include "insetproperties.h"
#include <QObject>
#include <QRectF>
#include <QDate>
class QPainter;
/**
	Cette classe represente l'ensemble bordure + cartouche qui encadre le
	schema electrique.
*/
class BorderInset : public QObject {
	Q_OBJECT
	
	// constructeurs, destructeur
	public:
	BorderInset(QObject * = 0);
	virtual ~BorderInset();
	
	private:
	BorderInset(const BorderInset &);
	
	// methodes
	public:
	void draw(QPainter *, qreal = 0.0, qreal = 0.0);
	
	// methodes d'acces en lecture aux dimensions
	/// @return le nombre de colonnes du schema
	int     nbColumn()            const { return(nb_columns);                    }
	/// @return la taille des colonnes en pixels
	qreal   columnsWidth()        const { return(columns_width);                 }
	/// @return la hauteur, en pixels, des en-tetes des colonnes
	qreal   columnsHeaderHeight() const { return(columns_header_height);         }
	/// @return la hauteur des colonnes, en-tetes inclus
	qreal   columnsHeight()       const { return(columns_height);                }
	/// @return la largeur de la bordure
	qreal   borderWidth()         const { return(nb_columns * columns_width);    }
	/// @return la hauteur de la bordure
	qreal   borderHeight()        const { return(columns_height + inset_height); }
	/// @return la largeur du cartouche
	qreal   insetWidth()          const { return(inset_width);                   }
	/// @return la hauteur du cartouche
	qreal   insetHeight()         const { return(inset_height);                  }
	/// @return la hauteur minimale acceptee des colonnes
	qreal   minColumnsHeight()    const { return(min_columns_height);            }
	/// @return le nombre minimum accepte de colonnes
	int     minNbColumns()        const { return(min_nb_columns);                }
	
	// methodes d'acces en lecture aux informations du cartouche
	/// @return le champ "Auteur" du cartouche
	QString author()              const { return(bi_author);             }
	/// @return le champ "Date" du cartouche
	QDate   date()                const { return(bi_date);               }
	/// @return le champ "Titre" du cartouche
	QString title()               const { return(bi_title);              }
	/// @return le champ "Folio" du cartouche
	QString folio()               const { return(bi_folio);              }
	/// @return le champ "Fichier" du cartouche
	QString fileName()            const { return(bi_filename);           }
	
	// methodes d'acces en lecture aux options
	/// @return true si le cartouche est affiche, false sinon
	bool    insetIsDisplayed()    const { return(display_inset);         }
	/// @return true si les entetes des colonnes sont affiches, false sinon
	bool    columnsAreDisplayed() const { return(display_columns);       }
	/// @return true si la bordure est affichee, false sinon
	bool    borderIsDisplayed()   const { return(display_border);        }
	
	// methodes d'acces en ecriture aux dimensions
	void addColumn             ();
	void removeColumn          ();
	void setNbColumns          (int);
	void setColumnsWidth       (const qreal &);
	void setColumnsHeaderHeight(const qreal &);
	void setColumnsHeight      (const qreal &);
	void setInsetWidth         (const qreal &);
	void setInsetHeight        (const qreal &);
	void adjustInsetToColumns  ();
	
	// methodes d'acces en ecriture aux informations du cartouche
	/// @param author le nouveau contenu du champ "Auteur"
	void setAuthor             (const QString &author)   { bi_author       = author;   }
	/// @param date le nouveau contenu du champ "Date"
	void setDate               (const QDate   &date)     { bi_date         = date;     }
	/// @param title le nouveau contenu du champ "Titre"
	void setTitle              (const QString &title)    { bi_title        = title;    }
	/// @param folio le nouveau contenu du champ "Folio"
	void setFolio              (const QString &folio)    { bi_folio        = folio;    }
	/// @param filename le nouveau contenu du champ "Fichier"
	void setFileName           (const QString &filename) { bi_filename     = filename; }
	
	/// @return les proprietes du cartouches
	InsetProperties exportInset() {
		InsetProperties ip;
		ip.author = bi_author;
		ip.date = bi_date;
		ip.title = bi_title;
		ip.folio = bi_folio;
		ip.filename = bi_filename;
		return(ip);
	}
	
	/// @param ip les nouvelles proprietes du cartouche
	void importInset(const InsetProperties &ip) {
		bi_author = ip.author;
		bi_date = ip.date;
		bi_title = ip.title;
		bi_folio = ip.folio;
		bi_filename = ip.filename;
	}
	
	// methodes d'acces en ecriture aux options
	/// @param di true pour afficher le cartouche, false sinon
	void displayInset          (bool di)                 { display_inset   = di;       }
	/// @param dc true pour afficher les entetes des colonnes, false sinon
	void displayColumns        (bool dc)                 { display_columns = dc;       }
	/// @param db true pour afficher la bordure du schema, false sinon
	void displayBorder         (bool db)                 { display_border  = db;       }
	
	private:
	void updateRectangles();
	
	// signaux
	signals:
	/**
		Signal emis lorsque la bordure change
		@param old_border Ancienne bordure
		@param new_border Nouvelle bordure
	*/
	void borderChanged(QRectF old_border, QRectF new_border);
	
	// attributs
	private:
	// informations du cartouche
	QString bi_author;
	QDate   bi_date;
	QString bi_title;
	QString bi_folio;
	QString bi_filename;
	
	// dimensions du cadre et du cartouche
	int    nb_columns;
	int    min_nb_columns;
	qreal  columns_width;
	qreal  columns_header_height;
	qreal  columns_height;
	qreal  min_columns_height;
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
};
#endif

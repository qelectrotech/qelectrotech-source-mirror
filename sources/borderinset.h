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
#include "borderproperties.h"
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
	static int   minNbColumns();
	static qreal minColumnsWidth();
	static int   minNbRows();
	static qreal minRowsHeight();
	
	void draw(QPainter *, qreal = 0.0, qreal = 0.0);
	
	// methodes d'acces en lecture aux dimensions
	// colonnes
	/// @return le nombre de colonnes du schema
	int   nbColumns()           const { return(nb_columns);                    }
	/// @return la largeur des colonnes en pixels
	qreal columnsWidth()        const { return(columns_width);                 }
	/// @return la taille de l'ensemble des colonnes, en-tete des lignes non inclus
	qreal columnsTotalWidth()   const { return(nb_columns * columns_width);    }
	/// @return la hauteur, en pixels, des en-tetes des colonnes
	qreal columnsHeaderHeight() const { return(columns_header_height);         }
	
	// lignes
	/// @return le nombre de lignes du schema
	int   nbRows()              const { return(nb_rows);                       }
	/// @return la hauteur des lignes en pixels
	qreal rowsHeight()          const { return(rows_height);                   }
	/// @return la taille de l'ensemble des lignes, en-tete des colonnes non inclus
	qreal rowsTotalHeight()     const { return(nb_rows * rows_height);         }
	/// @return la largeur, en pixels, des en-tetes des lignes
	qreal rowsHeaderWidth()     const { return(rows_header_width);             }
	
	// cadre sans le cartouche = schema
	/// @return la largeur du schema, c'est-a-dire du cadre sans le cartouche
	qreal diagramWidth()        const { return(columnsTotalWidth() + rowsHeaderWidth());    }
	/// @return la hauteurdu schema, c'est-a-dire du cadre sans le cartouche
	qreal diagramHeight()       const { return(rowsTotalHeight() + columnsHeaderHeight());  }
	
	// cartouche
	/// @return la largeur du cartouche
	qreal   insetWidth()          const { return(inset_width);                 }
	/// @return la hauteur du cartouche
	qreal   insetHeight()         const { return(inset_height);                }
	
	// cadre avec le cartouche
	/// @return la hauteur de la bordure
	qreal   borderWidth()        const { return(diagramWidth());                  }
	/// @return la hauteur de la bordure
	qreal   borderHeight()       const { return(diagramHeight() + insetHeight()); }
	
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
	/// @return true si les entetes des lignes sont affiches, false sinon
	bool    rowsAreDisplayed()    const { return(display_rows);          }
	/// @return true si la bordure est affichee, false sinon
	bool    borderIsDisplayed()   const { return(display_border);        }
	
	// methodes d'acces en ecriture aux dimensions
	void addColumn             ();
	void addRow                ();
	void removeColumn          ();
	void removeRow             ();
	void setNbColumns          (int);
	void setNbRows             (int);
	void setColumnsWidth       (const qreal &);
	void setRowsHeight         (const qreal &);
	void setColumnsHeaderHeight(const qreal &);
	void setRowsHeaderWidth    (const qreal &);
	void setDiagramHeight      (const qreal &);
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
	
	InsetProperties exportInset();
	void importInset(const InsetProperties &);
	BorderProperties exportBorder();
	void importBorder(const BorderProperties &);
	
	// methodes d'acces en ecriture aux options
	/// @param di true pour afficher le cartouche, false sinon
	void displayInset          (bool di)                 { display_inset   = di;       }
	/// @param dc true pour afficher les entetes des colonnes, false sinon
	void displayColumns        (bool dc)                 { display_columns = dc;       }
	/// @param dr true pour afficher les entetes des lignes, false sinon
	void displayRows           (bool dr)                 { display_rows    = dr;       }
	/// @param db true pour afficher la bordure du schema, false sinon
	void displayBorder         (bool db)                 { display_border  = db;       }
	
	private:
	void updateRectangles();
	QString incrementLetters(const QString &);
	
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
	
	// dimensions du cadre (lignes et colonnes)
	// colonnes : nombres et dimensions
	int    nb_columns;
	qreal  columns_width;
	qreal  columns_header_height;
	
	// lignes : nombres et dimensions
	int    nb_rows;
	qreal  rows_height;
	qreal  rows_header_width;
	
	// dimensions du cartouche
	qreal  inset_width;
	qreal  inset_height;
	
	// rectangles utilises pour le dessin
	QRectF  diagram;
	QRectF  inset;
	QRectF  inset_author;
	QRectF  inset_date;
	QRectF  inset_title;
	QRectF  inset_file;
	QRectF  inset_folio;
	
	// booleens pour les options de dessin
	bool display_inset;
	bool display_columns;
	bool display_rows;
	bool display_border;
};
#endif

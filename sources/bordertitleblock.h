/*
	Copyright 2006-2012 Xavier Guerrin
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
#ifndef BORDERTITLEBLOCK_H
#define BORDERTITLEBLOCK_H
#include "diagramcontext.h"
#include "titleblockproperties.h"
#include "borderproperties.h"
#include <QObject>
#include <QRectF>
#include <QDate>
class QPainter;
class DiagramPosition;
class TitleBlockTemplate;
class TitleBlockTemplateRenderer;
/**
	Cette classe represente l'ensemble bordure + cartouche qui encadre le
	schema electrique.
*/
class BorderTitleBlock : public QObject {
	Q_OBJECT
	
	// constructeurs, destructeur
	public:
	BorderTitleBlock(QObject * = 0);
	virtual ~BorderTitleBlock();
	
	private:
	BorderTitleBlock(const BorderTitleBlock &);
	
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
	qreal   titleBlockWidth()          const { return(titleblock_width);                 }
	qreal   titleBlockHeight()         const;
	
	// cadre avec le cartouche
	/// @return la hauteur de la bordure
	qreal   borderWidth()        const { return(diagramWidth());                  }
	/// @return la hauteur de la bordure
	qreal   borderHeight()       const { return(diagramHeight() + titleBlockHeight()); }
	
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
	bool    titleBlockIsDisplayed()    const { return(display_titleblock);         }
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
	void setTitleBlockWidth         (const qreal &);
	void adjustTitleBlockToColumns  ();
	
	DiagramPosition convertPosition(const QPointF &);
	
	// methodes d'acces en ecriture aux informations du cartouche
	/// @param author le nouveau contenu du champ "Auteur"
	void setAuthor             (const QString &author)   { bi_author       = author;   }
	/// @param date le nouveau contenu du champ "Date"
	void setDate               (const QDate   &date)     { bi_date         = date;     }
	/// @param title le nouveau contenu du champ "Titre"
	void setTitle              (const QString &title) {
		if (bi_title != title) {
			bi_title = title;
			emit(diagramTitleChanged(title));
		}
	}
	/// @param folio le nouveau contenu du champ "Folio"
	void setFolio              (const QString &folio)    { bi_folio        = folio;    }
	void setFolioData(int, int);
	/// @param filename le nouveau contenu du champ "Fichier"
	void setFileName           (const QString &filename) { bi_filename     = filename; }
	
	void titleBlockToXml(QDomElement &);
	void titleBlockFromXml(const QDomElement &);
	void borderToXml(QDomElement &);
	void borderFromXml(const QDomElement &);
	
	TitleBlockProperties exportTitleBlock();
	void importTitleBlock(const TitleBlockProperties &);
	BorderProperties exportBorder();
	void importBorder(const BorderProperties &);
	
	const TitleBlockTemplate *titleBlockTemplate();
	void setTitleBlockTemplate(const TitleBlockTemplate *);
	QString titleBlockTemplateName() const;
	
	public slots:
	void titleBlockTemplateChanged(const QString &);
	void titleBlockTemplateRemoved(const QString &, const TitleBlockTemplate * = 0);
	
	// methodes d'acces en ecriture aux options
	void displayTitleBlock(bool);
	void displayColumns(bool);
	void displayRows(bool);
	void displayBorder(bool);
	
	private:
	void updateRectangles();
	void updateDiagramContextForTitleBlock();
	QString incrementLetters(const QString &);
	
	// signaux
	signals:
	/**
		Signal emis lorsque la bordure change
		@param old_border Ancienne bordure
		@param new_border Nouvelle bordure
	*/
	void borderChanged(QRectF old_border, QRectF new_border);
	/**
		Signal emise lorsque des options d'affichage change
	*/
	void displayChanged();
	
	/**
		Signal emis lorsque le titre du schema change
	*/
	void diagramTitleChanged(const QString &);
	
	/**
		Signal emis lorsque le cartouche requiert une mise a jour des donnees
		utilisees pour generer le folio.
	*/
	void needFolioData();
	
	/**
		Signal emitted when this object needs to set a specific title block
		template. This object cannot handle the job since it does not know of
		its parent project.
	*/
	void needTitleBlockTemplate(const QString &);
	
	// attributs
	private:
	// informations du cartouche
	QString bi_author;
	QDate   bi_date;
	QString bi_title;
	QString bi_folio;
	QString bi_final_folio;
	int folio_index_;
	int folio_total_;
	QString bi_filename;
	DiagramContext additional_fields_;
	
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
	qreal  titleblock_width;
	qreal  titleblock_height;
	
	// rectangles utilises pour le dessin
	QRectF  diagram;
	QRectF  titleblock;
	
	// booleens pour les options de dessin
	bool display_titleblock;
	bool display_columns;
	bool display_rows;
	bool display_border;
	TitleBlockTemplateRenderer *titleblock_template_renderer;
};
#endif

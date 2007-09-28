#ifndef INSET_PROPERTIES_H
#define INSET_PROPERTIES_H
#include <QDate>
#include <QString>
class InsetProperties {
	public:
	InsetProperties() {
	}
	virtual ~InsetProperties() {
	}
	bool operator==(const InsetProperties &ip) {
		return(
			ip.title == title &&\
			ip.author == author &&\
			ip.date == date &&\
			ip.filename == filename &&\
			ip.folio == folio
		);
	}
	bool operator!=(const InsetProperties &ip) {
		return(!(*this == ip));
	}
	// attributs
	QString title;
	QString author;
	QDate date;
	QString filename;
	QString folio;
};
#endif

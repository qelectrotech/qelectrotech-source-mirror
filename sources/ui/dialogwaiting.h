/*
		Copyright 2006-2016 The QElectroTech Team
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

#ifndef DIALOGWAITING_H
#define DIALOGWAITING_H

#include <QDialog>

namespace Ui {
	class DialogWaiting;
}

class DialogWaiting : public QDialog
{
	Q_OBJECT

public:
	explicit DialogWaiting(QWidget *parent = 0);
	~DialogWaiting();

	void setProgressBar(int val);
	void setProgressBarRange(int min, int max);
	void setProgressReset();
	void setTitle(const QString& val);
	void setDetail(const QString& val);

	private:
	Ui::DialogWaiting *ui;
};

#endif // DIALOGWAITING_H

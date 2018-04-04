/*
		Copyright 2006-2017 The QElectroTech Team
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
#include <QMutex>

namespace Ui {
	class DialogWaiting;
}

class DialogWaiting : public QDialog
{
	Q_OBJECT
	public:
		static DialogWaiting* instance(QWidget *parent = nullptr)
		{
			static QMutex mutex;
			if(!m_static_dialog)
			{
				mutex.lock();
				if(!m_static_dialog)
					m_static_dialog = new DialogWaiting(parent);
				mutex.unlock();
			}
			return m_static_dialog;
		}
		
		static bool hasInstance()
		{
			if(m_static_dialog == nullptr)
				return false;
			else
				return true;
		}
		
		static void dropInstance()
		{
			static QMutex mutex;
			if(m_static_dialog)
			{
				mutex.lock();
				m_static_dialog->deleteLater();
				m_static_dialog = nullptr;
				mutex.unlock();
			}
		}
	private:
		static DialogWaiting *m_static_dialog;
	

	public:
		explicit DialogWaiting(QWidget *parent = nullptr);
		~DialogWaiting() override;
	
		void setProgressBar(int val);
		void setProgressBarRange(int min, int max);
		void setProgressReset();
		void setTitle(const QString& val);
		void setDetail(const QString& val);
		int progressBarValue() const;
	
	private:
		Ui::DialogWaiting *ui;
};

#endif // DIALOGWAITING_H

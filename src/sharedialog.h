/*  smplayer, GUI front-end for mplayer.
    Copyright (C) 2006-2025 Ricardo Villalba <ricardo@smplayer.info>

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/

#ifndef SHAREDIALOG_H
#define SHAREDIALOG_H

#include "ui_sharedialog.h"

class ShareDialog : public QDialog, public Ui::ShareDialog
{
	Q_OBJECT

public:
	ShareDialog( QWidget* parent = 0, Qt::WindowFlags f = QFlag(0) );
	~ShareDialog();

	bool isRemindChecked();

	void setActions(int a) { actions_taken = a; }
	int actions() { return actions_taken; }

	void showRemindCheck(bool b);

private slots:
	void on_donate_button_clicked();
	void on_facebook_button_clicked();
	void on_twitter_button_clicked();

private:
	int actions_taken;
	QString share_url;
};

#endif

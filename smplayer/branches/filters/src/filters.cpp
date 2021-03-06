/*  smplayer, GUI front-end for mplayer.
    Copyright (C) 2006-2012 Ricardo Villalba <rvm@users.sourceforge.net>

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

#include "filters.h"
#include <QSettings>
#include <QWidget>
#include <QAction>
#include <QActionGroup>

Filters::Filters(QObject * parent) : QObject(parent) 
{
	init();
}

void Filters::init() {
	list.clear();

	// Video
	list["noise"] = Filter(tr("add noise"), "noise", "9ah:5ah");
	list["deblock"] = Filter(tr("deblock"), "pp", "vb/hb");
	list["gradfun"] = Filter(tr("gradfun"), "gradfun");
	list["denoise_off"] = Filter(tr("denoise off"), "", "", "denoise");
	list["denoise_normal"] = Filter(tr("normal denoise"), "hqdn3d", "", "denoise");
	list["denoise_soft"] = Filter(tr("soft denoise"), "hqdn3d", "2:1:2", "denoise");
	list["blur"] = Filter(tr("blur"), "unsharp", "lc:-1.5");
	list["sharpen"] = Filter(tr("sharpen"), "unsharp", "lc:1.5");

	// Audio
	list["volnorm"] = Filter(tr("volume normalization"), "volnorm", "1");
}

Filter * Filters::item(const QString & key) {
	return &list[key];
}

QList<QAction *> Filters::createActions(QWidget * parent) {
	actions.clear();

	QMap<QString, Filter>::iterator i;
	for (i = list.begin(); i != list.end(); ++i) {
		//set->setValue(i.key(), i.value().options());
		QAction * a = new QAction( i.value().trName(), parent );
		a->setObjectName(i.key());
		a->setCheckable(true);

		actions.push_back(a);

		QString group_name = i.value().group();
		if (!group_name.isEmpty()) {
			QActionGroup *ag = groups[group_name];
			if (ag == 0) {
				ag = new QActionGroup(parent);
				//ag->setExclusive(false);
				groups[group_name] = ag;
			}
			ag->addAction(a);
		}
	}

	return actions;
}

QList <QActionGroup*> Filters::actionGroups() {
	QList <QActionGroup*> g;

	QMap<QString, QActionGroup *>::iterator i;
	for (i = groups.begin(); i != groups.end(); ++i) {
		g.push_back(i.value());
	}
	return g;
}

QString Filters::filtersToString() {
	QString s;
	QMap<QString, Filter>::iterator i;
	for (i = list.begin(); i != list.end(); ++i) {
		if (i.value().enabled()) {
			if (!s.isEmpty()) s += ",";
			s += i.value().filter();
		}
	}
	return s;
}

void Filters::setEnabled(const QString & filter_name, bool enable) {
	list[filter_name].setEnabled(enable);
	QString group = list[filter_name].group();

	// Disable filters in the same group
	if (!group.isEmpty()) {
		QMap<QString, Filter>::iterator i;
		for (i = list.begin(); i != list.end(); ++i) {
			if ((i.key() != filter_name) && (i.value().group() == group)) i.value().setEnabled(false);
		}
	}
}

void Filters::save(QSettings *set) {
	set->beginGroup("filter_options");

	QMap<QString, Filter>::iterator i;
	for (i = list.begin(); i != list.end(); ++i) {
		set->setValue(i.key(), i.value().options());
	}

	set->endGroup();
}

void Filters::load(QSettings *set) {
	set->beginGroup("filter_options");

	QMap<QString, Filter>::iterator i;
	for (i = list.begin(); i != list.end(); ++i) {
		i.value().setOptions( set->value(i.key(), i.value().options()).toString() );
	}

	set->endGroup();
}

#include "moc_filters.cpp"

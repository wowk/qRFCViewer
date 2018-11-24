/****************************************************************************

    qRFCView, A smart IETF RFC viewer based on the Qt4 library.
    Copyright (C) 2005 Mitsubishi Electric ITE-TCL, R. Rollet (rollet@tcl.ite.mee.com)

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
    Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
*********************************************************************************/

#include <QFile>
#include <QIcon>
#include <QApplication>
#include "mainwindow.h"

int main(int argc, char *argv[]) {
    Q_INIT_RESOURCE(rfcview);
    QFont font("Courier");
    font.setStyleHint(QFont::Monospace);
    QApplication app(argc, argv);
    app.setAttribute(Qt::AA_UseHighDpiPixmaps);
    app.setWindowIcon(QIcon(":/images/rfcview.png"));
    app.setFont(font);
    MainWindow mainWin;
    mainWin.show();
    return app.exec();
}

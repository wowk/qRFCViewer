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

#include <QtGui>
#include <QToolTip>
#include "qrfceditor.h"

QRFCEditor::QRFCEditor(QWidget *parent)
    : QTextBrowser(parent) {
    m_iCurrentPositionIdx=0;
    m_translator = new QTranslator(this);
    connect(this, SIGNAL(translate(QString)), m_translator, SLOT(translate(QString)));
    connect(m_translator, SIGNAL(finish(QString)), this, SLOT(translateFinished(QString)));
    connect(m_translator, SIGNAL(error(QString)), this, SLOT(translateError(QString)));
}


QRFCEditor::~QRFCEditor() {
}


void QRFCEditor::setSource ( const QUrl & name ) {
    QRegExp qRegExpRFC("rfc([\\d]+).txt");

    if (name.toString()[0]=='#') {
        //qDebug() << "setSource: " + name.toString().mid(1,-1);
        scrollToAnchor2(name.toString().mid(1,-1) );
    } else if ( qRegExpRFC.indexIn(name.toString())!=-1) {
        //qDebug() << "RFC " + qRegExpRFC.cap(1) + " is required";
        emit RFCReq( qRegExpRFC.cap(1).toUInt() );
    }
}

void QRFCEditor::scrollToAnchor2 ( const QString & name ) {
    // Record the link
    //QTextCursor qTextCursor=textCursor();
    QTextCursor qTextCursor=cursorForPosition(QPoint(0,0) );
    //qDebug() << qTextCursor.position() ;
    while (m_iCurrentPositionIdx != m_qPositionPath.size() )
        // Remove the end of the list
        m_qPositionPath.removeLast();

    // Save position at the end of the list
    m_qPositionPath.append(qTextCursor.position() );
    m_iCurrentPositionIdx++;

    QTextEdit::scrollToAnchor(name);
    emit backwardAvailable(true);
    emit forwardAvailable(false);
}

void QRFCEditor::backward () {
    QTextCursor qTextCursor=cursorForPosition(QPoint(0,0) );
    //uint32_t iPosition;

    if (m_iCurrentPositionIdx>0) {
        //qDebug() << qTextCursor.position() ;
        if ( m_iCurrentPositionIdx == m_qPositionPath.size() )
            m_qPositionPath.append(qTextCursor.position() );
        m_iCurrentPositionIdx--;
        qTextCursor.movePosition( QTextCursor::End);
        setTextCursor(qTextCursor);
        qTextCursor.setPosition( m_qPositionPath[m_iCurrentPositionIdx] );
        setTextCursor(qTextCursor);
        ensureCursorVisible ();

        if (m_iCurrentPositionIdx==0)
            emit backwardAvailable(false);
        emit forwardAvailable(true);
    }

}

void QRFCEditor::forward () {
    QTextCursor qTextCursor=textCursor();
    if (m_iCurrentPositionIdx<m_qPositionPath.size()-1) {
        m_iCurrentPositionIdx++;
        qTextCursor.movePosition( QTextCursor::End);
        setTextCursor(qTextCursor);
        qTextCursor.setPosition( m_qPositionPath[m_iCurrentPositionIdx] );
        setTextCursor(qTextCursor);
        ensureCursorVisible ();
        if (m_iCurrentPositionIdx==m_qPositionPath.size()-1)
            emit forwardAvailable(false);
        emit backwardAvailable(true);
    }
}

void QRFCEditor::translateFinished(QString s) {
    QFontMetrics fontMetrics(this->font());
    QToolTip::showText(this->cursor().pos(), s, this, fontMetrics.tightBoundingRect(s), 2000);
}

void QRFCEditor::translateError(QString s) {

}

bool QRFCEditor::isBackwardAvailable () {
    return (m_iCurrentPositionIdx>0);
}

bool QRFCEditor::isForwardAvailable () {
    return (m_iCurrentPositionIdx<m_qPositionPath.size()-1);
}

void QRFCEditor::mousePressEvent(QMouseEvent *ev) {
    QTextBrowser::mousePressEvent(ev);
    QToolTip::hideText();
}

void QRFCEditor::mouseReleaseEvent(QMouseEvent *e) {
    QTextBrowser::mouseReleaseEvent(e);
    QTextCursor cur =  this->textCursor();
    cur.select(QTextCursor::SelectionType::WordUnderCursor);
    setTextCursor(cur);

    QString text = cur.selectedText();
    text = text.trimmed();
    QRegExp regexp(".*[a-z]{2,}.*");
    if( !regexp.exactMatch(text) ) {
        return;
    }

    emit translate(text);
}

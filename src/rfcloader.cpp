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

#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QEventLoop>
#include <QUrl>
#include <QFile>
#include "rfcloader.h"
#include <QMessageBox>
#include <QtDebug>
#include <QDir>
#include <QDesktopServices>
#include <QStandardPaths>

QRFCLoader::QRFCLoader(QObject *parent)
    : QObject(parent) {
    m_qNetworkAccessManager = new QNetworkAccessManager(this);
    connect(m_qNetworkAccessManager, SIGNAL( finished( QNetworkReply* ) ), this, SLOT( onFinished( QNetworkReply* )) );
    m_qDir = QStandardPaths::writableLocation(QStandardPaths::GenericDataLocation) + "/qRFCView";
    QDir dir = QDir(m_qDir);
    dir.mkpath(m_qDir);
    QUrl url = QUrl(QString("https://www.ietf.org/rfc/"));
    SetDownloadURL(url);
    m_iCurrentRequestID=-1;
}


QRFCLoader::~QRFCLoader() {
    //delete m_qHttp;
    delete m_qNetworkAccessManager;
}

void QRFCLoader::SetDownloadURL(QUrl &qURL) {
    m_qIETFSite=qURL.host();
    m_qIETFPath=qURL.path();
}

void QRFCLoader::GetFile(uint32_t iRFCNum) {
    QString qFilename;
    qFilename = m_qDir + "/rfc" + QString::number(iRFCNum) +".txt";
    QFile qFile(qFilename);
    qFile.open(QFile::OpenModeFlag::ReadOnly);
    if ( qFile.isOpen() && qFile.size() > 0 ) {
        qFile.close();
        emit done(qFilename);
        return;
    }

    // RFC is not yet loaded
    // Open a file in the default dir.
    RFCDesc_t sRFCDesc;
    sRFCDesc.pFile = new QFile(qFilename);
    sRFCDesc.iRFCNum=iRFCNum;

    if (!sRFCDesc.pFile->open(QIODevice::WriteOnly)) {
        delete sRFCDesc.pFile;
        QMessageBox::information(nullptr, tr("RFCView"),
                                 tr("Unable to write RFC %1")
                                 .arg(iRFCNum));

        return;
    }
    QString qUrl= QString("https://") + m_qIETFSite + m_qIETFPath + QString("rfc%1.txt").arg(iRFCNum, 4, 10, QChar('0'));


    qDebug() << qUrl;

    QNetworkRequest request(qUrl);
    QEventLoop event;

    m_iCurrentRequestID++;

    emit start( qFilename );

    m_RequestList.insert(m_iCurrentRequestID, sRFCDesc);
    m_qReply = m_qNetworkAccessManager->get(request);
    connect(m_qReply, SIGNAL(downloadProgress(qint64,qint64)), this, SLOT(onDownloading(qint64, qint64)) );
    connect(m_qReply, SIGNAL(finished()), &event, SLOT(quit()));
    event.exec();
}

void QRFCLoader::startDownload(int iRequestID) {
    //qDebug() << "startDownload="+QString::number(iRequestID);
    if ( m_RequestList.contains(iRequestID))
        m_iCurrentRequestID=iRequestID;
    else
        m_iCurrentRequestID=-1;
}

void QRFCLoader::onDownloading(qint64 bytesRead, qint64 totalBytes) {
    emit(downloadUpdate(bytesRead, totalBytes));
}

void QRFCLoader::onFinished(QNetworkReply *reply) {
    fileDownload(m_iCurrentRequestID, reply);
}

void QRFCLoader::fileDownload(int iRequestID, QNetworkReply *reply) {
    QString qFilename;
    RFCDesc_t sRFCDesc;
    //qDebug() << "fileDownload="+QString::number(iRequestID)+","+QString::number(m_iCurrentRequestID);
    QVariant status_code = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute);
    uint32_t statusCode = 0;
    if (!status_code.isValid()) {
        return;
    } else {
        statusCode = status_code.toUInt();
    }
    if ( m_RequestList.contains(iRequestID) ) {
        sRFCDesc=m_RequestList.value(iRequestID);
        if (m_iCurrentRequestID==iRequestID) {
            qFilename = m_qDir + "/rfc" + QString::number(sRFCDesc.iRFCNum) +".txt";
            sRFCDesc.pFile->write(reply->readAll());
            sRFCDesc.pFile->close();
            delete sRFCDesc.pFile;

            if (statusCode == 200) {
                qDebug() << "File downloaded: " << qFilename;
                emit done( qFilename );
            } else
                QMessageBox::information(nullptr, tr("RFCView"),
                                         tr("Unable to load RFC %1: ")
                                         .arg(sRFCDesc.iRFCNum) +  QString(reply->error()) ) ;

            m_RequestList.remove(iRequestID);
            m_iCurrentRequestID--;
        } else {
            sRFCDesc.pFile->remove();
        }
    }
}

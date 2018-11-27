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
#include <QFontDialog>
#include <QNetworkAccessManager>
#include <QFileDialog>
#include <QStatusBar>
#include <QInputDialog>
#include <QMenu>
#include <QMenuBar>
#include <QMessageBox>
#include <QToolBar>
#include <QProgressBar>
#include <QPrinter>
#include <QLibrary>
#include <QDir>
#include <QDesktopServices>

#include <limits>

#include "rfcloader.h"
#include "mainwindow.h"
#include "mdichild.h"
#include "cdialogsetdirectory.h"
#include "cdialogfind.h"
#include "qrfceditor.h"
#include "cprintdialog.h"
#include "translator.h"
#include "translatordialog.h"


MainWindow::MainWindow() {
    /* OSX style */
    setUnifiedTitleAndToolBarOnMac(true);

    m_qTabWidget = new QTabWidget();
    m_qTabWidget->setDocumentMode(true);
    m_qTabWidget->setTabsClosable(true);
    m_qTabWidget->setMovable(true);

    setCentralWidget(m_qTabWidget);

    connect(m_qTabWidget, SIGNAL(currentChanged(int)), this, SLOT(updateMenus()));
    connect(m_qTabWidget, SIGNAL(tabCloseRequested(int)), this, SLOT(close_tab(int)));

    /*
    windowMapper = new QSignalMapper(this);
    connect(windowMapper, SIGNAL(mapped(QWidget *)),
    workspace, SLOT(setActiveWindow(QWidget *)));
    */
    m_translatorSettingDlg = new TranslatorDialog(this);
    m_translator        = new Translator(this);
    m_translatorCache   = new TranslatorCache(this);

    readSettings();

    m_translatorCache->loadCache(m_translatorSettingDlg->cacheFile());
    m_translator->updateTranslatorSlot(
        m_translatorSettingDlg->translatorSite(),
        m_translatorSettingDlg->sourceLanguage(),
        m_translatorSettingDlg->targetLanguage());
    connect(m_translatorSettingDlg, SIGNAL(updateTranslatorSig(QString, QString, QString)),
            m_translator, SLOT(updateTranslatorSlot(QString, QString, QString)));

    m_pRFCLoader = new QRFCLoader(this);
    connect(m_pRFCLoader, SIGNAL(done(QString)),this, SLOT(RFCReady(QString)));
    connect(m_pRFCLoader, SIGNAL(start(QString)),this, SLOT(RFCStart(QString)));

    open_dialog = new QFileDialog(this, Qt::Sheet);
    //open_dialog->setDirectory(QDesktopServices::storageLocation(QDesktopServices::DataLocation) + "qRFCView");
    open_dialog->setDirectory(m_pRFCLoader->GetDir());
    open_dialog->setFileMode(QFileDialog::ExistingFile);
    open_dialog->setNameFilter("RFC documents (*.txt)");
    open_dialog->setWindowModality(Qt::WindowModal);
    connect(open_dialog, SIGNAL(finished(int)), this, SLOT(open_dialog_finished(int)));

    createActions();
    createMenus();
    createToolBars();
    createStatusBar();
    updateMenus();
    //m_pDialogFind=NULL;
    m_pDialogFind=new CDialogFind(this);
    m_qFont = QFont("Courier", 13);
    m_qFont.setStyleHint(QFont::Monospace);
    connect(m_pDialogFind, SIGNAL(findnext()), this, SLOT(findnext()));
    setWindowTitle(tr("qRFCView"));
    //m_pRFCLoader->GetFile(794);
}

void MainWindow::closeEvent(QCloseEvent *event) {
    //workspace->closeAllWindows();
    /*
    if (activeMdiChild()) {
        event->ignore();
    } else {
        writeSettings();
        event->accept();
    }*/
    writeSettings();
    event->accept();
}

void MainWindow::open_dialog_finished(int result) {
    if (result != QDialog::Accepted)
        return;

    QString fileName = open_dialog->selectedFiles().value(0);
    QFileInfo qFileInfo(fileName);

    if (fileName.isEmpty())
        return;

    MdiChild *existing = findMdiChild(fileName);
    if (existing) {
        m_qTabWidget->setCurrentWidget(existing);
        return;
    }

    MdiChild *child = createMdiChild(qFileInfo.fileName());
    if (child->loadFile(fileName)) {
        child->setCurrentFont(m_qFont);
        statusBar()->showMessage(tr("File loaded"), 2000);
        child->show();
    } else {
        child->close();
    }

    this->activateWindow();
    this->raise();

}

void MainWindow::close_tab(int index) {
    MdiChild *pMdiChild = qobject_cast<MdiChild *>(m_qTabWidget->widget(index));
    if (pMdiChild) {
        m_qTabWidget->removeTab(index);
        delete pMdiChild;
    }

    updateMenus();
}

void MainWindow::translatorSetting() {
    m_translatorSettingDlg->exec();
}

void MainWindow::getrfc() {
    // Load a RFC number
    bool bOK;
    int iRFCNum = QInputDialog::getInt(this, tr("Please enter a RFC number"),
                                       tr("RFC#:"), 0, 1, std::numeric_limits<int>::max(), 1, &bOK);
    if (bOK)
        RFCLoad( iRFCNum );
}

void MainWindow::findOpen() {
    m_pDialogFind->show();
    m_pDialogFind->raise();
    m_pDialogFind->activateWindow();
}

void MainWindow::findnext() {
    bool bResult;
    uint32_t iOptionFlags;

    QString qTextToFind=m_pDialogFind->GetTextToFind();
    iOptionFlags=m_pDialogFind->GetOptionFlags();
    MdiChild *pMdiChild=activeMdiChild();
    if (pMdiChild && !qTextToFind.isEmpty() ) {
        bResult=pMdiChild->FindText(qTextToFind, iOptionFlags );
        if (!bResult)
            statusBar()->showMessage(tr("No more occurence"), 3000);
        else
            statusBar()->clearMessage();
        iOptionFlags|=FIND_OPTIONSFLAG_CURSOR;
        m_pDialogFind->SetOptionFlags(iOptionFlags);
    }
    updateMenus();
}

void MainWindow::findprev() {
    bool bResult;
    uint32_t iOptionFlags;

    QString qTextToFind=m_pDialogFind->GetTextToFind();
    iOptionFlags=m_pDialogFind->GetOptionFlags();
    MdiChild *pMdiChild=activeMdiChild();
    if (pMdiChild && !qTextToFind.isEmpty() ) {
        iOptionFlags|=FIND_OPTIONSFLAG_CURSOR|FIND_OPTIONSFLAG_BACKWARD;
        bResult=pMdiChild->FindText(qTextToFind, iOptionFlags );
        if (!bResult)
            statusBar()->showMessage(tr("No more occurence"), 3000);
        else
            statusBar()->clearMessage();
    }
}

void MainWindow::forward() {
    MdiChild *pMdiChild=activeMdiChild();
    if (pMdiChild)
        pMdiChild->m_pTextEdit->forward();
}


void MainWindow::backward() {
    MdiChild *pMdiChild=activeMdiChild();
    if (pMdiChild)
        pMdiChild->m_pTextEdit->backward();
}

void MainWindow::setFont() {
    bool bOK;
    MdiChild *pMDIChild;
    QFont qFont=QFontDialog::getFont(&bOK, QFont("Courier", 13), this );
    m_qFont=qFont;
    if (bOK) {
        for (int i=0; i<m_qTabWidget->count(); i++) {
            qDebug() << "Thien here " << i << endl;
            pMDIChild = qobject_cast<MdiChild *>(m_qTabWidget->widget(i));
            pMDIChild->setCurrentFont(m_qFont);
        }
    }
}

void MainWindow::print() {
    MdiChild *pMdiChild=activeMdiChild();
    if (pMdiChild) {
        QPrinter printer(QPrinter::PrinterResolution);

        printer.setFullPage(true);
        printer.setNumCopies(2);
        QPrintDialog dlg(&printer, this);
        dlg.setPrintRange(QAbstractPrintDialog::PageRange);
        dlg.setMinMax(1, pMdiChild->GetNbPages() );
        dlg.setFromTo(1, 2);
        dlg.setEnabledOptions(QAbstractPrintDialog::PrintPageRange);

        if (dlg.exec() == QDialog::Accepted) {
            pMdiChild->Print(&printer, dlg.printRange()==QAbstractPrintDialog::AllPages, dlg.fromPage(), dlg.toPage() );
            pMdiChild->Print(&printer, false, 4,6);
        }

    }
}

void MainWindow::about() {
    QString displayText = QString("<b>qRFCView</b><br> A smart RFC Viewer using the Qt5 library.<br><br> Mitsubishi Electric, 2005")
                          + QString("<br><div>Icons made by <a href=\"https://www.flaticon.com/authors/popcorns-arts\" title=\"Icon Pond\">Icon Pond</a> from <a href=\"https://www.flaticon.com/\" title=\"Flaticon\">www.flaticon.com</a> ")
                          + QString("is licensed by <a href=\"http://creativecommons.org/licenses/by/3.0/\" title=\"Creative Commons BY 3.0\" target=\"_blank\">CC 3.0 BY</a></div>");
    QMessageBox::about(this, tr("About qRFCView"),
                       tr(displayText.toStdString().c_str()));
}

void MainWindow::updateMenus() {
    /*
        bool hasMdiChild = (activeMdiChild() != 0);

        closeAct->setEnabled(hasMdiChild);
        closeAllAct->setEnabled(hasMdiChild);
        tileAct->setEnabled(hasMdiChild);
        cascadeAct->setEnabled(hasMdiChild);
        nextAct->setEnabled(hasMdiChild);
        previousAct->setEnabled(hasMdiChild);
        separatorAct->setVisible(hasMdiChild);
    */
    findAct->setEnabled( activeMdiChild()!=NULL);
    printAct->setEnabled( activeMdiChild()!=NULL);
    findnextAct->setEnabled( activeMdiChild()!=NULL && (!m_pDialogFind->GetTextToFind().isEmpty()) );
    findprevAct->setEnabled( activeMdiChild()!=NULL && (!m_pDialogFind->GetTextToFind().isEmpty()) );
    forwardAct->setEnabled( activeMdiChild()!=NULL && activeMdiChild()->m_pTextEdit->isForwardAvailable() );
    backwardAct->setEnabled( activeMdiChild()!=NULL && activeMdiChild()->m_pTextEdit->isBackwardAvailable() );
}

void MainWindow::updateWindowMenu() {
    /*
    windowMenu->clear();
    windowMenu->addAction(closeAct);
    windowMenu->addAction(closeAllAct);
    windowMenu->addSeparator();
    windowMenu->addAction(tileAct);
    windowMenu->addAction(cascadeAct);
    windowMenu->addSeparator();
    windowMenu->addAction(nextAct);
    windowMenu->addAction(previousAct);
    windowMenu->addAction(separatorAct);

    QList<QWidget *> windows = workspace->windowList();
    separatorAct->setVisible(!windows.isEmpty());

    for (int i = 0; i < windows.size(); ++i) {
        MdiChild *child = qobject_cast<MdiChild *>(windows.at(i));

        QString text;
        if (i < 9) {
            text = tr("&%1. %2").arg(i + 1)
                                .arg(child->userFriendlyCurrentFile());
        } else {
            text = tr("%1. %2").arg(i + 1)
                                .arg(child->userFriendlyCurrentFile());
        }
        QAction *action  = windowMenu->addAction(text);
        action->setCheckable(true);
        action ->setChecked(child == activeMdiChild());
        connect(action, SIGNAL(triggered()), windowMapper, SLOT(map()));
        windowMapper->setMapping(action, child);
    }*/
}

MdiChild *MainWindow::createMdiChild(const QString &qTitle) {
    MdiChild *child = new MdiChild();
    m_qTabWidget->addTab(child, qTitle);

    connect(child->m_pTextEdit, SIGNAL(RFCReq( uint32_t )), this, SLOT(RFCLoad( uint32_t  )) );
    connect(child->m_pTextEdit, SIGNAL(forwardAvailable(bool)), forwardAct, SLOT(setEnabled(bool)) );
    connect(child->m_pTextEdit, SIGNAL(backwardAvailable(bool)), backwardAct, SLOT(setEnabled(bool)) );
    connect(child->m_pTextEdit, SIGNAL(translateSig(QString, QString)),
            m_translator, SLOT(translateSlot(QString, QString)));
    connect(child->m_pTextEdit, SIGNAL(findCacheSig(QString, QString)),
            m_translatorCache, SLOT(findCacheSlot(QString, QString)));
    connect(child->m_pTextEdit, SIGNAL(addCacheSig(QString, QString)),
            m_translatorCache, SLOT(addCacheSlot(QString, QString)));
    connect(m_translator, SIGNAL(translateFinishSig(QString, QString)),
            child->m_pTextEdit, SLOT(translateFinishedSlot(QString, QString)));
    connect(m_translator, SIGNAL(translateErrorSig(QString, QString)),
            child->m_pTextEdit, SLOT(translateErrorSlot(QString, QString)));
    connect(m_translatorCache, SIGNAL(foundCacheSig(QString, QString)),
            child->m_pTextEdit, SLOT(foundCacheSlot(QString, QString)));
    connect(m_translatorCache, SIGNAL(foundNoCacheSig(QString, QString)),
            child->m_pTextEdit, SLOT(foundNoCacheSlot(QString, QString)));

    m_qTabWidget->setCurrentWidget(child);
    return child;
}

void MainWindow::createActions() {
    openAct = new QAction(QIcon(":/images/open.png"), tr("&Open..."), this);
    openAct->setShortcut(tr("Ctrl+O"));
    openAct->setStatusTip(tr("Open an existing file"));
    connect(openAct, SIGNAL(triggered()), open_dialog, SLOT(open()));

    loadAct = new QAction(QIcon(":/images/load.png"), tr("&Load RFC..."), this);
    loadAct->setShortcut(tr("Ctrl+L"));
    loadAct->setStatusTip(tr("Load a RFC from its number"));
    connect(loadAct, SIGNAL(triggered()), this, SLOT(getrfc()));

    printAct = new QAction(QIcon(":/images/print.png"), tr("&Print..."), this);
    connect(printAct, SIGNAL(triggered()), this, SLOT(print()));

    exitAct = new QAction(tr("E&xit"), this);
    exitAct->setShortcut(tr("Ctrl+Q"));
    exitAct->setStatusTip(tr("Exit the application"));
    connect(exitAct, SIGNAL(triggered()), qApp, SLOT(closeAllWindows()));

    setFontAct = new QAction(QIcon(), tr("&Set Font..."), this);
    connect(setFontAct, SIGNAL(triggered()), this, SLOT(setFont()));

    forwardAct = new QAction(QIcon(":/images/forward.png"), tr("&Forward"), this);
    connect(forwardAct, SIGNAL(triggered()), this, SLOT(forward()));
    backwardAct = new QAction(QIcon(":/images/backward.png"), tr("&Backward"), this);
    connect(backwardAct, SIGNAL(triggered()),this, SLOT(backward()));
    findAct = new QAction(QIcon(":/images/find.png"), tr("&Find..."), this);
    findAct->setShortcut(tr("Ctrl+F"));
    findAct->setStatusTip(tr("Find a string in the currently opened file"));
    connect(findAct, SIGNAL(triggered()), this, SLOT(findOpen()));
    findnextAct = new QAction(QIcon(), tr("&Find Next"), this);
    findnextAct->setShortcut(tr("F3"));
    connect(findnextAct, SIGNAL(triggered()), this, SLOT(findnext()));
    findprevAct = new QAction(QIcon(), tr("&Find Previous"), this);
    findprevAct->setShortcut(tr("Shift+F3"));
    connect(findprevAct, SIGNAL(triggered()), this, SLOT(findprev()));

    /*


    closeAllAct = new QAction(tr("Close &All"), this);
    closeAllAct->setStatusTip(tr("Close all the windows"));
    connect(closeAllAct, SIGNAL(triggered()),
            workspace, SLOT(closeAllWindows()));

    tileAct = new QAction(tr("&Tile"), this);
    tileAct->setStatusTip(tr("Tile the windows"));
    connect(tileAct, SIGNAL(triggered()), workspace, SLOT(tile()));

    cascadeAct = new QAction(tr("&Cascade"), this);
    cascadeAct->setStatusTip(tr("Cascade the windows"));
    connect(cascadeAct, SIGNAL(triggered()), workspace, SLOT(cascade()));

    nextAct = new QAction(tr("Ne&xt"), this);
    nextAct->setShortcut(tr("Ctrl+F6"));
    nextAct->setStatusTip(tr("Move the focus to the next window"));
    connect(nextAct, SIGNAL(triggered()),
            workspace, SLOT(activateNextWindow()));

    previousAct = new QAction(tr("Pre&vious"), this);
    previousAct->setShortcut(tr("Ctrl+Shift+F6"));
    previousAct->setStatusTip(tr("Move the focus to the previous "
                                  "window"));
    connect(previousAct, SIGNAL(triggered()),
            workspace, SLOT(activatePreviousWindow()));

    separatorAct = new QAction(this);
    separatorAct->setSeparator(true);
    */

    setTranslatorAct = new QAction(tr("Translator"), this);
    connect(setTranslatorAct, SIGNAL(triggered()), this, SLOT(translatorSetting()));

    aboutAct = new QAction(tr("&About"), this);
    aboutAct->setStatusTip(tr("Show the application's About box"));
    connect(aboutAct, SIGNAL(triggered()), this, SLOT(about()));

    aboutQtAct = new QAction(tr("About &Qt"), this);
    aboutQtAct->setStatusTip(tr("Show the Qt library's About box"));
    connect(aboutQtAct, SIGNAL(triggered()), qApp, SLOT(aboutQt()));
}

void MainWindow::createMenus() {
    fileMenu = menuBar()->addMenu(tr("&File"));
    fileMenu->addAction(openAct);
    fileMenu->addAction(loadAct);
    fileMenu->addAction(printAct);
    fileMenu->addSeparator();
    fileMenu->addAction(exitAct);

    editMenu = menuBar()->addMenu(tr("&Edit"));
    editMenu->addAction(backwardAct);
    editMenu->addAction(forwardAct);
    editMenu->addAction(findAct);
    editMenu->addAction(findprevAct);
    editMenu->addAction(findnextAct);
    editMenu->addSeparator();
    editMenu->addAction(setFontAct);
    editMenu->addAction(setTranslatorAct);

    //windowMenu = menuBar()->addMenu(tr("&Window"));
    //connect(windowMenu, SIGNAL(aboutToShow()), this, SLOT(updateWindowMenu()));

    //menuBar()->addSeparator();

    helpMenu = menuBar()->addMenu(tr("&Help"));
    helpMenu->addAction(aboutAct);
    helpMenu->addAction(aboutQtAct);
}

void MainWindow::createToolBars() {
    fileToolBar = addToolBar(tr("ToolBar"));
    fileToolBar->setToolButtonStyle(Qt::ToolButtonTextUnderIcon);
    fileToolBar->addAction(openAct);
    fileToolBar->addAction(loadAct);
    fileToolBar->addAction(printAct);
    fileToolBar->addAction(findAct);
    fileToolBar->addAction(backwardAct);
    fileToolBar->addAction(forwardAct);
}

void MainWindow::createStatusBar() {
    m_pProgressBar=new QProgressBar();
    m_pProgressBar->setRange(0,100);
    m_pProgressBar->setValue(50);
    connect(m_pRFCLoader, SIGNAL(downloadUpdate(qint64,qint64)), this, SLOT( updateRFCProgress(qint64,qint64)));
    statusBar()->showMessage(tr("Ready"));
    statusBar()->addWidget(m_pProgressBar, 0);
    //m_pProgressBar->show();
}

void MainWindow::readSettings() {
    QSettings settings("qRFCView", "qRFCView");
    QPoint pos = settings.value("pos", QPoint(200, 200)).toPoint();
    QSize size = settings.value("size", QSize(640, 480)).toSize();
    m_qFont.setFamily(settings.value("Font_family", m_qFont.family()).toString() );
    m_qFont.setPointSize(settings.value("Font_size",   m_qFont.pointSize()).toInt() );
    m_qFont.setWeight(settings.value("Font_weight", m_qFont.weight()).toInt());
    m_qFont.setItalic(settings.value("Font_italic", m_qFont.italic()).toBool());
    m_translatorSettingDlg->setTranslatorSite(settings.value("TranslatorSite", "Yandex").toString());
    m_translatorSettingDlg->setSourceLanguage(settings.value("TranslatorSource", "en").toString());
    m_translatorSettingDlg->setTargetLanguage(settings.value("TranslatorTarget", "zh").toString());
    m_translatorSettingDlg->setCacheFile(settings.value("TranslatorCacheFile", "qRFCTranslatorCache").toString());

    move(pos);
    resize(size);
}

void MainWindow::writeSettings() {
    QSettings settings("qRFCView", "qRFCView");
    settings.setValue("pos", pos());
    settings.setValue("size", size());
    settings.setValue("Font_family", m_qFont.family());
    settings.setValue("Font_size",   m_qFont.pointSize());
    settings.setValue("Font_weight", m_qFont.weight());
    settings.setValue("Font_italic", m_qFont.italic());
    settings.setValue("TranslatorSite", m_translatorSettingDlg->translatorSite());
    settings.setValue("TranslatorSource", m_translatorSettingDlg->sourceLanguage());
    settings.setValue("TranslatorTarget", m_translatorSettingDlg->targetLanguage());
    settings.setValue("TranslatorCacheFile", m_translatorSettingDlg->cacheFile());
}

MdiChild *MainWindow::activeMdiChild() {

    return qobject_cast<MdiChild *>(m_qTabWidget->currentWidget());
}

MdiChild *MainWindow::findMdiChild(const QString &fileName) {
    QString canonicalFilePath = QFileInfo(fileName).canonicalFilePath();

    for (int i=0; i<m_qTabWidget->count(); i++) {
        MdiChild *mdiChild = qobject_cast<MdiChild *>(m_qTabWidget->widget(i));
        if (mdiChild->currentFile() == canonicalFilePath)
            return mdiChild;
    }
    return 0;
}

void MainWindow::RFCLoad( uint32_t iRFCNum ) {
    QString qFilename=QString("rfc%1.txt").arg(iRFCNum);
    QFileInfo qFileInfo;

    // Check if the required RFC is not yet opened
    for (int i=0; i<m_qTabWidget->count(); i++) {
        MdiChild *mdiChild = qobject_cast<MdiChild *>(m_qTabWidget->widget(i));
        qFileInfo.setFile(mdiChild->currentFile());
        if (qFileInfo.fileName() == qFilename) {
            m_qTabWidget->setCurrentWidget(mdiChild);
            return;
        }
    }

    m_pRFCLoader->GetFile(iRFCNum);
}

void MainWindow::RFCStart(const QString &qFilename) {
    // Start downloading a RFC file
    Q_UNUSED(qFilename);
    statusBar()->showMessage(tr("Downloading ")+qFilename);
    statusBar()->clearMessage();
    statusBar()->addWidget(m_pProgressBar, true);
    m_pProgressBar->show();
}

void MainWindow::RFCReady(const QString &qFilename) {
    QFileInfo qFileInfo(qFilename);
    MdiChild *child = createMdiChild(qFileInfo.fileName());
    if (child->loadFile(qFilename)) {
        child->setCurrentFont(m_qFont);
        child->show();
    } else {
        child->close();
    }
    statusBar()->showMessage(tr("Load ")+qFilename, 2000);
    statusBar()->removeWidget(m_pProgressBar);
}

void MainWindow::updateRFCProgress(qint64 bytesRead, qint64 totalBytes) {
    m_pProgressBar->setMaximum(totalBytes);
    m_pProgressBar->setValue(bytesRead);
}

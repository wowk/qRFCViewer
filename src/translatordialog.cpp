#include "translatordialog.h"
#include "ui_translatordialog.h"
#include <QDebug>
#include <QFileDialog>


TranslatorDialog::TranslatorDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::TranslatorDialog) {
    ui->setupUi(this);
    setTranslatorSite("Yandex");
    setSourceLanguage("en");
    setTargetLanguage("zh");
}

TranslatorDialog::~TranslatorDialog() {
    delete ui;
}

const QString TranslatorDialog::translatorSite() {
    return site;
}

const QString TranslatorDialog::sourceLanguage() {
    return source;
}

const QString TranslatorDialog::targetLanguage() {
    return target;
}

const QString TranslatorDialog::cacheFile()
{
    return m_cacheFile;
}

void TranslatorDialog::setTranslatorSite(const QString &siteName) {
    if( siteName.indexOf("Baidu", 0, Qt::CaseInsensitive) >= 0 ) {
        ui->comboBoxSite->setCurrentIndex(0);
        site = "Baidu";
    } else {
        ui->comboBoxSite->setCurrentIndex(1);
        site = "Yandex";
    }
}

void TranslatorDialog::setSourceLanguage(const QString &sourceLang) {
    ui->comboBoxSourceLang->setCurrentIndex(0);
    source = sourceLang;
}

void TranslatorDialog::setTargetLanguage(const QString &targetLang) {
    ui->comboBoxTargetLang->setCurrentIndex(0);
    target = targetLang;
}

void TranslatorDialog::setCacheFile(const QString &cacheFile)
{
    m_cacheFile = cacheFile;
    ui->cacheFileLineEdit->setText(cacheFile);
}

void TranslatorDialog::showEvent(QShowEvent *ev) {
    qDebug() << "Show Event";
    setTranslatorSite(site);
    setSourceLanguage(source);
    setTargetLanguage(target);

    QDialog::showEvent(ev);
}

void TranslatorDialog::on_cancelButton_clicked() {
    this->close();
}

void TranslatorDialog::on_applyButton_clicked() {
    this->setTranslatorSite(ui->comboBoxSite->currentText());
    this->setSourceLanguage(ui->comboBoxSourceLang->currentText());
    this->setTargetLanguage(ui->comboBoxTargetLang->currentText());
    emit updateTranslatorSig(site, source, target);
    this->setCacheFile(ui->cacheFileLineEdit->text());
    this->close();
}

void TranslatorDialog::on_selectCacheFileBtn_clicked()
{
    QFileDialog fileDlg(this, tr("Choose Cache File"), ".");
    fileDlg.setFileMode(QFileDialog::FileMode::AnyFile);
    connect(&fileDlg, SIGNAL(fileSelected(QString)), ui->cacheFileLineEdit, SLOT(setText(QString)));
    fileDlg.exec();
}

#ifndef TRANSLATORDIALOG_H
#define TRANSLATORDIALOG_H

#include <QDialog>

namespace Ui {
class TranslatorDialog;
}

class TranslatorDialog : public QDialog
{
    Q_OBJECT

signals:
    void updateTranslatorSig(QString, QString, QString);

public:
    explicit TranslatorDialog(QWidget *parent = 0);
    ~TranslatorDialog();

    const QString translatorSite(void);
    const QString sourceLanguage(void);
    const QString targetLanguage(void);

    void setTranslatorSite(const QString & siteName);
    void setSourceLanguage(const QString & sourceLanguage);
    void setTargetLanguage(const QString & targetLanguage);

    void showEvent(QShowEvent* ev);

private slots:

    void on_cancelButton_clicked();
    void on_applyButton_clicked();

private:
    Ui::TranslatorDialog *ui;

    QString site;
    QString source;
    QString target;
};

#endif // TRANSLATORDIALOG_H

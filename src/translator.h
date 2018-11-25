#ifndef TRANSLATOR_H__
#define TRANSLATOR_H__

#include <QString>
#include <QNetworkAccessManager>

class TranslatorAPI;

class Translator : public QObject {
    Q_OBJECT
  public:
    explicit Translator(QObject* parent);

  public slots:
    void translate(QString s);
    void reply(QNetworkReply *reply);
    void updateTranslatorSlot(QString, QString, QString);

  signals:
    void finish(QString  s);
    void error(QString s);

  private:
    QNetworkAccessManager* netAccessMgr;

  private:
    TranslatorAPI* translatorAPI;
};

#endif

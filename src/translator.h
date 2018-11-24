#ifndef TRANSLATOR_H__
#define TRANSLATOR_H__

#include <QString>
#include <QNetworkAccessManager>


class QTranslator : public QObject {
    Q_OBJECT
  public:
    explicit QTranslator(QObject* parent);

  public slots:
    void translate(QString s);
    void reply(QNetworkReply *reply);

  signals:
    void finish(QString  s);
    void error(QString s);

  private:
    QNetworkAccessManager* netAccessMgr;

  private:
    static const QString appid;
    static const QString key;
};

#endif

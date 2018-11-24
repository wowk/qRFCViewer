#include <translator.h>
#include <QNetworkRequest>
#include <QCryptographicHash>
#include <QNetworkReply>
#include <QJsonDocument>


const QString QTranslator::appid = "20181124000238616";
const QString QTranslator::key   = "BWIrrd3ARnhIVX7IJEjk";

QTranslator::QTranslator(QObject* parent) : QObject(parent) {
    netAccessMgr = new QNetworkAccessManager(this);
    connect(netAccessMgr, SIGNAL(finished(QNetworkReply*)), this, SLOT(reply(QNetworkReply*)));
}

void QTranslator::translate(QString qword) {
    QString salt = QString::asprintf("%u", qrand() % 100000000U);
    QString strSign = appid + qword + salt + key;
    QByteArray bytes(strSign.toStdString().data(), strSign.size());
    QString sign = QString().fromStdString(QCryptographicHash::hash(bytes, QCryptographicHash::Algorithm::Md5).toHex().toStdString());
    QString query = "q=" + qword + "&from=auto&to=zh&appid=" + appid + "&salt=" + salt + "&sign=" + sign;
    QUrl url("http://fanyi-api.baidu.com/api/trans/vip/translate?" + query);
    QNetworkRequest translateRequest(url);
    netAccessMgr->get(translateRequest);
}

void QTranslator::reply(QNetworkReply *reply) {
    if(reply->attribute(QNetworkRequest::HttpStatusCodeAttribute) != 200 ) {
        qDebug() << "Got Error";
        return;
    }

    QByteArray data = reply->readAll();
    QJsonDocument doc = QJsonDocument::fromJson(data);
    emit finish(doc["trans_result"][0]["dst"].toString());
}

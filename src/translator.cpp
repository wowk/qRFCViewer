#include <translator.h>
#include <QNetworkRequest>
#include <QCryptographicHash>
#include <QNetworkReply>
#include <QJsonDocument>


class TranslatorAPI {
public:
    TranslatorAPI(const QString & s, const QString & t):
        from(s), to(t)
    {}

    virtual QNetworkRequest translateRequest(const QString & qword) = 0;
    virtual QString translateResult(const QByteArray & data) = 0;
    virtual ~TranslatorAPI() {}

protected:
    QString from;
    QString to;
};

class BaiduTranslatorAPI : public TranslatorAPI {
public:
    BaiduTranslatorAPI(const QString & s, const QString & t) :
        TranslatorAPI(s, t),
        appid("20181124000238616"),
        key("BWIrrd3ARnhIVX7IJEjk")
    {}

    QNetworkRequest translateRequest(const QString & qword){
        QString queryStr(qword.toUtf8().data());
        QString salt = QString::asprintf("%u", qrand() % 100000000U);
        QString strSign = appid + queryStr + salt + key;
        QByteArray bytes(strSign.toStdString().data(), strSign.size());
        QString sign = QString().fromStdString(QCryptographicHash::hash(bytes, QCryptographicHash::Algorithm::Md5).toHex().toStdString());
        QString query = "&from=" + from + "&to=" + to + "&appid=" + appid + "&salt=" + salt + "&sign=" + sign + "&q=" + queryStr;
        QUrl url("http://fanyi-api.baidu.com/api/trans/vip/translate?" + query);
        return QNetworkRequest(url);
    }

    QString translateResult(const QByteArray & data){
        QJsonDocument doc = QJsonDocument::fromJson(data);
        //qDebug() << "Baidu: " << doc;
        return doc["trans_result"][0]["dst"].toString();
    }

private:
    const QString appid;
    const QString key;
};

class YandexTranslatorAPI : public TranslatorAPI {
public:
    YandexTranslatorAPI(const QString & s, const QString & t) :
        TranslatorAPI(s, t),
        key("trnsl.1.1.20181124T160248Z.ee4557c60a06fbac.05af9931631627667f6d431932bb4de2a4ad8e8a")
    {}

    QNetworkRequest translateRequest(const QString & qword){
        QString urlStr = "https://translate.yandex.net/api/v1.5/tr.json/translate?";
        urlStr = urlStr + "lang=" + from + "-" + to;
        urlStr = urlStr + "&key=" + key;
        urlStr = urlStr + "&text=" + qword.toUtf8().data();
        return QNetworkRequest(QUrl(QUrl(urlStr).toEncoded().data()));
    }

    QString translateResult(const QByteArray & data){
        QJsonDocument doc = QJsonDocument::fromJson(data);
        //qDebug() << "Yandex: " << doc["text"];
        return doc["text"][0].toString();
    }

private:
    const QString key;
};

Translator::Translator(QObject* parent) : QObject(parent) {
    translatorAPI = nullptr;
    netAccessMgr = new QNetworkAccessManager(this);
    connect(netAccessMgr, SIGNAL(finished(QNetworkReply*)), this, SLOT(reply(QNetworkReply*)));
}

void Translator::translate(QString qword) {
    if( !translatorAPI ){
        translatorAPI = new BaiduTranslatorAPI("en", "zh");
    }

    netAccessMgr->get(translatorAPI->translateRequest(qword));
}

void Translator::reply(QNetworkReply *reply) {
    if(reply->attribute(QNetworkRequest::HttpStatusCodeAttribute) != 200 ) {
        qDebug() << "Got Error";
        qDebug() << reply->readAll().toStdString().data();
        return;
    }
    emit finish(translatorAPI->translateResult(reply->readAll()));
}

void Translator::updateTranslatorSlot(QString site, QString source, QString target)
{
    if( translatorAPI ){
        delete translatorAPI;
    }
    qDebug() << "Update Translator API" << endl;
    if( site.indexOf("Baidu", 0, Qt::CaseInsensitive) >= 0 ){
        translatorAPI = new BaiduTranslatorAPI(source, target);
    }else{
        translatorAPI = new YandexTranslatorAPI(source, target);
    }
}

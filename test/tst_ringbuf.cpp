#include <QtTest/QtTest>
#include "../ringbuf.h"
#include <qvector.h>

static QByteArray randomBytes(QRandomGenerator &random, const int maxLen)
{
    int len = random.bounded(maxLen);
    QByteArray ret(len, 0);
    for (int i = 0; i < len; ++i)
        ret[i] = char(random.bounded(32, 126));
    return ret;
}

class tst_RingBuf : public QObject
{
    Q_OBJECT
private slots:
    void randomChars();
    void randomCharsInterleavingTake();
};

void tst_RingBuf::randomChars()
{
#define SIZE 255
    RingBuf<char, SIZE> rb;
    QByteArray control;
    QRandomGenerator random;
    QCOMPARE(rb.count(), 0);
    QCOMPARE(rb.available(), SIZE);
    for (int i = 0; i < 1000000; ++i) {
        int countWas = rb.count();
        QByteArray rand = randomBytes(random, SIZE / 2);
        for (int j = 0; j < rand.count(); ++j)
            rb.insert(rand[j]);
        control.append(rand);
        QByteArray rbCurrentBytes;
        for (uint16_t j = 0; j < rb.count(); ++j)
            rbCurrentBytes.append(rb.at(j));
        if (control.size() > rb.count())
            control.remove(0, control.size() - rb.count());
        //qDebug() << i << ":" << "we had" << countWas << "; after inserting" << rand.count() << "we have" << rb.count() << ", available" << rb.available();
        //qDebug() << rand;
        QCOMPARE(rb.count(), qMin(SIZE, countWas + rand.length()));
        QCOMPARE(rb.count(), control.count());
        if (rbCurrentBytes != control) {
            qWarning() << "actually have" << rbCurrentBytes;
            qWarning() << "should have  " << control;
        }
        QCOMPARE(rbCurrentBytes, control);
        uint8_t toTake = uint8_t(qMin(SIZE / 4, int(rb.count())));
        bool takeItAll = (toTake == rb.count());
        control.remove(0, toTake);
        uint8_t took = rb.remove(toTake);
        //qDebug() << i << ":" << "took out" << toTake << "; now we have" << rb.count() << ", available" << rb.available();
        QCOMPARE(took, toTake);
        if (takeItAll)
            QCOMPARE(rb.count(), 0);
    }
}

void tst_RingBuf::randomCharsInterleavingTake()
{
#define SIZE 255
    RingBuf<char, SIZE> rb;
    QByteArray control;
    QRandomGenerator random;
    QCOMPARE(rb.count(), 0);
    QCOMPARE(rb.available(), SIZE);
    for (int i = 0; i < 100000; ++i) {
        QByteArray rand = randomBytes(random, SIZE / 2);
        int countWas = rb.count();
        uint8_t toTake = uint8_t(qMin(countWas, SIZE / 4));
        int countWillBe = qMin(SIZE, countWas - toTake + rand.length());
        QByteArray taken;
        int countControl = countWas;
        for (int j = 0; j < rand.count(); ++j) {
            if (taken.length() < toTake)
                taken.append(rb.take());
            else if (countControl < 255)
                ++countControl;
            rb.insert(rand[j]);
            QCOMPARE(rb.count(), countControl);
        }
        for (int j = taken.length(); j < toTake; ++j)
            taken.append(rb.take());
        QCOMPARE(taken.length(), toTake);
        control.append(rand);
        QCOMPARE(taken, control.left(taken.length()));
        control.remove(0, toTake);
        QByteArray rbCurrentBytes;
        for (uint16_t j = 0; j < rb.count(); ++j)
            rbCurrentBytes.append(rb.at(j));
//        qDebug() << i << ":" << "we had" << countWas << "; after inserting" << rand.count()
//                 << "we have" << rb.count() << ", available" << rb.available();
//        qDebug() << rand;
//        qDebug() << "expected rb count would be" << countWas << "-" << toTake << "+" << rand.length() << "=" << countWillBe;
        QCOMPARE(rb.count(), countWillBe);
        QCOMPARE(rbCurrentBytes.count(), rb.count());
        QCOMPARE(rb.count(), qMin(SIZE, control.count()));
        if (control.count() > rb.count())
            control.remove(0, control.size() - rb.count());
        if (rbCurrentBytes != control) {
            qWarning() << "actually have" << rbCurrentBytes;
            qWarning() << "should have  " << control;
        }
        QCOMPARE(rbCurrentBytes, control);
        qDebug() << i << ":" << "took out" << toTake
                 << "; now we have" << rb.count() << ", available" << rb.available();
    }
}

#include "tst_ringbuf.moc"
QTEST_MAIN(tst_RingBuf)

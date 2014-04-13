#include <QQuickItem>
#include <QQuickPaintedItem>
#include <QQuickWindow>

class FooItem : public QQuickPaintedItem {
    Q_OBJECT
public:
    FooItem() = default;
    void paint(QPainter*) override {}
Q_SIGNALS:
    void someSignal();
};

int main() {
    QQuickItem* qq = new QQuickItem();
    qq->connect(qq, SIGNAL(windowChanged(QQuickWindow*)), SLOT(update()));
    qq->connect(qq, SIGNAL(badSignal(QQuickWindow*)), SLOT(update())); // don't convert
    qq->connect(qq, SIGNAL(windowChanged(QQuickWindow*)), SLOT(badSlot())); // don't convert
    qq->connect(qq, SIGNAL(badSignal(QQuickWindow*)), SLOT(badSlot())); // don't convert

    QQuickPaintedItem* qp = new FooItem();
    qp->connect(qp, SIGNAL(fillColorChanged()), SLOT(deleteLater()));
    qp->connect(qp, SIGNAL(badSignal(QQuickWindow*)), SLOT(deleteLater())); // don't convert
    qp->connect(qp, SIGNAL(fillColorChanged()), SLOT(badSlot())); // don't convert
    qp->connect(qp, SIGNAL(badSignal(QQuickWindow*)), SLOT(badSlot())); // don't convert

    FooItem* f = new FooItem();
    f->connect(f, SIGNAL(someSignal()), SLOT(deleteLater()));
    f->connect(f, SIGNAL(badSignal(QQuickWindow*)), SLOT(deleteLater())); // don't convert
    f->connect(f, SIGNAL(someSignal()), SLOT(badSlot())); // don't convert
    f->connect(f, SIGNAL(badSignal(QQuickWindow*)), SLOT(badSlot())); // don't convert
    return 0;
}

#include "qquickitem.moc"

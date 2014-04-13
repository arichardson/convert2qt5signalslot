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
    QObject::connect(qq, &QQuickItem::windowChanged, qq, &QQuickItem::update);
    qq->connect(qq, SIGNAL(badSignal(QQuickWindow*)), SLOT(update())); // don't convert
    qq->connect(qq, SIGNAL(windowChanged(QQuickWindow*)), SLOT(badSlot())); // don't convert
    qq->connect(qq, SIGNAL(badSignal(QQuickWindow*)), SLOT(badSlot())); // don't convert

    QQuickPaintedItem* qp = new FooItem();
    QObject::connect(qp, &QQuickPaintedItem::fillColorChanged, qp, &QQuickPaintedItem::deleteLater);
    qp->connect(qp, SIGNAL(badSignal(QQuickWindow*)), SLOT(deleteLater())); // don't convert
    qp->connect(qp, SIGNAL(fillColorChanged()), SLOT(badSlot())); // don't convert
    qp->connect(qp, SIGNAL(badSignal(QQuickWindow*)), SLOT(badSlot())); // don't convert

    FooItem* f = new FooItem();
    QObject::connect(f, &FooItem::someSignal, f, &FooItem::deleteLater);
    f->connect(f, SIGNAL(badSignal(QQuickWindow*)), SLOT(deleteLater())); // don't convert
    f->connect(f, SIGNAL(someSignal()), SLOT(badSlot())); // don't convert
    f->connect(f, SIGNAL(badSignal(QQuickWindow*)), SLOT(badSlot())); // don't convert
    return 0;
}

#include "qquickitem.moc"

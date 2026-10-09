/****************************************************************************
** Meta object code from reading C++ file 'mainwindow.h'
**
** Created by: The Qt Meta Object Compiler version 67 (Qt 5.12.8)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include "mainwindow.h"
#include <QtCore/qbytearray.h>
#include <QtCore/qmetatype.h>
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'mainwindow.h' doesn't include <QObject>."
#elif Q_MOC_OUTPUT_REVISION != 67
#error "This file was generated using the moc from 5.12.8. It"
#error "cannot be used with the include files from this version of Qt."
#error "(The moc has changed too much.)"
#endif

QT_BEGIN_MOC_NAMESPACE
QT_WARNING_PUSH
QT_WARNING_DISABLE_DEPRECATED
struct qt_meta_stringdata_TrajectoryView_t {
    QByteArrayData data[1];
    char stringdata0[15];
};
#define QT_MOC_LITERAL(idx, ofs, len) \
    Q_STATIC_BYTE_ARRAY_DATA_HEADER_INITIALIZER_WITH_OFFSET(len, \
    qptrdiff(offsetof(qt_meta_stringdata_TrajectoryView_t, stringdata0) + ofs \
        - idx * sizeof(QByteArrayData)) \
    )
static const qt_meta_stringdata_TrajectoryView_t qt_meta_stringdata_TrajectoryView = {
    {
QT_MOC_LITERAL(0, 0, 14) // "TrajectoryView"

    },
    "TrajectoryView"
};
#undef QT_MOC_LITERAL

static const uint qt_meta_data_TrajectoryView[] = {

 // content:
       8,       // revision
       0,       // classname
       0,    0, // classinfo
       0,    0, // methods
       0,    0, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
       0,       // signalCount

       0        // eod
};

void TrajectoryView::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    Q_UNUSED(_o);
    Q_UNUSED(_id);
    Q_UNUSED(_c);
    Q_UNUSED(_a);
}

QT_INIT_METAOBJECT const QMetaObject TrajectoryView::staticMetaObject = { {
    &QWidget::staticMetaObject,
    qt_meta_stringdata_TrajectoryView.data,
    qt_meta_data_TrajectoryView,
    qt_static_metacall,
    nullptr,
    nullptr
} };


const QMetaObject *TrajectoryView::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *TrajectoryView::qt_metacast(const char *_clname)
{
    if (!_clname) return nullptr;
    if (!strcmp(_clname, qt_meta_stringdata_TrajectoryView.stringdata0))
        return static_cast<void*>(this);
    return QWidget::qt_metacast(_clname);
}

int TrajectoryView::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QWidget::qt_metacall(_c, _id, _a);
    return _id;
}
struct qt_meta_stringdata_MainWindow_t {
    QByteArrayData data[23];
    char stringdata0[209];
};
#define QT_MOC_LITERAL(idx, ofs, len) \
    Q_STATIC_BYTE_ARRAY_DATA_HEADER_INITIALIZER_WITH_OFFSET(len, \
    qptrdiff(offsetof(qt_meta_stringdata_MainWindow_t, stringdata0) + ofs \
        - idx * sizeof(QByteArrayData)) \
    )
static const qt_meta_stringdata_MainWindow_t qt_meta_stringdata_MainWindow = {
    {
QT_MOC_LITERAL(0, 0, 10), // "MainWindow"
QT_MOC_LITERAL(1, 11, 14), // "requestConnect"
QT_MOC_LITERAL(2, 26, 0), // ""
QT_MOC_LITERAL(3, 27, 4), // "host"
QT_MOC_LITERAL(4, 32, 4), // "port"
QT_MOC_LITERAL(5, 37, 17), // "requestDisconnect"
QT_MOC_LITERAL(6, 55, 11), // "requestNode"
QT_MOC_LITERAL(7, 67, 4), // "node"
QT_MOC_LITERAL(8, 72, 12), // "requestPreOp"
QT_MOC_LITERAL(9, 85, 12), // "requestStart"
QT_MOC_LITERAL(10, 98, 13), // "requestCmdVel"
QT_MOC_LITERAL(11, 112, 1), // "v"
QT_MOC_LITERAL(12, 114, 1), // "w"
QT_MOC_LITERAL(13, 116, 16), // "onConnectClicked"
QT_MOC_LITERAL(14, 133, 19), // "onDisconnectClicked"
QT_MOC_LITERAL(15, 153, 6), // "onPose"
QT_MOC_LITERAL(16, 160, 1), // "x"
QT_MOC_LITERAL(17, 162, 1), // "y"
QT_MOC_LITERAL(18, 164, 2), // "th"
QT_MOC_LITERAL(19, 167, 11), // "onConnState"
QT_MOC_LITERAL(20, 179, 2), // "up"
QT_MOC_LITERAL(21, 182, 12), // "onCmdChanged"
QT_MOC_LITERAL(22, 195, 13) // "onStopClicked"

    },
    "MainWindow\0requestConnect\0\0host\0port\0"
    "requestDisconnect\0requestNode\0node\0"
    "requestPreOp\0requestStart\0requestCmdVel\0"
    "v\0w\0onConnectClicked\0onDisconnectClicked\0"
    "onPose\0x\0y\0th\0onConnState\0up\0onCmdChanged\0"
    "onStopClicked"
};
#undef QT_MOC_LITERAL

static const uint qt_meta_data_MainWindow[] = {

 // content:
       8,       // revision
       0,       // classname
       0,    0, // classinfo
      12,   14, // methods
       0,    0, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
       6,       // signalCount

 // signals: name, argc, parameters, tag, flags
       1,    2,   74,    2, 0x06 /* Public */,
       5,    0,   79,    2, 0x06 /* Public */,
       6,    1,   80,    2, 0x06 /* Public */,
       8,    0,   83,    2, 0x06 /* Public */,
       9,    0,   84,    2, 0x06 /* Public */,
      10,    2,   85,    2, 0x06 /* Public */,

 // slots: name, argc, parameters, tag, flags
      13,    0,   90,    2, 0x08 /* Private */,
      14,    0,   91,    2, 0x08 /* Private */,
      15,    3,   92,    2, 0x08 /* Private */,
      19,    1,   99,    2, 0x08 /* Private */,
      21,    0,  102,    2, 0x08 /* Private */,
      22,    0,  103,    2, 0x08 /* Private */,

 // signals: parameters
    QMetaType::Void, QMetaType::QString, QMetaType::UShort,    3,    4,
    QMetaType::Void,
    QMetaType::Void, QMetaType::Int,    7,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void, QMetaType::Double, QMetaType::Double,   11,   12,

 // slots: parameters
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void, QMetaType::Double, QMetaType::Double, QMetaType::Double,   16,   17,   18,
    QMetaType::Void, QMetaType::Bool,   20,
    QMetaType::Void,
    QMetaType::Void,

       0        // eod
};

void MainWindow::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    if (_c == QMetaObject::InvokeMetaMethod) {
        auto *_t = static_cast<MainWindow *>(_o);
        Q_UNUSED(_t)
        switch (_id) {
        case 0: _t->requestConnect((*reinterpret_cast< const QString(*)>(_a[1])),(*reinterpret_cast< quint16(*)>(_a[2]))); break;
        case 1: _t->requestDisconnect(); break;
        case 2: _t->requestNode((*reinterpret_cast< int(*)>(_a[1]))); break;
        case 3: _t->requestPreOp(); break;
        case 4: _t->requestStart(); break;
        case 5: _t->requestCmdVel((*reinterpret_cast< double(*)>(_a[1])),(*reinterpret_cast< double(*)>(_a[2]))); break;
        case 6: _t->onConnectClicked(); break;
        case 7: _t->onDisconnectClicked(); break;
        case 8: _t->onPose((*reinterpret_cast< double(*)>(_a[1])),(*reinterpret_cast< double(*)>(_a[2])),(*reinterpret_cast< double(*)>(_a[3]))); break;
        case 9: _t->onConnState((*reinterpret_cast< bool(*)>(_a[1]))); break;
        case 10: _t->onCmdChanged(); break;
        case 11: _t->onStopClicked(); break;
        default: ;
        }
    } else if (_c == QMetaObject::IndexOfMethod) {
        int *result = reinterpret_cast<int *>(_a[0]);
        {
            using _t = void (MainWindow::*)(const QString & , quint16 );
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&MainWindow::requestConnect)) {
                *result = 0;
                return;
            }
        }
        {
            using _t = void (MainWindow::*)();
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&MainWindow::requestDisconnect)) {
                *result = 1;
                return;
            }
        }
        {
            using _t = void (MainWindow::*)(int );
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&MainWindow::requestNode)) {
                *result = 2;
                return;
            }
        }
        {
            using _t = void (MainWindow::*)();
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&MainWindow::requestPreOp)) {
                *result = 3;
                return;
            }
        }
        {
            using _t = void (MainWindow::*)();
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&MainWindow::requestStart)) {
                *result = 4;
                return;
            }
        }
        {
            using _t = void (MainWindow::*)(double , double );
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&MainWindow::requestCmdVel)) {
                *result = 5;
                return;
            }
        }
    }
}

QT_INIT_METAOBJECT const QMetaObject MainWindow::staticMetaObject = { {
    &QWidget::staticMetaObject,
    qt_meta_stringdata_MainWindow.data,
    qt_meta_data_MainWindow,
    qt_static_metacall,
    nullptr,
    nullptr
} };


const QMetaObject *MainWindow::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *MainWindow::qt_metacast(const char *_clname)
{
    if (!_clname) return nullptr;
    if (!strcmp(_clname, qt_meta_stringdata_MainWindow.stringdata0))
        return static_cast<void*>(this);
    return QWidget::qt_metacast(_clname);
}

int MainWindow::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QWidget::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 12)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 12;
    } else if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        if (_id < 12)
            *reinterpret_cast<int*>(_a[0]) = -1;
        _id -= 12;
    }
    return _id;
}

// SIGNAL 0
void MainWindow::requestConnect(const QString & _t1, quint16 _t2)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(&_t1)), const_cast<void*>(reinterpret_cast<const void*>(&_t2)) };
    QMetaObject::activate(this, &staticMetaObject, 0, _a);
}

// SIGNAL 1
void MainWindow::requestDisconnect()
{
    QMetaObject::activate(this, &staticMetaObject, 1, nullptr);
}

// SIGNAL 2
void MainWindow::requestNode(int _t1)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(&_t1)) };
    QMetaObject::activate(this, &staticMetaObject, 2, _a);
}

// SIGNAL 3
void MainWindow::requestPreOp()
{
    QMetaObject::activate(this, &staticMetaObject, 3, nullptr);
}

// SIGNAL 4
void MainWindow::requestStart()
{
    QMetaObject::activate(this, &staticMetaObject, 4, nullptr);
}

// SIGNAL 5
void MainWindow::requestCmdVel(double _t1, double _t2)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(&_t1)), const_cast<void*>(reinterpret_cast<const void*>(&_t2)) };
    QMetaObject::activate(this, &staticMetaObject, 5, _a);
}
QT_WARNING_POP
QT_END_MOC_NAMESPACE

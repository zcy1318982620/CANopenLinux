/****************************************************************************
** Meta object code from reading C++ file 'commworker.h'
**
** Created by: The Qt Meta Object Compiler version 67 (Qt 5.12.8)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include "commworker.h"
#include <QtCore/qbytearray.h>
#include <QtCore/qmetatype.h>
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'commworker.h' doesn't include <QObject>."
#elif Q_MOC_OUTPUT_REVISION != 67
#error "This file was generated using the moc from 5.12.8. It"
#error "cannot be used with the include files from this version of Qt."
#error "(The moc has changed too much.)"
#endif

QT_BEGIN_MOC_NAMESPACE
QT_WARNING_PUSH
QT_WARNING_DISABLE_DEPRECATED
struct qt_meta_stringdata_CommWorker_t {
    QByteArrayData data[29];
    char stringdata0[259];
};
#define QT_MOC_LITERAL(idx, ofs, len) \
    Q_STATIC_BYTE_ARRAY_DATA_HEADER_INITIALIZER_WITH_OFFSET(len, \
    qptrdiff(offsetof(qt_meta_stringdata_CommWorker_t, stringdata0) + ofs \
        - idx * sizeof(QByteArrayData)) \
    )
static const qt_meta_stringdata_CommWorker_t qt_meta_stringdata_CommWorker = {
    {
QT_MOC_LITERAL(0, 0, 10), // "CommWorker"
QT_MOC_LITERAL(1, 11, 9), // "connected"
QT_MOC_LITERAL(2, 21, 0), // ""
QT_MOC_LITERAL(3, 22, 12), // "disconnected"
QT_MOC_LITERAL(4, 35, 11), // "poseUpdated"
QT_MOC_LITERAL(5, 47, 1), // "x"
QT_MOC_LITERAL(6, 49, 1), // "y"
QT_MOC_LITERAL(7, 51, 2), // "th"
QT_MOC_LITERAL(8, 54, 7), // "logLine"
QT_MOC_LITERAL(9, 62, 3), // "msg"
QT_MOC_LITERAL(10, 66, 12), // "connectBoard"
QT_MOC_LITERAL(11, 79, 4), // "host"
QT_MOC_LITERAL(12, 84, 4), // "port"
QT_MOC_LITERAL(13, 89, 15), // "disconnectBoard"
QT_MOC_LITERAL(14, 105, 7), // "setNode"
QT_MOC_LITERAL(15, 113, 4), // "node"
QT_MOC_LITERAL(16, 118, 8), // "nmtPreOp"
QT_MOC_LITERAL(17, 127, 8), // "nmtStart"
QT_MOC_LITERAL(18, 136, 9), // "setCmdVel"
QT_MOC_LITERAL(19, 146, 1), // "v"
QT_MOC_LITERAL(20, 148, 1), // "w"
QT_MOC_LITERAL(21, 150, 11), // "onConnected"
QT_MOC_LITERAL(22, 162, 14), // "onDisconnected"
QT_MOC_LITERAL(23, 177, 11), // "onReadyRead"
QT_MOC_LITERAL(24, 189, 13), // "onSocketError"
QT_MOC_LITERAL(25, 203, 28), // "QAbstractSocket::SocketError"
QT_MOC_LITERAL(26, 232, 3), // "err"
QT_MOC_LITERAL(27, 236, 11), // "pollTimeout"
QT_MOC_LITERAL(28, 248, 10) // "cmdTimeout"

    },
    "CommWorker\0connected\0\0disconnected\0"
    "poseUpdated\0x\0y\0th\0logLine\0msg\0"
    "connectBoard\0host\0port\0disconnectBoard\0"
    "setNode\0node\0nmtPreOp\0nmtStart\0setCmdVel\0"
    "v\0w\0onConnected\0onDisconnected\0"
    "onReadyRead\0onSocketError\0"
    "QAbstractSocket::SocketError\0err\0"
    "pollTimeout\0cmdTimeout"
};
#undef QT_MOC_LITERAL

static const uint qt_meta_data_CommWorker[] = {

 // content:
       8,       // revision
       0,       // classname
       0,    0, // classinfo
      16,   14, // methods
       0,    0, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
       4,       // signalCount

 // signals: name, argc, parameters, tag, flags
       1,    0,   94,    2, 0x06 /* Public */,
       3,    0,   95,    2, 0x06 /* Public */,
       4,    3,   96,    2, 0x06 /* Public */,
       8,    1,  103,    2, 0x06 /* Public */,

 // slots: name, argc, parameters, tag, flags
      10,    2,  106,    2, 0x0a /* Public */,
      13,    0,  111,    2, 0x0a /* Public */,
      14,    1,  112,    2, 0x0a /* Public */,
      16,    0,  115,    2, 0x0a /* Public */,
      17,    0,  116,    2, 0x0a /* Public */,
      18,    2,  117,    2, 0x0a /* Public */,
      21,    0,  122,    2, 0x08 /* Private */,
      22,    0,  123,    2, 0x08 /* Private */,
      23,    0,  124,    2, 0x08 /* Private */,
      24,    1,  125,    2, 0x08 /* Private */,
      27,    0,  128,    2, 0x08 /* Private */,
      28,    0,  129,    2, 0x08 /* Private */,

 // signals: parameters
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void, QMetaType::Double, QMetaType::Double, QMetaType::Double,    5,    6,    7,
    QMetaType::Void, QMetaType::QString,    9,

 // slots: parameters
    QMetaType::Void, QMetaType::QString, QMetaType::UShort,   11,   12,
    QMetaType::Void,
    QMetaType::Void, QMetaType::Int,   15,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void, QMetaType::Double, QMetaType::Double,   19,   20,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void, 0x80000000 | 25,   26,
    QMetaType::Void,
    QMetaType::Void,

       0        // eod
};

void CommWorker::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    if (_c == QMetaObject::InvokeMetaMethod) {
        auto *_t = static_cast<CommWorker *>(_o);
        Q_UNUSED(_t)
        switch (_id) {
        case 0: _t->connected(); break;
        case 1: _t->disconnected(); break;
        case 2: _t->poseUpdated((*reinterpret_cast< double(*)>(_a[1])),(*reinterpret_cast< double(*)>(_a[2])),(*reinterpret_cast< double(*)>(_a[3]))); break;
        case 3: _t->logLine((*reinterpret_cast< const QString(*)>(_a[1]))); break;
        case 4: _t->connectBoard((*reinterpret_cast< const QString(*)>(_a[1])),(*reinterpret_cast< quint16(*)>(_a[2]))); break;
        case 5: _t->disconnectBoard(); break;
        case 6: _t->setNode((*reinterpret_cast< int(*)>(_a[1]))); break;
        case 7: _t->nmtPreOp(); break;
        case 8: _t->nmtStart(); break;
        case 9: _t->setCmdVel((*reinterpret_cast< double(*)>(_a[1])),(*reinterpret_cast< double(*)>(_a[2]))); break;
        case 10: _t->onConnected(); break;
        case 11: _t->onDisconnected(); break;
        case 12: _t->onReadyRead(); break;
        case 13: _t->onSocketError((*reinterpret_cast< QAbstractSocket::SocketError(*)>(_a[1]))); break;
        case 14: _t->pollTimeout(); break;
        case 15: _t->cmdTimeout(); break;
        default: ;
        }
    } else if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        switch (_id) {
        default: *reinterpret_cast<int*>(_a[0]) = -1; break;
        case 13:
            switch (*reinterpret_cast<int*>(_a[1])) {
            default: *reinterpret_cast<int*>(_a[0]) = -1; break;
            case 0:
                *reinterpret_cast<int*>(_a[0]) = qRegisterMetaType< QAbstractSocket::SocketError >(); break;
            }
            break;
        }
    } else if (_c == QMetaObject::IndexOfMethod) {
        int *result = reinterpret_cast<int *>(_a[0]);
        {
            using _t = void (CommWorker::*)();
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&CommWorker::connected)) {
                *result = 0;
                return;
            }
        }
        {
            using _t = void (CommWorker::*)();
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&CommWorker::disconnected)) {
                *result = 1;
                return;
            }
        }
        {
            using _t = void (CommWorker::*)(double , double , double );
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&CommWorker::poseUpdated)) {
                *result = 2;
                return;
            }
        }
        {
            using _t = void (CommWorker::*)(const QString & );
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&CommWorker::logLine)) {
                *result = 3;
                return;
            }
        }
    }
}

QT_INIT_METAOBJECT const QMetaObject CommWorker::staticMetaObject = { {
    &QObject::staticMetaObject,
    qt_meta_stringdata_CommWorker.data,
    qt_meta_data_CommWorker,
    qt_static_metacall,
    nullptr,
    nullptr
} };


const QMetaObject *CommWorker::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *CommWorker::qt_metacast(const char *_clname)
{
    if (!_clname) return nullptr;
    if (!strcmp(_clname, qt_meta_stringdata_CommWorker.stringdata0))
        return static_cast<void*>(this);
    return QObject::qt_metacast(_clname);
}

int CommWorker::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QObject::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 16)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 16;
    } else if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        if (_id < 16)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 16;
    }
    return _id;
}

// SIGNAL 0
void CommWorker::connected()
{
    QMetaObject::activate(this, &staticMetaObject, 0, nullptr);
}

// SIGNAL 1
void CommWorker::disconnected()
{
    QMetaObject::activate(this, &staticMetaObject, 1, nullptr);
}

// SIGNAL 2
void CommWorker::poseUpdated(double _t1, double _t2, double _t3)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(&_t1)), const_cast<void*>(reinterpret_cast<const void*>(&_t2)), const_cast<void*>(reinterpret_cast<const void*>(&_t3)) };
    QMetaObject::activate(this, &staticMetaObject, 2, _a);
}

// SIGNAL 3
void CommWorker::logLine(const QString & _t1)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(&_t1)) };
    QMetaObject::activate(this, &staticMetaObject, 3, _a);
}
QT_WARNING_POP
QT_END_MOC_NAMESPACE

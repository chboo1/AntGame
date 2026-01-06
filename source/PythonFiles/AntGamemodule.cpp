#define PY_SSIZE_T_CLEAN
#include <Python.h>

#include <deque>
#include <iostream>
#include <cstdint>
#include <string>
#include <thread>
#include <chrono>
#include <stddef.h>


#include "../headers/sockets.hpp"
#include "../headers/network.hpp"
#include "../headers/map.hpp"


struct PosObject
{
    PyObject_HEAD
    DPos p;
};


struct PPosObject
{
    PyObject_HEAD
    DPos p;
};


static PyMemberDef Pos_members[] = {
    {"x", Py_T_DOUBLE, offsetof(PosObject, p.x), 0, "X position"},
    {"y", Py_T_DOUBLE, offsetof(PosObject, p.y), 0, "Y position"},
    {nullptr}
};


static PyTypeObject PosType = {
    .ob_base = PyVarObject_HEAD_INIT(nullptr, 0)
    .tp_name = "AntGame.Pos",
    .tp_basicsize = sizeof(PosObject),
    .tp_itemsize = 0,
    .tp_flags = Py_TPFLAGS_DEFAULT,
    //TODO
    .tp_doc = PyDoc_STR("Placeholder doc string"),
    .tp_members = Pos_members,
};


static PyObject* PPos_getx(PyObject* op, void* closure)
{
    PPosObject* self = (PPosObject*)op;
    return PyFloat_FromDouble(self->p.x);
}


static PyObject* PPos_gety(PyObject* op, void* closure)
{
    PPosObject* self = (PPosObject*)op;
    return PyFloat_FromDouble(self->p.y);
}


static PyGetSetDef PPos_getsetters[] = {
    {"x", PPos_getx, nullptr, "X position", nullptr},
    {"y", PPos_gety, nullptr, "Y position", nullptr},
    {nullptr}
};


static PyTypeObject PPosType = {
    .ob_base = PyVarObject_HEAD_INIT(nullptr, 0)
    .tp_name = "AntGame.PPos",
    .tp_basicsize = sizeof(PPosObject),
    .tp_itemsize = 0,
    .tp_flags = Py_TPFLAGS_DEFAULT,
    //TODO
    .tp_doc = PyDoc_STR("Placeholder doc string"),
    .tp_getset = PPos_getsetters,
};


struct AntGameClientObject
{
    PyObject_HEAD
    std::deque<ConnectionManager::RequestID> reqIDs;
    std::deque<ConnectionManager::RequestID> cmdIDs;
    std::deque<unsigned int> cmdpids;
    std::deque<std::uint64_t> cmdargs;
    std::string recvData = "";

    Map*map = nullptr;
    Connection*conn = nullptr;

    unsigned char selfNestID = 0xff;
    std::string name = "";
    std::string address = "";
    int port = ANTNET_DEFAULT_PORT;
};





static void AntGameClient_dealloc(PyObject* op)
{
    AntGameClientObject*self = (AntGameClientObject*)op;
    if (self->map)
    {
        delete self->map;
    }
    if (self->conn)
    {
        self->conn->finish();
        delete self->conn;
    }
    Py_TYPE(self)->tp_free(self);
}


static PyObject* AntGameClient_getnests(PyObject* op, void *closure);
static PyObject* AntGameClient_getants(PyObject* op, void *closure);
static PyObject* AntGameClient_getmeid(PyObject* op, void* closure)
{
    AntGameClientObject*self = (AntGameClientObject*)op;
    if (self->map == nullptr || self->selfNestID == 0xff || self->conn == nullptr || !self->conn->connected())
    {
        PyErr_SetString(PyExc_RuntimeWarning, "Your client is not connected! Connect first to get meid.");
        return nullptr;
    }
    return PyLong_FromUInt32((std::uint32_t)self->selfNestID);
}
static PyObject* AntGameClient_getme(PyObject* op, void* closure);


static PyGetSetDef AntGameClient_getsetters[] = {
    {"nests", AntGameClient_getnests, nullptr, "map's nests", nullptr},
    {"ants", AntGameClient_getants, nullptr, "map's ants", nullptr},
    {"meID", AntGameClient_getmeid, nullptr, "this client's ID", nullptr},
    {"me", AntGameClient_getme, nullptr, "this client's nest", nullptr},
    {nullptr}
};


static PyMemberDef AntGameClient_members[] = {
    {"port", Py_T_INT, offsetof(AntGameClientObject, port), 0, "remote port"},
    {nullptr}
};


static PyTypeObject AntGameClientType = {
    .ob_base = PyVarObject_HEAD_INIT(nullptr, 0)
    .tp_name = "AntGame.AntGameClient",
    .tp_basicsize = sizeof(AntGameClientObject),
    .tp_itemsize = 0,
    .tp_dealloc = AntGameClient_dealloc,
    .tp_flags = Py_TPFLAGS_DEFAULT,
    //TODO
    .tp_doc = PyDoc_STR("Placeholder doc string"),
    .tp_members = AntGameClient_members,
    .tp_getset = AntGameClient_getsetters,
    .tp_new = PyType_GenericNew,
};


struct NestObject
{
    PyObject_HEAD
    ;
    PyObject*root;
    unsigned char nestID;
};


static PyTypeObject NestType = {
    .ob_base = PyVarObject_HEAD_INIT(nullptr, 0)
    .tp_name = "AntGame.Nest",
    .tp_basicsize = sizeof(NestObject),
    .tp_itemsize = 0,
    .tp_flags = Py_TPFLAGS_DEFAULT,
    //TODO
    .tp_doc = PyDoc_STR("Placeholder doc string"),
    .tp_new = PyType_GenericNew,
};


struct AntObject
{
    PyObject_HEAD
    ;
    PyObject*root;
    unsigned int antID;
};


static PyTypeObject AntType = {
    .ob_base = PyVarObject_HEAD_INIT(nullptr, 0)
    .tp_name = "AntGame.Ant",
    .tp_basicsize = sizeof(AntObject),
    .tp_itemsize = 0,
    .tp_flags = Py_TPFLAGS_DEFAULT,
    //TODO
    .tp_doc = PyDoc_STR("Placeholder doc string"),
    .tp_new = PyType_GenericNew,
};


static PyObject* AntGameClient_getnests(PyObject* op, void *closure)
{
    AntGameClientObject*self = (AntGameClientObject*)op;
    if (self->map == nullptr || self->map->nests.empty())
    {
        return PyList_New(0);
    }
    PyObject* ret = PyList_New(self->map->nests.size());
    if (!ret)
    {
        PyErr_SetString(PyExc_MemoryError, "Failed to get memory for the list of nests!");
        return nullptr;
    }
    for (int i = 0; i < self->map->nests.size() && i < 256; i++)
    {
        NestObject* item = new NestObject;
        if (!item)
        {
            Py_DECREF(ret);
            PyErr_SetString(PyExc_MemoryError, "Failed to get memory for a nest in the list of nests!");
            return nullptr;
        }
        item->root = op;
        item->nestID = i;
        Py_INCREF(item);
        PyList_SetItem(ret, i, (PyObject*)item);
    }
    return ret;
}


static PyObject* AntGameClient_getants(PyObject* op, void *closure)
{
    AntGameClientObject*self = (AntGameClientObject*)op;
    if (self->map == nullptr || self->map->antPermanents.empty())
    {
        return PyList_New(0);
    }
    unsigned int amount = 0;
    for (Ant*a : self->map->antPermanents)
    {
        if (a)
        {
            amount++;
        }
    }
    PyObject* ret = PyList_New(amount);
    if (!ret)
    {
        return nullptr;
    }
    unsigned int j = 0;
    for (unsigned int i = 0; i < self->map->antPermanents.size(); i++)
    {
        if (!self->map->antPermanents[i])
        {
            continue;
        }
        AntObject* item = new AntObject;
        if (!item)
        {
            Py_DECREF(ret);
            return nullptr;
        }
        item->root = op;
        item->antID = i;
        Py_INCREF(item);
        PyList_SetItem(ret, j, (PyObject*)item);
        j++;
    }
    return ret;
}


static PyObject* AntGameClient_getme(PyObject* op, void* closure)
{
    AntGameClientObject*self = (AntGameClientObject*)op;
    if (self->map == nullptr || self->selfNestID == 0xff || self->conn == nullptr || !self->conn->connected())
    {
        PyErr_SetString(PyExc_RuntimeWarning, "Your client is not connected! Connect first to get meid.");
        return nullptr;
    }
    NestObject*nobj = new NestObject;
    nobj->root = op;
    nobj->nestID = self->selfNestID;
    Py_INCREF((PyObject*)nobj);
    return (PyObject*)nobj;
}


static int AntGame_module_exec(PyObject *m)
{
    if (PyType_Ready(&AntGameClientType) < 0)
    {
        return -1;
    }
    if (PyModule_AddObjectRef(m, "AntGameClient", (PyObject*)&AntGameClientType) < 0)
    {
        return -1;
    }
    if (PyType_Ready(&NestType) < 0)
    {
        return -1;
    }
    if (PyModule_AddObjectRef(m, "Nest", (PyObject*)&NestType) < 0)
    {
        return -1;
    }
    if (PyType_Ready(&AntType) < 0)
    {
        return -1;
    }
    if (PyModule_AddObjectRef(m, "Ant", (PyObject*)&AntType) < 0)
    {
        return -1;
    }
    if (PyType_Ready(&PosType) < 0)
    {
        return -1;
    }
    if (PyModule_AddObjectRef(m, "Pos", (PyObject*)&PosType) < 0)
    {
        return -1;
    }
    if (PyType_Ready(&PPosType) < 0)
    {
        return -1;
    }
    if (PyModule_AddObjectRef(m, "PPos", (PyObject*)&PPosType) < 0)
    {
        return -1;
    }
    return 0;
}


static PyMethodDef AntGame_methods[] = {
    {nullptr, nullptr, 0, nullptr}
};


static PyModuleDef_Slot AntGame_module_slots[] = {
    {Py_mod_exec, (void*)AntGame_module_exec},
    {0, nullptr}
};


static struct PyModuleDef AntGame_module = {
    .m_base = PyModuleDef_HEAD_INIT,
    .m_name = "AntGame",
    .m_size = 0,
    .m_methods = AntGame_methods,
    .m_slots = AntGame_module_slots
};


PyMODINIT_FUNC PyInit_AntGame(void)
{
    return PyModuleDef_Init(&AntGame_module);
}
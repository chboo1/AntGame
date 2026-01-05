#define PY_SSIZE_T_CLEAN
#include <Python.h>

#include <deque>
#include <iostream>
#include <cstdint>
#include <string>
#include <thread>
#include <chrono>


#include "../headers/sockets.hpp"
#include "../headers/network.hpp"
#include "../headers/map.hpp"


struct AntGameClientObject
{
    PyObject_HEAD
    ;
    Map map;
    unsigned char selfNestID = 0xff;
    std::deque<ConnectionManager::RequestID> reqIDs;
    std::deque<ConnectionManager::RequestID> cmdIDs;
    std::deque<unsigned int> cmdpids;
    std::deque<std::uint64_t> cmdargs;
};


static PyTypeObject AntGameClientType = {
    .ob_base = PyVarObject_HEAD_INIT(nullptr, 0)
    .tp_name = "AntGame.AntGameClient",
    .tp_basicsize = sizeof(AntGameClientObject),
    .tp_itemsize = 0,
    .tp_flags = Py_TPFLAGS_DEFAULT,
    //TODO
    .tp_doc = PyDoc_STR("Placeholder doc string"),
    .tp_new = PyType_GenericNew,
};


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
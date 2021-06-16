#include <Python.h>
#include "hw.h"

int put_item(char *key, void *ptr) {
    PyObject *pName, *pModule, *pFunc;
    PyObject *pArgs, *pValue;
    int i;

    // set python working dir
    char setenv[1024];
    char path[256];
    getcwd(path, sizeof(path));
    sprintf(setenv, "PYTHONPATH=%s", path);
    putenv(setenv);

    Py_Initialize();

    // get python module and function name   
    pName = PyString_FromString("boto3_itf");
    pModule = PyImport_Import(pName);
    Py_DECREF(pName);
    if (pModule != NULL) {
        pFunc = PyObject_GetAttrString(pModule, "put_item");
        if (pFunc && PyCallable_Check(pFunc)) {
            // set args
            pArgs = PyTuple_New(2);
            pValue = PyString_FromString(key);
            PyTuple_SetItem(pArgs, 0, pValue);

            // woof seq_no
            pValue = PyString_FromString((char *)ptr);
            PyTuple_SetItem(pArgs, 1, pValue);

            // call function
            pValue = PyObject_CallObject(pFunc, pArgs);
            Py_DECREF(pArgs);
            if (pValue != NULL) {
                printf("Result of call: %ld\n", PyInt_AsLong(pValue));
                Py_DECREF(pValue);
            } else {
                Py_DECREF(pFunc);
                Py_DECREF(pModule);
                PyErr_Print();
                fprintf(stderr,"Call failed\n");
                return 1;
            }
        } else {
            if (PyErr_Occurred()) {
                PyErr_Print();
            }
            fprintf(stderr, "Cannot find function \"%s\"\n", "python_handler");
        }
        Py_XDECREF(pFunc);
        Py_DECREF(pModule);
    } else {
        PyErr_Print();
        fprintf(stderr, "Failed to load \"%s\"\n", "PYTHON_MODULE");
        return 1;
    }
    Py_Finalize();
    return 0;
}

#include <Python.h>
#include "woofc.h"

int python_handler(WOOF *wf, unsigned long seq_no, void *ptr) {
    PyObject *pName, *pModule, *pFunc;
    PyObject *pArgs, *pValue;
    int i;

    // set python working dir
    char setenv[1024];
    sprintf(setenv, "PYTHONPATH=/cspot");
    putenv(setenv);

    Py_Initialize();

    // get python module and function name   
    pName = PyString_FromString("python_handler");
    pModule = PyImport_Import(pName);
    Py_DECREF(pName);
    if (pModule != NULL) {
        pFunc = PyObject_GetAttrString(pModule, "python_handler");
        if (pFunc && PyCallable_Check(pFunc)) {
            // set args
            pArgs = PyTuple_New(2);

            // woof structure
            pValue = Py_BuildValue("{s:s,s:i,s:i,s:i,s:i,s:i}",
                "filename", wf->shared->filename,
                "seq_no", wf->shared->seq_no,
                "history_size", wf->shared->history_size,
                "head", wf->shared->head,
                "tail", wf->shared->tail,
                "element_size", wf->shared->element_size
                );
            PyTuple_SetItem(pArgs, 0, pValue);

            // woof seq_no
            pValue = PyInt_FromLong(seq_no);
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
        fprintf(stderr, "Failed to load \"%s\"\n", "python_handler");
        return 1;
    }
    Py_Finalize();
    return 0;
}

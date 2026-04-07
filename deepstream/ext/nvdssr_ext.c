/*
 * nvdssr_ext — CPython C extension for DeepStream SmartRecord.
 *
 * Bridges the gpointer gap: Python GI cannot emit action signals whose
 * parameters include C output pointers (gpointer).  This module wraps
 * g_signal_emit_by_name for the start-sr / stop-sr signals on nvurisrcbin
 * and provides helpers to:
 *   - extract GstElement* from pyservicemaker Node (pybind11 object)
 *   - emit start-sr / stop-sr on an element given as a raw pointer
 *   - parse NvDsSRRecordingInfo structs from sr-done callback pointers
 */

#define PY_SSIZE_T_CLEAN
#include <Python.h>
#include <stdint.h>
#include <gst/gst.h>
#include "gst-nvdssr.h"

/*
 * Extract GstElement* from either:
 *   (a) a PyGObject (gi element)  — GObject* at PyObject_HEAD + 0
 *   (b) a pyservicemaker Node     — C++ Object at pybind11 offset 16,
 *                                   GstObject* at C++ offset 8
 *   (c) an integer (raw pointer)
 *
 * Returns the validated GstElement* or sets an error and returns NULL.
 */
static GstElement *
_resolve_element(PyObject *py_obj)
{
    /* Case (c): integer — raw pointer */
    if (PyLong_Check(py_obj)) {
        uintptr_t addr = (uintptr_t)PyLong_AsUnsignedLongLong(py_obj);
        if (PyErr_Occurred()) return NULL;
        if (!addr) {
            PyErr_SetString(PyExc_ValueError, "NULL element pointer");
            return NULL;
        }
        GstElement *elem = (GstElement *)addr;
        if (!GST_IS_ELEMENT(elem)) {
            PyErr_SetString(PyExc_TypeError,
                            "integer does not point to a valid GstElement");
            return NULL;
        }
        return elem;
    }

    /* Case (a): PyGObject — check for __gpointer__ attribute */
    if (PyObject_HasAttrString(py_obj, "__gpointer__")) {
        /* Minimal PyGObject layout: PyObject_HEAD + GObject* */
        GObject *gobj = *(GObject **)((char *)py_obj + sizeof(PyObject));
        if (gobj && GST_IS_ELEMENT(gobj))
            return GST_ELEMENT(gobj);
        PyErr_SetString(PyExc_TypeError,
                        "PyGObject does not wrap a valid GstElement");
        return NULL;
    }

    /*
     * Case (b): pyservicemaker Node (pybind11)
     *
     * pybind11 layout: PyObject_HEAD (16 bytes) + C++ ptr (8 bytes)
     * C++ Object layout (from object.hpp):
     *   offset 0: vtable pointer
     *   offset 8: GstObject* object_
     */
    void *cpp_ptr = *(void **)((char *)py_obj + sizeof(PyObject));
    if (!cpp_ptr) {
        PyErr_SetString(PyExc_ValueError, "null C++ object in pybind11 wrapper");
        return NULL;
    }

    GstObject *gst_obj = *(GstObject **)((char *)cpp_ptr + 8);
    if (gst_obj && GST_IS_ELEMENT(gst_obj))
        return GST_ELEMENT(gst_obj);

    /* Fallback: scan first 64 bytes of C++ object */
    for (int off = 0; off <= 56; off += 8) {
        void *candidate = *(void **)((char *)cpp_ptr + off);
        if (candidate && ((uintptr_t)candidate > 0x10000) &&
            G_IS_OBJECT(candidate) && GST_IS_ELEMENT(candidate)) {
            return GST_ELEMENT(candidate);
        }
    }

    PyErr_SetString(PyExc_TypeError,
                    "cannot extract GstElement from the given object");
    return NULL;
}

/* ------------------------------------------------------------------ */
/* extract_gst_element(node) -> int (GstElement pointer)              */
/* ------------------------------------------------------------------ */

static PyObject *
nvdssr_extract_gst_element(PyObject *self, PyObject *args)
{
    PyObject *py_obj;
    if (!PyArg_ParseTuple(args, "O", &py_obj))
        return NULL;

    GstElement *elem = _resolve_element(py_obj);
    if (!elem)
        return NULL;

    g_object_ref(elem);
    return PyLong_FromUnsignedLongLong((uintptr_t)elem);
}

/* ------------------------------------------------------------------ */
/* release_gst_element(ptr) -> None                                   */
/* ------------------------------------------------------------------ */

static PyObject *
nvdssr_release_gst_element(PyObject *self, PyObject *args)
{
    unsigned long long ptr_val;
    if (!PyArg_ParseTuple(args, "K", &ptr_val))
        return NULL;

    GstElement *elem = (GstElement *)(uintptr_t)ptr_val;
    if (elem && GST_IS_ELEMENT(elem))
        g_object_unref(elem);

    Py_RETURN_NONE;
}

/* ------------------------------------------------------------------ */
/* find_child_nvurisrcbin(parent_ptr, source_id) -> int or None       */
/* ------------------------------------------------------------------ */

static PyObject *
nvdssr_find_child_nvurisrcbin(PyObject *self, PyObject *args)
{
    unsigned long long parent_ptr;
    int source_id;
    if (!PyArg_ParseTuple(args, "Ki", &parent_ptr, &source_id))
        return NULL;

    GstElement *parent = (GstElement *)(uintptr_t)parent_ptr;
    if (!parent || !GST_IS_BIN(parent)) {
        PyErr_SetString(PyExc_TypeError, "parent is not a valid GstBin");
        return NULL;
    }

    /* Try by name first: dsnvurisrcbin{source_id} */
    char name_buf[64];
    snprintf(name_buf, sizeof(name_buf), "dsnvurisrcbin%d", source_id);

    GstElement *child = gst_bin_get_by_name_recurse_up(
        GST_BIN(parent), name_buf);
    if (!child)
        child = gst_bin_get_by_name(GST_BIN(parent), name_buf);

    if (child && GST_IS_ELEMENT(child)) {
        /* gst_bin_get_by_name adds a ref; keep it */
        return PyLong_FromUnsignedLongLong((uintptr_t)child);
    }

    /* Fallback: iterate and find Nth nvurisrcbin by factory name */
    GstIterator *it = gst_bin_iterate_recurse(GST_BIN(parent));
    GValue item = G_VALUE_INIT;
    int idx = 0;
    GstElement *result = NULL;

    while (gst_iterator_next(it, &item) == GST_ITERATOR_OK) {
        GstElement *elem = GST_ELEMENT(g_value_get_object(&item));
        GstElementFactory *factory = gst_element_get_factory(elem);
        if (factory &&
            g_strcmp0(gst_plugin_feature_get_name(
                          GST_PLUGIN_FEATURE(factory)),
                      "nvurisrcbin") == 0) {
            if (idx == source_id) {
                result = elem;
                g_object_ref(result);
                g_value_unset(&item);
                break;
            }
            idx++;
        }
        g_value_unset(&item);
    }
    gst_iterator_free(it);

    if (result)
        return PyLong_FromUnsignedLongLong((uintptr_t)result);

    Py_RETURN_NONE;
}

/* ------------------------------------------------------------------ */
/* start_recording(element, start_time, duration) -> session_id       */
/* ------------------------------------------------------------------ */

static PyObject *
nvdssr_start_recording(PyObject *self, PyObject *args)
{
    PyObject *py_element;
    unsigned int start_time;
    unsigned int duration;

    if (!PyArg_ParseTuple(args, "OII", &py_element, &start_time, &duration))
        return NULL;

    GstElement *element = _resolve_element(py_element);
    if (!element)
        return NULL;

    NvDsSRSessionId session_id = 0;

    g_signal_emit_by_name(element, "start-sr",
                          &session_id,
                          start_time,
                          duration,
                          NULL);

    return PyLong_FromUnsignedLong((unsigned long)session_id);
}

/* ------------------------------------------------------------------ */
/* stop_recording(element, session_id) -> None                        */
/* ------------------------------------------------------------------ */

static PyObject *
nvdssr_stop_recording(PyObject *self, PyObject *args)
{
    PyObject *py_element;
    unsigned int session_id;

    if (!PyArg_ParseTuple(args, "OI", &py_element, &session_id))
        return NULL;

    GstElement *element = _resolve_element(py_element);
    if (!element)
        return NULL;

    g_signal_emit_by_name(element, "stop-sr", (guint)session_id);

    Py_RETURN_NONE;
}

/* ------------------------------------------------------------------ */
/* parse_recording_info(ptr) -> dict                                  */
/* ------------------------------------------------------------------ */

static PyObject *
nvdssr_parse_recording_info(PyObject *self, PyObject *args)
{
    unsigned long long ptr_val;

    if (!PyArg_ParseTuple(args, "K", &ptr_val))
        return NULL;

    NvDsSRRecordingInfo *info = (NvDsSRRecordingInfo *)(uintptr_t)ptr_val;
    if (!info) {
        PyErr_SetString(PyExc_ValueError, "NULL pointer");
        return NULL;
    }

    PyObject *dict = PyDict_New();
    if (!dict)
        return NULL;

    PyObject *v;

    v = PyLong_FromUnsignedLong(info->sessionId);
    PyDict_SetItemString(dict, "session_id", v); Py_DECREF(v);

    v = info->filename ? PyUnicode_FromString(info->filename) : Py_NewRef(Py_None);
    PyDict_SetItemString(dict, "filename", v); Py_DECREF(v);

    v = info->dirpath ? PyUnicode_FromString(info->dirpath) : Py_NewRef(Py_None);
    PyDict_SetItemString(dict, "dirpath", v); Py_DECREF(v);

    v = PyLong_FromUnsignedLongLong(info->duration);
    PyDict_SetItemString(dict, "duration", v); Py_DECREF(v);

    v = PyLong_FromLong(info->containerType);
    PyDict_SetItemString(dict, "container_type", v); Py_DECREF(v);

    v = PyLong_FromUnsignedLong(info->width);
    PyDict_SetItemString(dict, "width", v); Py_DECREF(v);

    v = PyLong_FromUnsignedLong(info->height);
    PyDict_SetItemString(dict, "height", v); Py_DECREF(v);

    v = PyBool_FromLong(info->containsVideo);
    PyDict_SetItemString(dict, "contains_video", v); Py_DECREF(v);

    v = PyBool_FromLong(info->containsAudio);
    PyDict_SetItemString(dict, "contains_audio", v); Py_DECREF(v);

    return dict;
}

/* ------------------------------------------------------------------ */
/* get_element_name(ptr) -> str                                       */
/* ------------------------------------------------------------------ */

static PyObject *
nvdssr_get_element_name(PyObject *self, PyObject *args)
{
    unsigned long long ptr_val;
    if (!PyArg_ParseTuple(args, "K", &ptr_val))
        return NULL;

    GstElement *elem = (GstElement *)(uintptr_t)ptr_val;
    if (!elem || !GST_IS_ELEMENT(elem)) {
        PyErr_SetString(PyExc_ValueError, "not a valid GstElement pointer");
        return NULL;
    }

    gchar *name = gst_object_get_name(GST_OBJECT(elem));
    PyObject *result = PyUnicode_FromString(name ? name : "(null)");
    g_free(name);
    return result;
}

/* ------------------------------------------------------------------ */
/* Module definition                                                  */
/* ------------------------------------------------------------------ */

static PyMethodDef nvdssr_methods[] = {
    {"extract_gst_element", nvdssr_extract_gst_element, METH_VARARGS,
     "extract_gst_element(node) -> int\n\n"
     "Extract the GstElement pointer from a pyservicemaker Node or PyGObject.\n"
     "Returns the pointer as an integer (with a ref added)."},
    {"release_gst_element", nvdssr_release_gst_element, METH_VARARGS,
     "release_gst_element(ptr) -> None\n\n"
     "Release a GstElement reference obtained from extract_gst_element."},
    {"find_child_nvurisrcbin", nvdssr_find_child_nvurisrcbin, METH_VARARGS,
     "find_child_nvurisrcbin(parent_ptr, source_id) -> int or None\n\n"
     "Find a child nvurisrcbin element inside a GstBin by source_id."},
    {"start_recording", nvdssr_start_recording, METH_VARARGS,
     "start_recording(element, start_time, duration) -> session_id\n\n"
     "Emit 'start-sr' on an nvurisrcbin element.\n"
     "element can be a PyGObject, a pyservicemaker Node, or an int pointer."},
    {"stop_recording", nvdssr_stop_recording, METH_VARARGS,
     "stop_recording(element, session_id) -> None\n\n"
     "Emit 'stop-sr' on an nvurisrcbin element."},
    {"parse_recording_info", nvdssr_parse_recording_info, METH_VARARGS,
     "parse_recording_info(ptr) -> dict\n\n"
     "Parse an NvDsSRRecordingInfo pointer into a Python dict."},
    {"get_element_name", nvdssr_get_element_name, METH_VARARGS,
     "get_element_name(ptr) -> str\n\n"
     "Get the GStreamer element name from a raw pointer."},
    {NULL, NULL, 0, NULL}
};

static struct PyModuleDef nvdssr_module = {
    PyModuleDef_HEAD_INIT,
    "nvdssr_ext",
    "CPython bridge for DeepStream SmartRecord signals (start-sr / stop-sr / sr-done).\n"
    "Also provides GstElement extraction from pyservicemaker Node objects.",
    -1,
    nvdssr_methods,
};

PyMODINIT_FUNC
PyInit_nvdssr_ext(void)
{
    return PyModule_Create(&nvdssr_module);
}

#include <string.h>
#include <stdarg.h>
#include <assert.h>

#include "nlr.h"
#include "misc.h"
#include "mpconfig.h"
#include "qstr.h"
#include "obj.h"
#include "objtuple.h"

// This is unified class for C-level and Python-level exceptions
// Python-level exceptions have empty ->msg and all arguments are in
// args tuple. C-level exceptions likely have ->msg set, and args is empty.
typedef struct mp_obj_exception_t {
    mp_obj_base_t base;
    mp_obj_t traceback; // a list object, holding (file,line,block) as numbers (not Python objects); a hack for now
    vstr_t *msg;
    mp_obj_tuple_t args;
} mp_obj_exception_t;

STATIC void mp_obj_exception_print(void (*print)(void *env, const char *fmt, ...), void *env, mp_obj_t o_in, mp_print_kind_t kind) {
    mp_obj_exception_t *o = o_in;
    if (o->msg != NULL) {
        print(env, "%s: %s", qstr_str(o->base.type->name), vstr_str(o->msg));
    } else {
        // Yes, that's how CPython has it
        // TODO now that exceptions are classes and instances, I think this needs to be changed to match CPython
        if (kind == PRINT_REPR) {
            print(env, "%s", qstr_str(o->base.type->name));
        }
        if (kind == PRINT_STR) {
            if (o->args.len == 0) {
                print(env, "");
                return;
            } else if (o->args.len == 1) {
                mp_obj_print_helper(print, env, o->args.items[0], PRINT_STR);
                return;
            }
        }
        tuple_print(print, env, &o->args, kind);
    }
}

STATIC mp_obj_t mp_obj_exception_make_new(mp_obj_t type_in, uint n_args, uint n_kw, const mp_obj_t *args) {
    mp_obj_type_t *type = type_in;

    if (n_kw != 0) {
        nlr_jump(mp_obj_new_exception_msg_varg(&mp_type_TypeError, "%s does not take keyword arguments", mp_obj_get_type_str(type_in)));
    }

    mp_obj_exception_t *o = m_new_obj_var(mp_obj_exception_t, mp_obj_t, n_args);
    o->base.type = type;
    o->traceback = MP_OBJ_NULL;
    o->msg = NULL;
    o->args.base.type = &tuple_type;
    o->args.len = n_args;
    memcpy(o->args.items, args, n_args * sizeof(mp_obj_t));
    return o;
}

STATIC void exception_load_attr(mp_obj_t self_in, qstr attr, mp_obj_t *dest) {
    mp_obj_exception_t *self = self_in;
    if (attr == MP_QSTR_args) {
        dest[0] = &self->args;
    } else if (self->base.type == &mp_type_StopIteration && attr == MP_QSTR_value) {
        if (self->args.len == 0) {
            dest[0] = mp_const_none;
        } else {
            dest[0] = self->args.items[0];
        }
    }
}

const mp_obj_type_t mp_type_BaseException = {
    { &mp_type_type },
    .name = MP_QSTR_BaseException,
    .print = mp_obj_exception_print,
    .make_new = mp_obj_exception_make_new,
    .load_attr = exception_load_attr,
};

#define MP_DEFINE_EXCEPTION_BASE(base_name) \
STATIC const mp_obj_tuple_t mp_type_ ## base_name ## _base_tuple = {{&tuple_type}, 1, {(mp_obj_t)&mp_type_ ## base_name}};\

#define MP_DEFINE_EXCEPTION(exc_name, base_name) \
const mp_obj_type_t mp_type_ ## exc_name = { \
    { &mp_type_type }, \
    .name = MP_QSTR_ ## exc_name, \
    .print = mp_obj_exception_print, \
    .make_new = mp_obj_exception_make_new, \
    .load_attr = exception_load_attr, \
    .bases_tuple = (mp_obj_t)&mp_type_ ## base_name ## _base_tuple, \
};

// List of all exceptions, arranged as in the table at:
// http://docs.python.org/3.3/library/exceptions.html
MP_DEFINE_EXCEPTION_BASE(BaseException)
//MP_DEFINE_EXCEPTION(SystemExit, BaseException)
//MP_DEFINE_EXCEPTION(KeyboardInterrupt, BaseException)
MP_DEFINE_EXCEPTION(GeneratorExit, BaseException)
MP_DEFINE_EXCEPTION(Exception, BaseException)
  MP_DEFINE_EXCEPTION_BASE(Exception)
  MP_DEFINE_EXCEPTION(StopIteration, Exception)
  MP_DEFINE_EXCEPTION(ArithmeticError, Exception)
    MP_DEFINE_EXCEPTION_BASE(ArithmeticError)
    MP_DEFINE_EXCEPTION(FloatingPointError, ArithmeticError)
    MP_DEFINE_EXCEPTION(OverflowError, ArithmeticError)
    MP_DEFINE_EXCEPTION(ZeroDivisionError, ArithmeticError)
  MP_DEFINE_EXCEPTION(AssertionError, Exception)
  MP_DEFINE_EXCEPTION(AttributeError, Exception)
  MP_DEFINE_EXCEPTION(BufferError, Exception)
  //MP_DEFINE_EXCEPTION(EnvironmentError, Exception)
  MP_DEFINE_EXCEPTION(EOFError, Exception)
  MP_DEFINE_EXCEPTION(ImportError, Exception)
  MP_DEFINE_EXCEPTION(IOError, Exception)
  MP_DEFINE_EXCEPTION(LookupError, Exception)
    MP_DEFINE_EXCEPTION_BASE(LookupError)
    MP_DEFINE_EXCEPTION(IndexError, LookupError)
    MP_DEFINE_EXCEPTION(KeyError, LookupError)
  MP_DEFINE_EXCEPTION(MemoryError, Exception)
  MP_DEFINE_EXCEPTION(NameError, Exception)
    MP_DEFINE_EXCEPTION_BASE(NameError)
    MP_DEFINE_EXCEPTION(UnboundLocalError, NameError)
  MP_DEFINE_EXCEPTION(OSError, Exception)
    MP_DEFINE_EXCEPTION_BASE(OSError)
    // Probably don't need these
    /*MP_DEFINE_EXCEPTION(BlockingIOError, OSError)
    MP_DEFINE_EXCEPTION(ChildProcessError, OSError)
    MP_DEFINE_EXCEPTION(ConnectionError, OSError)
      MP_DEFINE_EXCEPTION(BrokenPipeError, ConnectionError)
      MP_DEFINE_EXCEPTION(ConnectionAbortedError, ConnectionError)
      MP_DEFINE_EXCEPTION(ConnectionRefusedError, ConnectionError)
      MP_DEFINE_EXCEPTION(ConnectionResetError, ConnectionError)
    MP_DEFINE_EXCEPTION(InterruptedError, OSError)
    MP_DEFINE_EXCEPTION(IsADirectoryError, OSError)
    MP_DEFINE_EXCEPTION(NotADirectoryError, OSError)
    MP_DEFINE_EXCEPTION(PermissionError, OSError)
    MP_DEFINE_EXCEPTION(ProcessLookupError, OSError)
    MP_DEFINE_EXCEPTION(TimeoutError, OSError)*/
    MP_DEFINE_EXCEPTION(FileExistsError, OSError)
    MP_DEFINE_EXCEPTION(FileNotFoundError, OSError)
  //MP_DEFINE_EXCEPTION(ReferenceError, Exception)
  MP_DEFINE_EXCEPTION(RuntimeError, Exception)
    MP_DEFINE_EXCEPTION_BASE(RuntimeError)
    MP_DEFINE_EXCEPTION(NotImplementedError, RuntimeError)
  MP_DEFINE_EXCEPTION(SyntaxError, Exception)
    MP_DEFINE_EXCEPTION_BASE(SyntaxError)
    MP_DEFINE_EXCEPTION(IndentationError, SyntaxError)
      MP_DEFINE_EXCEPTION_BASE(IndentationError)
      //MP_DEFINE_EXCEPTION(TabError, IndentationError)
  MP_DEFINE_EXCEPTION(SystemError, Exception)
  MP_DEFINE_EXCEPTION(TypeError, Exception)
  MP_DEFINE_EXCEPTION(ValueError, Exception)
    //TODO: Implement UnicodeErrors which take arguments
  /*
  MP_DEFINE_EXCEPTION(Warning, Exception)
    MP_DEFINE_EXCEPTION_BASE(Warning)
    MP_DEFINE_EXCEPTION(DeprecationWarning, Warning)
    MP_DEFINE_EXCEPTION(PendingDeprecationWarning, Warning)
    MP_DEFINE_EXCEPTION(RuntimeWarning, Warning)
    MP_DEFINE_EXCEPTION(SyntaxWarning, Warning)
    MP_DEFINE_EXCEPTION(UserWarning, Warning)
    MP_DEFINE_EXCEPTION(FutureWarning, Warning)
    MP_DEFINE_EXCEPTION(ImportWarning, Warning)
    MP_DEFINE_EXCEPTION(UnicodeWarning, Warning)
    MP_DEFINE_EXCEPTION(BytesWarning, Warning)
    MP_DEFINE_EXCEPTION(ResourceWarning, Warning)
    */

mp_obj_t mp_obj_new_exception(const mp_obj_type_t *exc_type) {
    return mp_obj_new_exception_msg_varg(exc_type, NULL);
}

mp_obj_t mp_obj_new_exception_msg(const mp_obj_type_t *exc_type, const char *msg) {
    return mp_obj_new_exception_msg_varg(exc_type, msg);
}

mp_obj_t mp_obj_new_exception_msg_varg(const mp_obj_type_t *exc_type, const char *fmt, ...) {
    // check that the given type is an exception type
    assert(exc_type->make_new == mp_obj_exception_make_new);

    // make exception object
    mp_obj_exception_t *o = m_new_obj_var(mp_obj_exception_t, mp_obj_t, 0);
    o->base.type = exc_type;
    o->traceback = MP_OBJ_NULL;
    o->args.len = 0;

    if (fmt == NULL) {
        // no message
        o->msg = NULL;
    } else {
        // render exception message
        o->msg = vstr_new();
        va_list ap;
        va_start(ap, fmt);
        vstr_vprintf(o->msg, fmt, ap);
        va_end(ap);
    }

    return o;
}

// return true if the given object is an exception type
// TODO make this work for user defined exceptions
bool mp_obj_is_exception_type(mp_obj_t self_in) {
    if (MP_OBJ_IS_TYPE(self_in, &mp_type_type)) {
        mp_obj_type_t *self = self_in;
        return self->make_new == mp_obj_exception_make_new;
    } else {
        return false;
    }
}

// return true if the given object is an instance of an exception type
// TODO make this work for user defined exceptions
bool mp_obj_is_exception_instance(mp_obj_t self_in) {
    return mp_obj_get_type(self_in)->make_new == mp_obj_exception_make_new;
}

void mp_obj_exception_clear_traceback(mp_obj_t self_in) {
    // make sure self_in is an exception instance
    assert(mp_obj_get_type(self_in)->make_new == mp_obj_exception_make_new);
    mp_obj_exception_t *self = self_in;

    // just set the traceback to the null object
    // we don't want to call any memory management functions here
    self->traceback = MP_OBJ_NULL;
}

void mp_obj_exception_add_traceback(mp_obj_t self_in, qstr file, machine_uint_t line, qstr block) {
    // make sure self_in is an exception instance
    assert(mp_obj_get_type(self_in)->make_new == mp_obj_exception_make_new);
    mp_obj_exception_t *self = self_in;

    // for traceback, we are just using the list object for convenience, it's not really a list of Python objects
    if (self->traceback == MP_OBJ_NULL) {
        self->traceback = mp_obj_new_list(0, NULL);
    }
    mp_obj_list_append(self->traceback, (mp_obj_t)(machine_uint_t)file);
    mp_obj_list_append(self->traceback, (mp_obj_t)(machine_uint_t)line);
    mp_obj_list_append(self->traceback, (mp_obj_t)(machine_uint_t)block);
}

void mp_obj_exception_get_traceback(mp_obj_t self_in, machine_uint_t *n, machine_uint_t **values) {
    // make sure self_in is an exception instance
    assert(mp_obj_get_type(self_in)->make_new == mp_obj_exception_make_new);
    mp_obj_exception_t *self = self_in;

    if (self->traceback == MP_OBJ_NULL) {
        *n = 0;
        *values = NULL;
    } else {
        uint n2;
        mp_obj_list_get(self->traceback, &n2, (mp_obj_t**)values);
        *n = n2;
    }
}

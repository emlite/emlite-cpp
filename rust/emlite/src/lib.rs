pub mod env;
use crate::env::*;
use std::ffi::CStr;

/// Runs JS eval
#[macro_export]
macro_rules! eval {
    ($src: literal) => {{
        $crate::Val::global("eval").invoke(&[$crate::Val::from_str($src)])
    }};
    ($src: literal $(, $arg:expr)* $(,)?) => {{
        $crate::Val::global("eval").invoke(
            &[$crate::Val::from_str(&format!($src, $( $arg ),*)) ]
        )
    }};
}

/// A helper macro which packs values into a slice of Val
#[macro_export]
macro_rules! argv {
    ($($rest:expr),*) => {{
        [$($crate::Val::from($rest)),*]
    }};
}

/// A wrapper around a javascript handle
#[derive(Debug)]
pub struct Val {
    inner: Handle,
}

impl Val {
    /// Takes the ownership of a handle
    pub fn take_ownership(handle: Handle) -> Val {
        Val {
            inner: handle,
        }
    }

    /// Creates a Val object from another
    pub fn from_val(v: Val) -> Self {
        unsafe {
            emlite_val_inc_ref(v.inner);
        }
        Val {
            inner: v.inner,
        }
    }

    /// Returns the globalThis object
    pub fn global_this() -> Val {
        Val::take_ownership(unsafe { emlite_val_global_this() })
    }

    /// Gets the property `prop`
    pub fn get(&self, prop: &str) -> Val {
        let h = unsafe { emlite_val_obj_prop(self.as_handle(), prop.as_ptr() as _, prop.len()) };
        Val::take_ownership(h)
    }

    /// Gets a global object by `name`
    pub fn global(name: &str) -> Val {
        Val::global_this().get(name)
    }

    /// Gets a js null Val
    pub fn null() -> Val {
        Val::take_ownership(unsafe { emlite_val_null() })
    }

    /// Gets a js undefined Val
    pub fn undefined() -> Val {
        Val::take_ownership(unsafe { emlite_val_undefined() })
    }

    /// Gets a new js object
    pub fn object() -> Val {
        Val::take_ownership(unsafe { emlite_val_new_object() })
    }

    /// Gets a new js array
    pub fn array() -> Val {
        Val::take_ownership(unsafe { emlite_val_new_array() })
    }

    /// Creates a Val from an i32
    pub fn from_i32(i: i32) -> Val {
        Val::take_ownership(unsafe { emlite_val_make_int(i) })
    }

    /// Creates a Val from an f64
    pub fn from_f64(f: f64) -> Val {
        Val::take_ownership(unsafe { emlite_val_make_double(f) })
    }

    /// Creates a Val from str
    #[allow(clippy::should_implement_trait)]
    pub fn from_str(s: &str) -> Val {
        Val::take_ownership(unsafe { emlite_val_make_str(s.as_ptr() as _, s.len()) })
    }

    /// Returns the raw js handle
    #[inline(always)]
    pub fn as_handle(&self) -> Handle {
        self.inner
    }

    /// Set the underlying js object property `prop` to `val`
    pub fn set(&self, prop: &str, val: Val) {
        unsafe {
            emlite_val_obj_set_prop(
                self.as_handle(),
                prop.as_ptr() as _,
                prop.len(),
                val.as_handle(),
            )
        };
    }

    /// Checks whether a property `prop` exists
    pub fn has(&self, prop: &str) -> bool {
        unsafe { emlite_val_obj_has_prop(self.as_handle(), prop.as_ptr() as _, prop.len()) }
    }

    /// Checks whether a non-inherited property `prop` exists
    pub fn has_own_property(&self, prop: &str) -> bool {
        unsafe { emlite_val_obj_has_own_prop(self.as_handle(), prop.as_ptr() as _, prop.len()) }
    }

    /// Gets the typeof the underlying js object
    pub fn type_of(&self) -> String {
        unsafe {
            let ptr = emlite_val_typeof(self.as_handle());
            String::from_utf8_lossy(CStr::from_ptr(ptr).to_bytes()).to_string()
        }
    }

    /// Gets the element at index `idx`. Assumes the underlying js type is indexable
    pub fn at(&self, idx: usize) -> Val {
        Val::take_ownership(unsafe { emlite_val_get_elem(self.as_handle(), idx) })
    }

    /// Gets the underlying i32 value of a js object
    pub fn as_i32(&self) -> i32 {
        unsafe { emlite_val_get_value_int(self.as_handle()) as i32 }
    }

    /// Gets the underlying boolean value of a js object
    pub fn as_bool(&self) -> bool {
        self.as_handle() > 3
    }

    /// Gets the underlying f64 value of a js object
    pub fn as_f64(&self) -> f64 {
        unsafe { emlite_val_get_value_double(self.as_handle()) as _ }
    }

    /// Gets the underlying string value of a js object
    pub fn as_string(&self) -> String {
        unsafe {
            let ptr = emlite_val_get_value_string(self.as_handle());
            String::from_utf8_lossy(CStr::from_ptr(ptr).to_bytes()).to_string()
        }
    }

    /// Converts the underlying js array to an Vec of i32
    pub fn to_vec_i32(&self) -> Vec<i32> {
        let len = self.get("length").as_i32();
        let mut v: Vec<i32> = vec![];
        for i in 0..len {
            v.push(self.at(i as _).as_i32());
        }
        v
    }

    /// Converts the underlying js array to an Vec of f64
    pub fn to_vec_f64(&self) -> Vec<f64> {
        let len = self.get("length").as_i32();
        let mut v: Vec<f64> = vec![];
        for i in 0..len {
            v.push(self.at(i as _).as_f64());
        }
        v
    }

    /// Calls the method `f` with `args`, can return an undefined js value
    pub fn call(&self, f: &str, args: &[Val]) -> Val {
        unsafe {
            let arr = Val::take_ownership(emlite_val_new_array());
            for arg in args {
                emlite_val_push(arr.as_handle(), arg.as_handle());
            }
            Val::take_ownership(emlite_val_obj_call(
                self.as_handle(),
                f.as_ptr() as _,
                f.len(),
                arr.as_handle(),
            ))
        }
    }

    /// Calls the object's constructor with `args` constructing a new object
    pub fn new(&self, args: &[Val]) -> Val {
        unsafe {
            let arr = Val::take_ownership(emlite_val_new_array());
            for arg in args {
                emlite_val_push(arr.as_handle(), arg.as_handle());
            }
            Val::take_ownership(emlite_val_construct_new(self.as_handle(), arr.as_handle()))
        }
    }

    /// Invokes the function object with `args`, can return an undefined js value
    pub fn invoke(&self, args: &[Val]) -> Val {
        unsafe {
            let arr = Val::take_ownership(emlite_val_new_array());
            for arg in args {
                emlite_val_push(arr.as_handle(), arg.as_handle());
            }
            Val::take_ownership(emlite_val_func_call(self.as_handle(), arr.as_handle()))
        }
    }

    /// Creates js function from a function pointer and returns its handle wrapped in a Val object
    pub fn make_fn(f: fn(Handle) -> Handle) -> Val {
        let idx: u32 = f as usize as u32;
        unsafe { Val::take_ownership(emlite_val_make_callback(idx)) }
    }

    /// Awaits the invoked function object
    pub fn await_(&self) -> Val {
        eval!(
            r#"
            (async () => {{
                let obj = EMLITE_VALMAP.toValue({});
                let ret = await obj;
                return EMLITE_VALMAP.toHandle(ret);
            }})()
        "#,
            self.as_handle()
        )
    }

    /// Decrements the refcount of the underlying handle
    pub fn delete(v: Val) {
        unsafe {
            emlite_val_dec_ref(v.as_handle());
        }
    }

    /// Throws a js object represented by Val
    pub fn throw(v: Val) {
        unsafe {
            emlite_val_throw(v.as_handle());
        }
    }

    /// Checks whether this Val is an instanceof `v`
    pub fn instanceof(&self, v: Val) -> bool {
        unsafe { emlite_val_instanceof(self.as_handle(), v.as_handle()) }
    }
}

impl From<i32> for Val {
    fn from(v: i32) -> Self {
        Val::from_i32(v)
    }
}

impl From<f64> for Val {
    fn from(v: f64) -> Self {
        Val::from_f64(v)
    }
}

impl From<()> for Val {
    fn from(_: ()) -> Self {
        Val::undefined()
    }
}

impl From<&str> for Val {
    fn from(item: &str) -> Self {
        Val::from_str(item)
    }
}

impl From<String> for Val {
    fn from(item: String) -> Self {
        Val::from_str(&item)
    }
}

impl Drop for Val {
    fn drop(&mut self) {
        unsafe { emlite_val_dec_ref(self.as_handle()) }
    }
}

impl Clone for Val {
    fn clone(&self) -> Val {
        unsafe { emlite_val_inc_ref(self.as_handle()); }
        Val::take_ownership(self.as_handle())
    }
}

use std::ops::{Deref, DerefMut};

/// A console wrapper
#[derive(Clone, Debug)]
pub struct Console {
    val: Val,
}

impl Console {
    /// Gets the console
    pub fn get() -> Console {
        Console {
            val: Val::global("console"),
        }
    }

    /// Logs into the console
    pub fn log(&self, args: &[Val]) {
        self.val.call("log", args);
    }

    /// console.warn
    pub fn warn(&self, args: &[Val]) {
        self.val.call("warn", args);
    }

    /// console.info
    pub fn info(&self, args: &[Val]) {
        self.val.call("info", args);
    }

    /// Returns the underlying handle of the console
    pub fn as_handle(&self) -> Handle {
        self.val.as_handle()
    }
}

impl Deref for Console {
    type Target = Val;

    fn deref(&self) -> &Self::Target {
        &self.val
    }
}

impl DerefMut for Console {
    fn deref_mut(&mut self) -> &mut Self::Target {
        &mut self.val
    }
}

impl Into<Val> for Console {
    fn into(self) -> Val {
        Val::take_ownership(self.inner)
    }
}

use std::cmp::Ordering;
use std::ops::Not;

impl PartialEq for Val {
    fn eq(&self, other: &Val) -> bool {
        unsafe { emlite_val_strictly_equals(self.as_handle(), other.as_handle()) }
    }
}

impl PartialOrd for Val {
    fn partial_cmp(&self, other: &Val) -> Option<Ordering> {
        unsafe {
            if emlite_val_strictly_equals(self.as_handle(), other.as_handle()) {
                Some(Ordering::Equal)
            } else if emlite_val_gt(self.as_handle(), other.as_handle()) {
                Some(Ordering::Greater)
            } else if emlite_val_lt(self.as_handle(), other.as_handle()) {
                Some(Ordering::Less)
            } else {
                None
            }
        }
    }
}

impl Not for Val {
    type Output = bool;

    fn not(self) -> Self::Output {
        unsafe { emlite_val_not(self.as_handle()) }
    }
}

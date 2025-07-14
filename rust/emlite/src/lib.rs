#![no_std]
extern crate alloc;

pub mod env;
use crate::env::*;
use core::ffi::CStr;
use alloc::string::String;
use alloc::string::ToString;
use alloc::boxed::Box;
use alloc::vec::Vec;
use alloc::format;

#[repr(u32)]
pub enum EmlitePredefHandles {
    Null = 0,
    Undefined,
    False,
    True,
    GlobalThis,
    Console,
    Reserved,
}

/// Runs JS eval
#[macro_export]
macro_rules! eval {
    ($src: literal) => {{
        $crate::Val::global("eval").invoke(&[$crate::Val::from($src)])
    }};
    ($src: literal $(, $arg:expr)* $(,)?) => {{
        $crate::Val::global("eval").invoke(
            &[$crate::Val::from(&format!($src, $( $arg ),*)) ]
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
    /// Returns the globalThis object
    pub const fn global_this() -> Val {
        Val { inner: EmlitePredefHandles::GlobalThis as _ }
    }

    /// Gets the property `prop`
    pub fn get<T: Into<Val>>(&self, prop: T) -> Val {
        let h = unsafe { emlite_val_get(self.as_handle(), prop.into().as_handle()) };
        Val::take_ownership(h)
    }

    /// Gets a global object by `name`
    pub fn global(name: &str) -> Val {
        Val::global_this().get(name)
    }

    /// Gets a js null Val
    pub const fn null() -> Val {
        Val { inner: EmlitePredefHandles::Null as _ }
    }

    /// Gets a js undefined Val
    pub const fn undefined() -> Val {
        Val { inner: EmlitePredefHandles::Undefined as _ }
    }

    /// Gets a new js object
    pub fn object() -> Val {
        Val::take_ownership(unsafe { emlite_val_new_object() })
    }

    /// Gets a new js array
    pub fn array() -> Val {
        Val::take_ownership(unsafe { emlite_val_new_array() })
    }

    /// Set the underlying js object property `prop` to `val`
    pub fn set<K: Into<Val>, V: Into<Val>>(&self, prop: K, val: V) {
        unsafe {
            emlite_val_set(
                self.as_handle(),
                prop.into().as_handle(),
                val.into().as_handle(),
            )
        };
    }

    /// Checks whether a property `prop` exists
    pub fn has<T: Into<Val>>(&self, prop: T) -> bool {
        unsafe { emlite_val_has(self.as_handle(), prop.into().as_handle()) }
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
    pub fn at<T: Into<Val>>(&self, idx: T) -> Val {
        Val::take_ownership(unsafe { emlite_val_get(self.as_handle(), idx.into().as_handle()) })
    }

    /// Converts the underlying js array to a Vec of V
    pub fn to_vec<V: FromVal>(&self) -> Vec<V> {
        let len = self.get("length").as_::<usize>();
        let mut v: Vec<V> = Vec::with_capacity(len);
        for i in 0..len {
            v.push(self.at::<i32>(i as _).as_::<V>());
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
    pub fn make_fn_raw(f: fn(Handle, Handle) -> Handle, data: Handle) -> Val {
        let idx: u32 = f as usize as u32;
        unsafe { Val::take_ownership(emlite_val_make_callback(idx, data)) }
    }

    /// Creates a js function from a Rust closure and returns a Val
    pub fn make_fn<F: FnMut(&[Val]) -> Val>(cb: F) -> Val {
        fn shim(args: Handle, data: Handle) -> Handle {
            let v = Val::take_ownership(args);
            let vals: Vec<Val> = v.to_vec();
            let func0 = Val::take_ownership(data);
            let a = func0.as_::<i32>() as usize as *mut Box<dyn FnMut(&[Val]) -> Val>;
            let f: &mut (dyn FnMut(&[Val]) -> Val) = unsafe { &mut **a };
            core::mem::forget(func0);
            f(&vals).as_handle()
        }
        #[allow(clippy::type_complexity)]
        let a: *mut Box<dyn FnMut(&[Val]) -> Val> = Box::into_raw(Box::new(Box::new(cb)));
        let data = Val::from(a as Handle);
        unsafe {
            emlite_val_inc_ref(data.as_handle());
        }
        Self::make_fn_raw(shim, data.as_handle())
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
    pub fn throw(v: Val) -> ! {
        unsafe {
            emlite_val_throw(v.as_handle());
        }
    }

    /// Checks whether this Val is an instanceof `v`
    pub fn instanceof(&self, v: Val) -> bool {
        unsafe { emlite_val_instanceof(self.as_handle(), v.as_handle()) }
    }

    pub fn is_number(&self) -> bool {
        unsafe { emlite_val_is_number(self.as_handle()) }
    }
    
    pub fn is_string(&self) -> bool {
        unsafe { emlite_val_is_string(self.as_handle()) }
    }

    pub fn is_null(&self) -> bool {
        self.as_handle() == EmlitePredefHandles::Null as u32
    }

    pub fn is_undefined(&self) -> bool {
        self.as_handle() == EmlitePredefHandles::Undefined as u32
    }

    pub fn is_error(&self) -> bool {
        self.instanceof(Val::global("Error"))
    }

    pub fn is_function(&self) -> bool {
        self.instanceof(Val::global("Function"))
    }

    #[inline(always)]
    pub fn as_<T>(&self) -> T
    where
        T: FromVal,
    {
        T::from_val(self)
    }
}

impl From<bool> for Val {
    fn from(v: bool) -> Self {
        Val::take_ownership(unsafe { emlite_val_make_int(v as _) })
    }
}

impl From<i8> for Val {
    fn from(v: i8) -> Self {
        Val::take_ownership(unsafe { emlite_val_make_int(v as _) })
    }
}

impl From<u8> for Val {
    fn from(v: u8) -> Self {
        Val::take_ownership(unsafe { emlite_val_make_int(v as _) })
    }
}

impl From<i16> for Val {
    fn from(v: i16) -> Self {
        Val::take_ownership(unsafe { emlite_val_make_int(v as _) })
    }
}

impl From<u16> for Val {
    fn from(v: u16) -> Self {
        Val::take_ownership(unsafe { emlite_val_make_int(v as _) })
    }
}

impl From<i32> for Val {
    fn from(v: i32) -> Self {
        Val::take_ownership(unsafe { emlite_val_make_int(v) })
    }
}

impl From<u32> for Val {
    fn from(v: u32) -> Self {
        Val::take_ownership(unsafe { emlite_val_make_int(v as _) })
    }
}

impl From<i64> for Val {
    fn from(v: i64) -> Self {
        Val::take_ownership(unsafe { emlite_val_make_int(v as _) })
    }
}

impl From<u64> for Val {
    fn from(v: u64) -> Self {
        Val::take_ownership(unsafe { emlite_val_make_int(v as _) })
    }
}

impl From<usize> for Val {
    fn from(v: usize) -> Self {
        Val::take_ownership(unsafe { emlite_val_make_int(v as _) })
    }
}

impl From<isize> for Val {
    fn from(v: isize) -> Self {
        Val::take_ownership(unsafe { emlite_val_make_int(v as _) })
    }
}

impl From<f32> for Val {
    fn from(v: f32) -> Self {
        Val::take_ownership(unsafe { emlite_val_make_double(v as _) })
    }
}

impl From<f64> for Val {
    fn from(v: f64) -> Self {
        Val::take_ownership(unsafe { emlite_val_make_double(v) })
    }
}

impl From<()> for Val {
    fn from(_: ()) -> Self {
        Val::undefined()
    }
}

impl From<&str> for Val {
    fn from(s: &str) -> Self {
        Val::take_ownership(unsafe { emlite_val_make_str(s.as_ptr() as _, s.len()) })
    }
}

impl From<String> for Val {
    fn from(s: String) -> Self {
        Val::take_ownership(unsafe { emlite_val_make_str(s.as_ptr() as _, s.len()) })
    }
}

impl From<&String> for Val {
    fn from(s: &String) -> Self {
        Val::take_ownership(unsafe { emlite_val_make_str(s.as_ptr() as _, s.len()) })
    }
}

impl Drop for Val {
    fn drop(&mut self) {
        unsafe { emlite_val_dec_ref(self.as_handle()) }
    }
}

impl Clone for Val {
    fn clone(&self) -> Val {
        unsafe {
            emlite_val_inc_ref(self.as_handle());
        }
        Val::take_ownership(self.as_handle())
    }
}

use core::ops::{Deref, DerefMut};

/// A console wrapper
#[derive(Clone, Debug)]
pub struct Console {
    val: Val,
}

impl Console {
    /// Gets the console
    pub const fn get() -> Console {
        Console {
            val: Val {
                inner: EmlitePredefHandles::Console as _,
            },
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

impl From<Console> for Val {
    fn from(val: Console) -> Self {
        Val::take_ownership(val.inner)
    }
}

use core::cmp::Ordering;
use core::ops::Not;

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

pub trait FromVal: Sized {
    /// Creates a Val object from another
    fn from_val(v: &Val) -> Self;
    /// Takes the ownership of a handle
    fn take_ownership(v: Handle) -> Self;
    /// Returns the raw js handle
    fn as_handle(&self) -> Handle;
}

impl FromVal for Val {
    fn from_val(v: &Val) -> Self {
        unsafe {
            emlite_val_inc_ref(v.inner);
        }
        Val {
            inner: v.as_handle(),
        }
    }
    fn take_ownership(v: Handle) -> Self {
        Val { inner: v }
    }
    #[inline(always)]
    fn as_handle(&self) -> Handle {
        self.inner
    }
}

impl FromVal for bool {
    fn from_val(v: &Val) -> Self {
        unsafe {
            !env::emlite_val_not(v.as_handle())
        }
    }
    fn take_ownership(v: Handle) -> Self {
        Self::from_val(&Val::take_ownership(v))
    }
    fn as_handle(&self) -> Handle {
        if *self { EmlitePredefHandles::False as u32 } else { EmlitePredefHandles::True as u32 }
    }
}

macro_rules! impl_int {
    ($($t:ty),*) => {$(
        impl FromVal for $t {
            fn from_val(v: &Val) -> Self {
                unsafe {
                    emlite_val_get_value_int(v.as_handle()) as Self
                }
            }
            fn take_ownership(v: Handle) -> Self {
                unsafe { emlite_val_get_value_int(v) as Self }
            }
            fn as_handle(&self) -> Handle {
                0
            }
        }
    )*}
}

impl_int!(i8, i16, i32, i64, isize, u8, u16, u32, u64, usize);

macro_rules! impl_float {
    ($($t:ty),*) => {$(
        impl FromVal for $t {
            fn from_val(v: &Val) -> Self {
                unsafe { emlite_val_get_value_double(v.as_handle()) as Self }
            }
            fn take_ownership(v: Handle) -> Self {
                unsafe { emlite_val_get_value_double(v) as Self }
            }
            fn as_handle(&self) -> Handle {
                0
            }
        }
    )*}
}

impl_float!(f32, f64);

impl FromVal for String {
    fn from_val(v: &Val) -> Self {
        unsafe {
            let ptr = emlite_val_get_value_string(v.as_handle());
            CStr::from_ptr(ptr).to_string_lossy().into_owned()
        }
    }
    fn take_ownership(v: Handle) -> Self {
        unsafe {
            let ptr = emlite_val_get_value_string(v);
            CStr::from_ptr(ptr).to_string_lossy().into_owned()
        }
    }
    fn as_handle(&self) -> Handle {
        0
    }
}


impl FromVal for &str {
    fn from_val(v: &Val) -> Self {
        unsafe {
            let ptr = emlite_val_get_value_string(v.as_handle());
            CStr::from_ptr(ptr).to_str().unwrap()
        }
    }
    fn take_ownership(v: Handle) -> Self {
        unsafe {
            let ptr = emlite_val_get_value_string(v);
            CStr::from_ptr(ptr).to_str().unwrap()
        }
    }
    fn as_handle(&self) -> Handle {
        0
    }
}

mod defs;
use crate::defs::*;
use std::cell::RefCell;
use std::ffi::CStr;

thread_local! {
    pub static CB_TABLE: RefCell<Vec<fn(Handle)>> = RefCell::new(Vec::new());
}

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

#[macro_export]
macro_rules! argv {
    ($($rest:expr),*) => {{
        [$($crate::Val::from($rest)),*]
    }};
}

pub struct Val {
    v_: Handle,
}

impl Val {
    pub fn from_handle(handle: Handle) -> Val {
        Val { v_: handle }
    }

    pub fn from_val(v: Val) -> Self {
        let handle = v.as_handle();
        Self { v_: handle }
    }

    pub fn global_this() -> Val {
        Val::from_handle(unsafe { emlite_val_global_this() })
    }

    pub fn get(&self, prop: &str) -> Val {
        let h = unsafe { emlite_val_obj_prop(self.v_, prop.as_ptr() as _, prop.len()) };
        Val::from_handle(h)
    }

    pub fn global(name: &str) -> Val {
        Val::global_this().get(name)
    }

    pub fn null() -> Val {
        Val::from_handle(unsafe { emlite_val_null() })
    }

    pub fn undefined() -> Val {
        Val::from_handle(unsafe { emlite_val_undefined() })
    }

    pub fn object() -> Val {
        Val::from_handle(unsafe { emlite_val_new_object() })
    }

    pub fn array() -> Val {
        Val::from_handle(unsafe { emlite_val_new_array() })
    }

    pub fn from_i32(i: i32) -> Val {
        Val::from_handle(unsafe { emlite_val_make_int(i) })
    }

    pub fn from_f64(f: f64) -> Val {
        Val::from_handle(unsafe { emlite_val_make_double(f) })
    }

    #[allow(clippy::should_implement_trait)]
    pub fn from_str(s: &str) -> Val {
        Val::from_handle(unsafe { emlite_val_make_str(s.as_ptr() as _, s.len()) })
    }

    pub fn as_handle(&self) -> Handle {
        self.v_
    }

    pub fn set(&self, prop: &str, val: Val) {
        unsafe {
            emlite_val_obj_set_prop(self.v_, prop.as_ptr() as _, prop.len(), val.as_handle())
        };
    }

    pub fn has(&self, prop: &str) -> bool {
        unsafe { emlite_val_obj_has_prop(self.v_, prop.as_ptr() as _, prop.len()) }
    }

    pub fn has_own_property(&self, prop: &str) -> bool {
        unsafe { emlite_val_obj_has_own_prop(self.v_, prop.as_ptr() as _, prop.len()) }
    }

    pub fn type_of(&self) -> String {
        unsafe {
            let ptr = emlite_val_typeof(self.v_);
            String::from_utf8_lossy(CStr::from_ptr(ptr).to_bytes()).to_string()
        }
    }

    pub fn at(&self, idx: usize) -> Val {
        Val::from_handle(unsafe { emlite_val_get_elem(self.v_, idx) })
    }

    pub fn as_i32(&self) -> i32 {
        unsafe { emlite_val_get_value_int(self.v_) as i32 }
    }

    pub fn as_bool(&self) -> bool {
        unsafe { emlite_val_get_value_int(self.v_) > 3 }
    }

    pub fn as_f64(&self) -> f64 {
        unsafe { emlite_val_get_value_double(self.v_) as _ }
    }

    pub fn as_string(&self) -> String {
        unsafe {
            let ptr = emlite_val_get_value_string(self.v_);
            String::from_utf8_lossy(CStr::from_ptr(ptr).to_bytes()).to_string()
        }
    }

    pub fn to_vec_i32(&self) -> Vec<i32> {
        let len = self.get("length").as_i32();
        let mut v: Vec<i32> = vec![];
        for i in 0..len {
            v.push(self.at(i as _).as_i32());
        }
        v
    }

    pub fn to_vec_f64(&self) -> Vec<f64> {
        let len = self.get("length").as_i32();
        let mut v: Vec<f64> = vec![];
        for i in 0..len {
            v.push(self.at(i as _).as_f64());
        }
        v
    }

    pub fn call(&self, f: &str, args: &[Val]) -> Val {
        unsafe {
            let arr = emlite_val_new_array();
            for arg in args {
                emlite_val_push(arr, arg.as_handle());
            }
            Val::from_handle(emlite_val_obj_call(self.v_, f.as_ptr() as _, f.len(), arr))
        }
    }

    pub fn new(&self, args: &[Val]) -> Val {
        unsafe {
            let arr = emlite_val_new_array();
            for arg in args {
                emlite_val_push(arr, arg.as_handle());
            }
            Val::from_handle(emlite_val_construct_new(self.v_, arr))
        }
    }

    pub fn invoke(&self, args: &[Val]) -> Val {
        unsafe {
            let arr = emlite_val_new_array();
            for arg in args {
                emlite_val_push(arr, arg.as_handle());
            }
            Val::from_handle(emlite_val_func_call(self.v_, arr))
        }
    }

    pub fn make_js_function(f: fn(Handle)) -> Val {
        let id = CB_TABLE.with_borrow_mut(move |v| {
            let id = v.len();
            v.push(f);
            id
        });
        Val::from_handle(unsafe { emlite_val_make_callback(id as u32) })
    }

    pub fn await_(&self) -> Val {
        eval!(
            r#"
            (async () => {{
                let obj = ValMap.toValue({});
                let ret = await obj;
                return ValMap.toHandle(ret);
            }})()
        "#,
            self.v_
        )
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
        Val::null()
    }
}

// impl From<&Val> for Val {
//     fn from(item: &Val) -> Self {
//         Val::from_val(item.into())
//     }
// }

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

pub struct Console {
    val: Val,
}

impl Console {
    pub fn get() -> Console {
        Console {
            val: Val::global("console"),
        }
    }

    pub fn log(&self, args: &[Val]) {
        self.val.call("log", args);
    }

    pub fn as_handle(&self) -> Handle {
        self.val.v_
    }
}

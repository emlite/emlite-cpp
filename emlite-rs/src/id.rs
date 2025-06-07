use std::any::TypeId;

pub fn get_type_id<T: 'static>() -> TypeId {
    std::any::TypeId::of::<T>()
}

pub struct GenericWireType(pub f64);

pub trait JsType {
    fn id() -> TypeId;
    fn from_generic_wire_type(v: GenericWireType) -> Self;
}

impl JsType for bool {
    fn id() -> TypeId {
        get_type_id::<bool>()
    }
    fn from_generic_wire_type(v: GenericWireType) -> Self {
        v.0 != 0f64
    }
}

macro_rules! register_rust_int {
    ($t:ty, $name:expr) => {
        impl JsType for $t {
            fn id() -> TypeId {
                get_type_id::<$t>()
            }
            fn from_generic_wire_type(v: GenericWireType) -> Self {
                v.0 as _
            }
        }
    };
}

macro_rules! register_rust_float {
    ($t:ty, $name:expr) => {
        impl JsType for $t {
            fn id() -> TypeId {
                get_type_id::<$t>()
            }
            fn from_generic_wire_type(v: GenericWireType) -> Self {
                v.0 as _
            }
        }
    };
}

register_rust_int!(u8, "rust_u8");
register_rust_int!(u16, "rust_u16");
register_rust_int!(u32, "rust_u32");
register_rust_int!(i8, "rust_i8");
register_rust_int!(i16, "rust_i16");
register_rust_int!(i32, "rust_i32");
register_rust_int!(usize, "rust_usize");
register_rust_int!(isize, "rust_isize");
register_rust_float!(f32, "rust_f32");
register_rust_float!(f64, "rust_f64");

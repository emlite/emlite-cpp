class UniqueList {
    constructor() {
        // duplicate storage gives us two‑way O(1) lookup at the cost of extra memory.
        this._items = [];
        this._index = new Map();
    }

    add(value) {
        if (this._index.has(value)) {
            return this._index.get(value);
        }
        const i = this._items.length;
        this._items.push(value);
        this._index.set(value, i);
        return i;
    }

    toHandle(value) { return this.add(value); }
    get(i) { return this._items[i]; }
    toValue(i) { return this.get(i); }
    has(value) { return this._index.has(value); }
    get size() { return this._items.length; }

    delete(value) {
        const i = this._index.get(value);
        if (i === undefined) return false;

        const lastIdx = this._items.length - 1;
        const lastVal = this._items[lastIdx];

        if (i !== lastIdx) {
            this._items[i] = lastVal;
            this._index.set(lastVal, i);
        }

        this._items.pop();
        this._index.delete(value);
        return true;
    }

    [Symbol.iterator]() { return this._items.values(); }
}

const OBJECT_MAP = new UniqueList();
globalThis.ValMap = OBJECT_MAP;
OBJECT_MAP.add(null);
OBJECT_MAP.add(undefined);
OBJECT_MAP.add(false);
OBJECT_MAP.add(true);
OBJECT_MAP.add(globalThis);

const enc = new TextEncoder('utf-8');
const dec = new TextDecoder('utf-8');

const CB_STORE = new Map();
let nextCbId = 0;

export class Emlite {
    constructor(memory) {
        this._memory = memory ?? new WebAssembly.Memory({ initial: 258, maximum: 4096 });
        this._updateViews();
    }

    _updateViews() {
        const b = this._memory.buffer;

        this._i8 = new Int8Array(b);
        this._u8 = new Uint8Array(b);
        this._i16 = new Int16Array(b);
        this._u16 = new Uint16Array(b);
        this._i32 = new Int32Array(b);
        this._u32 = new Uint32Array(b);
        this._f32 = new Float32Array(b);
        this._f64 = new Float64Array(b);

        globalThis.HEAP8 = this._i8;
        globalThis.HEAPU8 = this._u8;
        globalThis.HEAP16 = this._i16;
        globalThis.HEAPU16 = this._u16;
        globalThis.HEAP32 = this._i32;
        globalThis.HEAPU32 = this._u32;
        globalThis.HEAPF32 = this._f32;
        globalThis.HEAPF64 = this._f64;
    }

    setExports(exports) {
        this.exports = exports;
    }

    cStr(ptr, len) {
        return dec.decode(new Uint8Array(this._memory.buffer, ptr, len));
    }

    copyStringToWasm(str) {
        if (typeof this.exports.malloc !== "undefined") {
            const utf8 = enc.encode(str + "\0");
            const ptr = this.exports.malloc(utf8.length);
            if (ptr === 0) throw new Error("malloc failed in copyStringToWasm");
            new Uint8Array(this._memory.buffer).set(utf8, ptr);
            return ptr;
        } else {
            return 0;
        }
    }

    get env() {
        return {
            emlite_val_null: () => 0,
            emlite_val_undefined: () => 1,
            emlite_val_false: () => 2,
            emlite_val_true: () => 3,
            emlite_val_global_this: () => 4,

            memory: this._memory,

            __cxa_allocate_exception: () => { },
            __cxa_free_exception: () => { },
            __cxa_throw: () => { },

            emlite_val_new_array: () => OBJECT_MAP.add([]),
            emlite_val_new_object: () => OBJECT_MAP.add({}),
            emlite_val_make_int: n => OBJECT_MAP.add(n | 0),
            emlite_val_make_double: n => OBJECT_MAP.add(n),
            emlite_val_make_str: (ptr, len) => OBJECT_MAP.add(this.cStr(ptr, len)),

            emlite_val_get_value_int: n => OBJECT_MAP.get(n),
            emlite_val_get_value_double: n => OBJECT_MAP.get(n),
            emlite_val_get_value_string: n => this.copyStringToWasm(OBJECT_MAP.get(n)),
            emlite_val_typeof: n => this.copyStringToWasm(typeof OBJECT_MAP.get(n)),

            emlite_val_push: (arrRef, valRef) => OBJECT_MAP.get(arrRef).push(valRef),
            emlite_val_get_elem: (n, idx) => OBJECT_MAP.add(OBJECT_MAP.get(n)[idx]),

            emlite_val_not: arg => !OBJECT_MAP.get(arg),
            emlite_val_is_string: arg => {
                const obj = OBJECT_MAP.get(arg);
                return typeof obj === "string" || obj instanceof String;
            },
            emlite_val_is_number: arg => typeof OBJECT_MAP.get(arg) === "number",
            emlite_val_gt: (a, b) => OBJECT_MAP.get(a) > OBJECT_MAP.get(b),
            emlite_val_gte: (a, b) => OBJECT_MAP.get(a) >= OBJECT_MAP.get(b),
            emlite_val_lt: (a, b) => OBJECT_MAP.get(a) < OBJECT_MAP.get(b),
            emlite_val_lte: (a, b) => OBJECT_MAP.get(a) <= OBJECT_MAP.get(b),
            emlite_val_equals: (a, b) => OBJECT_MAP.get(a) == OBJECT_MAP.get(b),
            emlite_val_strictly_equals: (a, b) => OBJECT_MAP.get(a) === OBJECT_MAP.get(b),
            emlite_val_instanceof: (a, b) => OBJECT_MAP.get(a) instanceof OBJECT_MAP.get(b),

            emlite_val_obj_prop: (objRef, pPtr, pLen) => {
                const target = OBJECT_MAP.get(objRef);
                const prop = this.cStr(pPtr, pLen);
                return OBJECT_MAP.add(target[prop]);
            },
            emlite_val_obj_set_prop: (objRef, pPtr, pLen, val) => {
                const target = OBJECT_MAP.get(objRef);
                const prop = this.cStr(pPtr, pLen);
                target[prop] = OBJECT_MAP.get(val);
            },
            emlite_val_obj_has_prop: (objRef, pPtr, pLen) => {
                const target = OBJECT_MAP.get(objRef);
                const prop = this.cStr(pPtr, pLen);
                return Reflect.has(target, prop);
            },
            emlite_val_obj_has_own_prop: (objRef, pPtr, pLen) => {
                const target = OBJECT_MAP.get(objRef);
                const prop = this.cStr(pPtr, pLen);
                return Object.prototype.hasOwnProperty.call(target, prop);
            },

            emlite_val_delete: n => OBJECT_MAP.delete(n),
            emlite_val_throw: n => { throw OBJECT_MAP.get(n); },

            emlite_val_make_callback: fidx => {
                const id = nextCbId++;
                CB_STORE.set(id, fidx);

                const jsFn = (...args) => {
                    const arrHandle = OBJECT_MAP.add(args.map(v => OBJECT_MAP.add(v)));
                    const ret = this.exports.__indirect_function_table.get(fidx)(arrHandle);

                    OBJECT_MAP.delete(arrHandle);
                    return ret;
                };
                return OBJECT_MAP.add(jsFn);
            },

            emlite_val_obj_call: (objRef, mPtr, mLen, argvRef) => {
                const target = OBJECT_MAP.get(objRef);
                const method = this.cStr(mPtr, mLen);
                const args = OBJECT_MAP.get(argvRef).map(h => OBJECT_MAP.get(h));
                return OBJECT_MAP.add(Reflect.apply(target[method], target, args));
            },
            emlite_val_construct_new: (objRef, argvRef) => {
                const target = OBJECT_MAP.get(objRef);
                const args = OBJECT_MAP.get(argvRef).map(h => OBJECT_MAP.get(h));
                return OBJECT_MAP.add(Reflect.construct(target, args));
            },
            emlite_val_func_call: (objRef, argvRef) => {
                const target = OBJECT_MAP.get(objRef);
                const args = OBJECT_MAP.get(argvRef).map(h => OBJECT_MAP.get(h));
                return OBJECT_MAP.add(Reflect.apply(target, undefined, args));
            },
            emscripten_notify_memory_growth: (i) => this._updateViews(),
        };
    }
}

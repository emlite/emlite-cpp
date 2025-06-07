class UniqueList {
    constructor() {
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

    toHandle(value) {
        return this.add(value);
    }

    get(i) { return this._items[i]; }

    toValue(i) { return this.get(i); }

    has(value) { return this._index.has(value); }

    get size() { return this._items.length; }

    delete(value) {
        if (!this._index.has(value)) return false;
        const i = this._index.get(value);
        const last = this._items.pop();
        if (i < this._items.length) {
            this._items[i] = last;
            this._index.set(last, i);
        }
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

export class Emlite {
    constructor(memory) {
        this._memory = memory ? memory : new WebAssembly.Memory({ initial: 100 });
    }

    setExports(exports) {
        this.exports = exports;
    }

    cStr(ptr, len) {
        return dec.decode(new Uint8Array(this._memory.buffer, ptr, len));
    }

    copyStringToWasm(str) {
        const utf8 = enc.encode(str + "\0");
        const ptr = this.exports.malloc(utf8.length);
        if (ptr === 0) throw new Error('malloc failed in copyStringToWasm');
        new Uint8Array(this._memory.buffer).set(utf8, ptr);
        return ptr;
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
            emlite_val_new_object: () => OBJECT_MAP.add(new Object),
            emlite_val_push: (arrRef, valRef) => OBJECT_MAP.get(arrRef).push(valRef),
            emlite_val_make_int: (n) => OBJECT_MAP.add(n | 0),
            emlite_val_make_double: (n) => OBJECT_MAP.add(n),
            emlite_val_make_str: (ptr, len) => OBJECT_MAP.add(this.cStr(ptr, len)),
            emlite_val_get_value_int: (n) => OBJECT_MAP.get(n),
            emlite_val_get_value_double: (n) => OBJECT_MAP.get(n),
            emlite_val_get_value_string: (n) => this.copyStringToWasm(OBJECT_MAP.get(n)),
            emlite_val_typeof: (n) => this.copyStringToWasm(typeof OBJECT_MAP.get(n)),
            emlite_val_get_elem: (n, idx) => OBJECT_MAP.add(OBJECT_MAP.get(n)[idx]),
            emlite_val_not: (arg) => !OBJECT_MAP.get(arg),
            emlite_val_is_string: (arg) => {
                let obj = OBJECT_MAP.get(arg);
                typeof obj === "string" || obj instanceof String
            },
            emlite_val_is_number: (arg) => typeof OBJECT_MAP.get(arg) === "number",
            emlite_val_gt: (arg1, arg2) => OBJECT_MAP.get(arg1) > OBJECT_MAP.get(arg2),
            emlite_val_gte: (arg1, arg2) => OBJECT_MAP.get(arg1) >= OBJECT_MAP.get(arg2),
            emlite_val_lt: (arg1, arg2) => OBJECT_MAP.get(arg1) < OBJECT_MAP.get(arg2),
            emlite_val_lte: (arg1, arg2) => OBJECT_MAP.get(arg1) <= OBJECT_MAP.get(arg2),
            emlite_val_equals: (arg1, arg2) => OBJECT_MAP.get(arg1) == OBJECT_MAP.get(arg2),
            emlite_val_strictly_equals: (arg1, arg2) => OBJECT_MAP.get(arg1) === OBJECT_MAP.get(arg2),
            emlite_val_instanceof: (arg1, arg2) => OBJECT_MAP.get(arg1) instanceof OBJECT_MAP.get(arg2),
            emlite_val_delete: (n) => delete OBJECT_MAP.get(n),
            emlite_val_make_callback: (id) => {
                const fn = (event) => {
                    const evtHandle = OBJECT_MAP.add(event);
                    this.exports.wasm_invoke_cb(id, evtHandle);
                };
                return OBJECT_MAP.add(fn);
            },
            emlite_val_obj_call: (objRef, mPtr, mLen, argvRef) => {
                const target = OBJECT_MAP.get(objRef);
                const method = this.cStr(mPtr, mLen);
                const args = [];
                for (let index = 0; index < OBJECT_MAP.get(argvRef).length; index++) {
                    args.push(OBJECT_MAP.get(OBJECT_MAP.get(argvRef)[index]));
                }
                return OBJECT_MAP.add(Reflect.apply(target[method], target, args));
            },
            emlite_val_construct_new: (objRef, argvRef) => {
                const target = OBJECT_MAP.get(objRef);
                const args = [];
                for (let index = 0; index < OBJECT_MAP.get(argvRef).length; index++) {
                    args.push(OBJECT_MAP.get(OBJECT_MAP.get(argvRef)[index]));
                }
                return OBJECT_MAP.add(Reflect.construct(target, args));
            },
            emlite_val_func_call: (objRef, argvRef) => {
                const target = OBJECT_MAP.get(objRef);
                const args = [];
                for (let index = 0; index < OBJECT_MAP.get(argvRef).length; index++) {
                    args.push(OBJECT_MAP.get(OBJECT_MAP.get(argvRef)[index]));
                }
                return OBJECT_MAP.add(Reflect.apply(target, undefined, args));
            },
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
                return target.hasOwnProperty(prop);
            },
        };
    }
}
class UniqueList {
    constructor() {
        // duplicate storage gets us two-way O(1) lookup, but doubles memory.
        // however in js everything is a reference, so that should be ok!
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

const HEADER_BYTES = 8;
const ALIGN = 8;
const PAGE_SIZE = 65536;

const u32 = () => new Uint32Array(memory.buffer);
const alignUp = (x, a) => (x + a - 1) & ~(a - 1);

let freelist = 0;
let brk = alignUp(heapBase, ALIGN);

const sizeAt = off => u32()[off >>> 2];
const nextAt = off => u32()[(off >>> 2) + 1];
const setSize = (off, sz) => { u32()[off >>> 2] = sz; };
const setNext = (off, nx) => { u32()[(off >>> 2) + 1] = nx; };

const grow = bytes => {
    const shortfall = brk + bytes - memory.buffer.byteLength;
    if (shortfall > 0) {
        const pages = Math.ceil(shortfall / PAGE_SIZE);
        memory.grow(pages);
    }
};

function emlite_malloc(n) {
    n = alignUp(n, ALIGN);
    let prev = 0;
    let cur = freelist;

    while (cur !== 0 && Math.abs(sizeAt(cur)) < n) {
        prev = cur;
        cur = nextAt(cur);
    }

    if (cur !== 0) {
        const blkSize = Math.abs(sizeAt(cur));
        const remain = blkSize - n;

        if (remain > HEADER_BYTES) {
            const newFree = cur + HEADER_BYTES + n;
            setSize(newFree, -remain + HEADER_BYTES);
            setNext(newFree, nextAt(cur));
            setSize(cur, n);
            if (prev === 0) freelist = newFree; else setNext(prev, newFree);
        } else {
            if (prev === 0) freelist = nextAt(cur); else setNext(prev, nextAt(cur));
            setSize(cur, blkSize);
        }
        return cur + HEADER_BYTES;
    }

    const off = brk;
    grow(n + HEADER_BYTES);
    setSize(off, n);
    brk += HEADER_BYTES + n;
    return off + HEADER_BYTES;
}

function emlite_free(ptr) {
    if (!ptr) return;
    const blk = ptr - HEADER_BYTES;
    const sz = sizeAt(blk);

    if (sz <= 0) throw new Error("double free / corrupted block");
    setSize(blk, -sz);

    if (freelist === 0 || blk < freelist) {
        setNext(blk, freelist);
        freelist = blk;
    } else {
        let cur = freelist;
        while (nextAt(cur) !== 0 && nextAt(cur) < blk) cur = nextAt(cur);
        setNext(blk, nextAt(cur));
        setNext(cur, blk);
    }

    const nxt = nextAt(blk);
    if (nxt && blk + HEADER_BYTES + Math.abs(sizeAt(blk)) === nxt) {
        setSize(blk, -(
            Math.abs(sizeAt(blk)) +
            HEADER_BYTES +
            Math.abs(sizeAt(nxt))
        ));
        setNext(blk, nextAt(nxt));
    }

    if (freelist !== blk) {
        let cur = freelist;
        while (nextAt(cur) !== 0 && nextAt(cur) < blk) cur = nextAt(cur);
        if (cur + HEADER_BYTES + Math.abs(sizeAt(cur)) === blk) {
            setSize(cur, -(
                Math.abs(sizeAt(cur)) +
                HEADER_BYTES +
                Math.abs(sizeAt(blk))
            ));
            setNext(cur, nextAt(blk));
        }
    }
}

function emlite_realloc(ptr, n) {
    if (!ptr) return malloc(n);
    if (n === 0) { free(ptr); return 0; }

    const blk = ptr - HEADER_BYTES;
    const oldSize = Math.abs(sizeAt(blk));
    if (n <= oldSize) return ptr;

    const newPtr = malloc(n);
    new Uint8Array(memory.buffer, newPtr, n)
        .set(new Uint8Array(memory.buffer, ptr, oldSize));
    free(ptr);
    return newPtr;
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
        let alloc_fn = undefined;
        if (this.exports.malloc !== undefined) alloc_fn = this.exports.malloc; else alloc_fn = emlite_malloc;
        const ptr = alloc_fn(utf8.length);
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
            emlite_val_throw: (n) => { throw OBJECT_MAP.get(n); },
            emlite_val_make_callback: (fidx) => {
                const id = nextCbId++;
                CB_STORE.set(id, fidx);
                const jsFn = (...args) => {
                    const arrHandle = OBJECT_MAP.add(
                        args.map(v => OBJECT_MAP.add(v))
                    );
                    return this
                        .exports.__indirect_function_table
                        .get(fidx)(arrHandle);
                };
                return OBJECT_MAP.add(jsFn);
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
            emlite_malloc: emlite_malloc,
            emlite_free: emlite_free,
            emlite_realloc: emlite_realloc,
        };
    }
}
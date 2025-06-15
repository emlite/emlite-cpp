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

const HEADER_BYTES = 8;
const ALIGN = 8;
const PAGE_SIZE = 65536;
const alignUp = (x, a) => ((x + a - 1) >>> 0) & ~(a - 1);

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
        this.brk = 0;
        this.freelist = 0;
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
        this.brk = alignUp(this.exports.__heap_base.value, ALIGN);
    }

    u32() { return new Uint32Array(this._memory.buffer); }

    sizeAt(off) { return this.u32()[off >>> 2]; }
    nextAt(off) { return this.u32()[(off >>> 2) + 1]; }
    setSize(off, sz) { this.u32()[off >>> 2] = sz; }
    setNext(off, nx) { this.u32()[(off >>> 2) + 1] = nx; }

    growAndRefresh(bytes) {
        const shortfall = this.brk + bytes - this._memory.buffer.byteLength;
        if (shortfall > 0) {
            const pages = Math.ceil(shortfall / PAGE_SIZE);
            this._memory.grow(pages);
            this._updateViews();
        }
    }

    cStr(ptr, len) {
        return dec.decode(new Uint8Array(this._memory.buffer, ptr, len));
    }

    emlite_malloc(n) {
        n = alignUp(n, ALIGN);

        let prev = 0;
        let cur = this.freelist;
        while (cur !== 0 && Math.abs(this.sizeAt(cur)) < n) {
            prev = cur;
            cur = this.nextAt(cur);
        }

        if (cur !== 0) {
            const blkSize = Math.abs(this.sizeAt(cur));
            const remain = blkSize - n;

            if (remain > HEADER_BYTES) {
                const newFree = cur + HEADER_BYTES + n;
                this.setSize(newFree, -(remain - HEADER_BYTES));
                this.setNext(newFree, this.nextAt(cur));

                this.setSize(cur, n);
                if (prev === 0) {
                    this.freelist = newFree;
                } else {
                    this.setNext(prev, newFree);
                }
            } else {
                if (prev === 0) {
                    this.freelist = this.nextAt(cur);
                } else {
                    this.setNext(prev, this.nextAt(cur));
                }
                this.setSize(cur, blkSize);
            }
            return cur + HEADER_BYTES;
        }

        const off = this.brk;
        this.growAndRefresh(n + HEADER_BYTES);
        this.setSize(off, n);
        this.brk += HEADER_BYTES + n;
        return off + HEADER_BYTES;
    }

    emlite_free(ptr) {
        if (!ptr) return;
        const blk = ptr - HEADER_BYTES;
        const sz = this.sizeAt(blk);
        if (sz <= 0) throw new Error("double free / corrupted block");

        this.setSize(blk, -sz);

        if (this.freelist === 0 || blk < this.freelist) {
            this.setNext(blk, this.freelist);
            this.freelist = blk;
        } else {
            let cur = this.freelist;
            while (this.nextAt(cur) !== 0 && this.nextAt(cur) < blk) cur = this.nextAt(cur);
            this.setNext(blk, this.nextAt(cur));
            this.setNext(cur, blk);
        }

        const nxt = this.nextAt(blk);
        if (nxt && blk + HEADER_BYTES + Math.abs(this.sizeAt(blk)) === nxt && this.sizeAt(nxt) < 0) {
            this.setSize(blk, -(Math.abs(this.sizeAt(blk)) + HEADER_BYTES + Math.abs(this.sizeAt(nxt))));
            this.setNext(blk, this.nextAt(nxt));
        }

        if (this.freelist !== blk) {
            let cur = this.freelist;
            while (this.nextAt(cur) !== 0 && this.nextAt(cur) < blk) cur = this.nextAt(cur);
            if (cur + HEADER_BYTES + Math.abs(this.sizeAt(cur)) === blk && this.sizeAt(cur) < 0) {
                this.setSize(cur, -(Math.abs(this.sizeAt(cur)) + HEADER_BYTES + Math.abs(this.sizeAt(blk))));
                this.setNext(cur, this.nextAt(blk));
            }
        }
    }

    copyStringToWasm(str) {
        const utf8 = enc.encode(str + "\0");
        const allocFn = this.exports?.malloc ?? this.emlite_malloc.bind(this);
        const ptr = allocFn(utf8.length);
        if (ptr === 0) throw new Error("malloc failed in copyStringToWasm");
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

            emlite_malloc: n => this.emlite_malloc(n),
            emlite_free: ptr => this.emlite_free(ptr),
            emlite_realloc: (ptr, n) => {
                if (!ptr) return this.emlite_malloc(n);
                if (n === 0) { this.emlite_free(ptr); return 0; }

                const blk = ptr - HEADER_BYTES;
                const oldSize = Math.abs(this.sizeAt(blk));
                if (n <= oldSize) return ptr;

                const newPtr = this.emlite_malloc(n);
                new Uint8Array(this._memory.buffer, newPtr, n).set(
                    new Uint8Array(this._memory.buffer, ptr, oldSize)
                );
                this.emlite_free(ptr);
                return newPtr;
            },
            emscripten_notify_memory_growth: (i) => this._updateViews(),
        };
    }
}

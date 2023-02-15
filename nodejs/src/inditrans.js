const None = 0;
const TamilTraditional = 1;
const TamilSuperscripted = 2;
const IgnoreQuotedMarkers = 4;
const IgnoreVedicAccents = 8;
const InferAnuswara = 16;
const RetainZeroWidthChars = 32;



init() {
  const binary = fs.readFileSync('inditrans.wasm');
  WebAssembly.instantiate(binary).then(({ instance }) => {
    console.log(instance.exports.add(40, 2));
  });
}

var MeshoptDecoder = (function() {
  "use strict";

  var instance, heap;

  var env = {
    emscripten_notify_memory_growth: function(index) {
      heap = new Uint8Array(instance.exports.memory.buffer);
    }
  };

  var promise =
    (typeof fetch === 'function' ?
      fetch('data:application/octet-stream;base64,' + wasm)
      .then(response => response.arrayBuffer()) :
      Promise.resolve(Buffer.from(wasm, 'base64').buffer))
    .then(bytes => WebAssembly.instantiate(bytes, { env }))
    .then(function(result) {
      instance = result.instance;
      env.emscripten_notify_memory_growth(0);
    });

  function decode(fun, target, count, size, source) {
    var sbrk = instance.exports.sbrk;
    var tp = sbrk(count * size);
    var sp = sbrk(source.length);
    heap.set(source, sp);
    var res = fun(tp, count, size, sp, source.length);
    target.set(heap.subarray(tp, tp + count * size));
    sbrk(tp - sbrk(0));
    if (res != 0) {
      throw new Error("Malformed buffer data: " + res);
    }
  };

  return {
    ready: promise,
    decodeVertexBuffer: function(target, count, size, source) {
      decode(instance.exports.meshopt_decodeVertexBuffer, target, count, size, source);
    },
    decodeIndexBuffer: function(target, count, size, source) {
      decode(instance.exports.meshopt_decodeIndexBuffer, target, count, size, source);
    }
  };
})();

if (typeof exports === 'object' && typeof module === 'object')
  module.exports = MeshoptDecoder;
else if (typeof define === 'function' && define['amd'])
  define([], function() {
    return MeshoptDecoder;
  });
else if (typeof exports === 'object')
  exports["MeshoptDecoder"] = MeshoptDecoder;
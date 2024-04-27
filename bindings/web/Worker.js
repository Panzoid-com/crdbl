onmessage=function(d){
  var msgQueue = [];
  onmessage=(m)=>msgQueue.push(m);
  d=d.data;
  d["instantiateWasm"]=(info,receiveInstance)=>{var instance=new WebAssembly.Instance(d["wasm"],info);receiveInstance(instance,d["wasm"]);return instance.exports};import(d.js).then(Module => {
    onmessage=null;
    d["queuedMessages"]=msgQueue;
    Module.default(d);
    d.wasm=d.mem=d.js=0;
  });};

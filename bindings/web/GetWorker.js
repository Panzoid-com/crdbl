Module["getWorker"] = function (id) {
  return _wasmWorkers[id];
}

_wasmWorkers[0] = typeof self !== 'undefined' ? self : globalThis;

if (Module["queuedMessages"]) {
  Module["queuedMessages"] = Module["queuedMessages"].forEach(
    _wasmWorkerDelayedMessageQueue ? _wasmWorkerAppendToQueue : _wasmWorkerRunPostMessage);
}

// These functions are implemented in js because parsing is a lot faster this way
// and we avoid copying strings to wasm memory
function applyWrappers() {
  const parseNodeId = function (nodeId) {
    const nodeId_parsed = nodeId.split(",");
    return [parseInt(nodeId_parsed[0]), parseInt(nodeId_parsed[1]), parseInt(nodeId_parsed[2])];
  }

  Module.ProjectDB.getChildId = (rootNodeId, offset) => {
    const lastIndex = rootNodeId.lastIndexOf(',');
    const prefix = rootNodeId.substring(0, lastIndex + 1);
    const childId = parseInt(rootNodeId.substring(lastIndex + 1));
    return prefix + (childId + offset).toString();
  };

  Module.ProjectDB.getChildOffset = (nodeId) => {
    const lastIndex = nodeId.lastIndexOf(',');
    return parseInt(nodeId.substring(lastIndex + 1));
  }

  const NULLID = Module.ProjectDB.NULLID();
  Module.ProjectDB.NULLID = () => NULLID;
  const ROOTID = Module.ProjectDB.ROOTID();
  Module.ProjectDB.ROOTID = () => ROOTID;
  const INITID = Module.ProjectDB.INITID();
  Module.ProjectDB.INITID = () => INITID;
  const PENDINGID = Module.ProjectDB.PENDINGID();
  Module.ProjectDB.PENDINGID = () => PENDINGID;

  Module.ProjectDB.prototype.getNodeChildren = function(nodeId, includePending)
  {
    if (includePending === undefined)
    {
      includePending = false;
    }

    const nodeId_parsed = parseNodeId(nodeId);
    return this.getNodeChildren_raw(
      nodeId_parsed[0], nodeId_parsed[1], nodeId_parsed[2],
      includePending);
  };

  Module.ProjectDB.prototype.getNode = function(nodeId)
  {
    const nodeId_parsed = parseNodeId(nodeId);
    return this.getNode_raw(nodeId_parsed[0], nodeId_parsed[1], nodeId_parsed[2]);
  }

  Module.ProjectDB.prototype.getNodeValue = function(nodeId)
  {
    const nodeId_parsed = parseNodeId(nodeId);
    return this.getNodeValue_raw(nodeId_parsed[0], nodeId_parsed[1], nodeId_parsed[2]);
  }

  Module.ProjectDB.prototype.getNodeBlockValue = function(nodeId)
  {
    const nodeId_parsed = parseNodeId(nodeId);
    return this.getNodeBlockValue_raw(nodeId_parsed[0], nodeId_parsed[1], nodeId_parsed[2]);
  }
}

if (Module["ProjectDB"])
{
  applyWrappers();
}
else
{
  Module["onRuntimeInitialized"] = function () {
    applyWrappers();
  };
}

# crdbl (**C**onflict-free **R**eplicated **D**ata**b**ase **L**ibrary)

`crdbl` ("credible") is a CRDT-based database library built for collaborative editing, version control, and beyond. It offers a rich set unique of features that provide a foundation for building innovative, data-driven applications.

## Demo

[Try a basic online demo](https://Panzoid-com.github.io/crdbl) showing a few db features with a text editor.

## Key Features

- **Operation-based Architecture**: `crdbl` works by applying primitive operations like `createNode()`, `addChild()`, `insertText()`, `setValue()`, etc. to make changes to the database. These operations can be filtered, modified, and combined using stream-like abstractions.
- **Built-in Data Structures**: Support for various primitive node types, including maps, lists, sets, fixed-size value types (int, float, etc.), UTF-8 strings, binary blobs, and references.
- **Typing System**: `crdbl`'s typing system supports type inheritance chains, allowing for application-specific types to efficiently reuse common data patterns and schemas.
- **Efficient Concurrency**: Efficiently merge operations from concurrent editors, enabling robust real-time multi-user editing and syncing with very low overhead.
- **Flexible Serialization**: `crdbl` supports multiple data serialization formats with an extensible design.
- **Lightweight Snapshots**: Save and load database states at arbitrary points in time using lightweight snapshots (vector clocks) with operation-level granularity.
- **Selective Undo/Redo**: Efficiently undo or redo any specific operation in the project history, allowing for flexible and unique editing patterns.
- **Operation Filters**: Use operation filter streams to selectively toggle operations by timestamp, user, arbitrary tags, etc., enabling classic VCS-style branches and more advanced patterns like independent project layers.
- **Multithreading**: Optional multithreading support via web workers, allowing different workers to share a database instance for multithreaded web apps.

## Installation

To install `crdbl`, you can use npm:

```bash
npm i @panzoid/crdbl
```

See examples for basic usage.

## Examples
Explore examples in the `examples/` directory to see some minimal examples of common functionality.

1. Install dependencies:
```bash
cd examples
npm i
```

2. Run an example:
```bash
node serialization/index.js
```

Additionally, you may find the tests useful for understanding the database API.

## Building from Source

### Prerequisites

- Install [Emscripten](https://emscripten.org/docs/getting_started/downloads.html) to compile the project for WebAssembly.
- To build native artifacts for the library or tests, only CMake and a C++20 compiler is required.

### Building

1. Create a build directory in the repository root:

```bash
mkdir build
cd build
```

2. Build the WASM wrapper using `emcmake` and `emmake`:
```bash
emcmake cmake ..
emmake make ProjectDBWeb
```

3. Run the `fixbuild.sh` script from the `bindings/web/` directory to make final adjustments to the build artifacts:

```bash
cd ../bindings/web
./fixbuild.sh
```

The final package files will be output to `bindings/web/dist/`.

## Used By
`crdbl` is actively used in [Panzoid Gen4](https://app.panzoid.com), an online video editor that leverages many of the database's unique features.

## Contributing
Contributions are welcome! If you're looking to contribute, please review the current issues and pull requests to see if your issue has already been addressed or discussed.

## License
This project is licensed under Apache License 2.0. See the `LICENSE` file for more details.
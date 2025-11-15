# Topaz
The Topaz programming language. Memory safety and speed!

See source code documentation in [here](https://samirshef.github.io/topazlang/)

## Usage
```bash
topazc path/to/src.tp
```

Where `path/to/src.tp` is relative path to source

## Compiler options
1) `--tokens` - printing parsed tokens as `<type> : '<value>' (<column>/<line>)`
2) `--ir` - printing generated LLVM IR code
3) `--obj` - compiling source to object file
4) `--path` - compiling source to executable into passed after this option path (for example: `topazc source.tp --path build/main`)
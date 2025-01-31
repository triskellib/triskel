# triskel-bench

Lays out CFGs without displaying them. Useful for benchmarking.

## Usage

It can be used on a file, it will lay out each function in the LLVM module and generate a CSV with statistics.

```
$ triskel-bench <llvm bytecode>
```

It can also be used on a single function.

```
$ triskel-bench <llvm bytecode> <function name>
```
### Description

The `netcode` library is a C++11 [network coding](https://en.wikipedia.org/wiki/Linear_network_coding) library using [Galois fields](https://en.wikipedia.org/wiki/Finite_field). C interfaces are also provided.

### Requirements

- gcc >= 4.7 or clang >= 3.3
- gf-complete (https://github.com/ceph/gf-complete)
- cmake >= 3.10
- boost 1.68

### Building and installing

Basic commands:

```
$ mkdir build && cd build
$ cmake ..
$ make && make install
```

Configuring gf-complete path:

```$ cmake -DGF_COMPLETE_ROOT=...```

Configuring compiler to use:

```$ cmake -DCMAKE_CXX_COMPILER=... -DCMAKE_C_COMPILER=...```

### Testing

Launching tests:

``` ./tests/tests ```

Enabling code coverage (GCC only):

```$ cmake -DCOVERAGE=1```


### Documentation

If a `doxygen` executable has been found, the documentation can be generated:

```$ make doc```

Internal documentation can be generated if an option is given to cmake:

```$ cmake -DINTERNAL_DOC=1```

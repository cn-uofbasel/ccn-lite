# CCN-lite for Unix

## Prerequisites

CCN-lite requires OpenSSL. Use the following to install it:
* Ubuntu: `sudo apt-get install libssl-dev cmake`
* macOS: `brew install openssl cmake`

## Installation

1.  Clone the repository:
    ```bash
    git clone https://github.com/cn-uofbasel/ccn-lite
    ```

2.  Set environment variable `$CCNL_HOME` and add the binary folder of CCN-lite to your `$PATH`:
    ```bash
    export CCNL_HOME="`pwd`/ccn-lite"
    export PATH=$PATH:"$CCNL_HOME/bin"
    ```

    To make these variables permanent, add them to your shell's `.rc` file, e.g. `~/.bashrc`.

3.  Build CCN-lite using `cmake`:
    ```bash
    cd $CCNL_HOME
    mkdir build && cd build
    cmake ..
    make
    ```
    On macOS (with OpenSSL installed by Homebrew):
    ```bash
    cmake -DOPENSSL_ROOT_DIR=/usr/local/Cellar/openssl/1.0.2l/ \
    -DOPENSSL_LIBRARIES=/usr/local/Cellar/openssl/1.0.2l/lib \
    -DOPENSSL_INCLUDE_DIR=/usr/local/Cellar/openssl/1.0.2l/include ../src
    ```

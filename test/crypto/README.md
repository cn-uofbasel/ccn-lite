# ccn-lite/test/crypto -- test files for crypto functionality

Files in this directory:

```
README.md     -- this file

1.ccntlv      -- HMAC256 signed packets for CCNx1.0 encoding
2.ccntlv
3.ccntlv

keys.txt      -- keys (base64 encoded), see below
```

Use the ccn-lite-valid program to test these signed packets.

## Keys

Keys are collected in a file ```keys.txt```, one key per line,
encoded in base64.

Three keys are currently in this file:
```
26 bytes:
"abcdefghijklmnopqrstuvwxyz"

32 bytes:
"abcdefghijklmnopqrstuvwxyz012345"

64 bytes:
"abcdefghijklmnopqrstuvwxyz012345abcdefghijklmnopqrstuvwxyz012345"
```

// eof

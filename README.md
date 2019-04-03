# p2p-network
Dang p2p protocol

```
gcc src/* -o alice -pthread

./alice 127.0.0.1 2222 127.0.0.1 2500 log
```

#### Usage:
```
./alice s_ip s_port peer_ip peer_port logfile

logfile can be replaced with stdout
To sync     -- _sync
To download -- filename
```

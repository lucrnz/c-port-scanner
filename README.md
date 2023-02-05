# Port Scanner
A multithreaded port scanner that uses sockets to scan a range of ports on a target IP.

Disclaimer: This is a toy project, I don't intend to give support for it.

## Usage
`./pscanner <ip address> <start_port> <end_port> <timeout>`

- `ip address`: the IP address to scan
- `start_port`: the first port in the range to scan
- `end_port`: the last port in the range to scan
- `timeout`: the number of seconds to wait for a response before timing out

## Build
```
make pscanner
```

## Example
```
./pscanner 127.0.0.1 1 65535 30
```

## Output
The program outputs the status of a port, only if its open. `Port %number% -- OPEN` will appear on the standard output.

## Copying

Read the file `LICENSE` for more information.
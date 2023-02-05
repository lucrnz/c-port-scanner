# Port Scanner
A multithreaded port scanner that uses sockets to scan a range of ports on a target IP.

## Usage
`./pscanner <ip address> <start_port> <end_port> <timeout>`

- `ip address`: the IP address to scan
- `start_port`: the first port in the range to scan
- `end_port`: the last port in the range to scan
- `timeout`: the number of seconds to wait for a response before timing out

## Build
make
gcc -pthread pscanner.c -o pscanner

## Example
./pscanner 127.0.0.1 1 65535 1

## Output
The program outputs the status of each port: either `open` or `closed`. If a port is open, it also outputs `--- FOUND OPEN PORT ---` to indicate a potential security risk.

## Copying

Read the file `LICENSE` for more information.
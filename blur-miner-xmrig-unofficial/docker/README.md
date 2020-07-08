### To build

`docker build -t blur-xmrig .`

### To run

`docker run -it -v /dev/hugepages:/dev/hugepages --privileged --cap-add=ALL blur-xmrig`

By default it mines to my node and wallet.

### To configure

`docker run -it -v /dev/hugepages:/dev/hugepages --privileged --cap-add=ALL blur-xmrig <node IP>:<node port> <blur wallet address>`

Example:

`docker run -it -v /dev/hugepages:/dev/hugepages --privileged --cap-add=ALL blur-xmrig foo.bar.com:12345 bQoiwdfjhgwirehgvuiowrhbvowruetyhgvowuretygvowreutygvwieurthgyiwreutyhruruhiu`

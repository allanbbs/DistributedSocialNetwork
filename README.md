# Distributed Social Network

Authors

1. Allan Sousa (up201800149@edu.fe.up.pt)
2. Breno Pimentel (up201800170@edu.fe.up.pt)
3. Diogo Gomes (up201806572@edu.fe.up.pt)

### Prerequisites

 - C++17, Python3

 - Boost.Asio:
https://www.boost.org/doc/libs/1_78_0/doc/html/boost_asio.html

 - PySide6 (QT for Python):
https://pypi.org/project/PySide6/

 - CMake:
https://cmake.org/download/

 - Operational System:
The project was only tested in Linux. Altough the libraries used are cross-platform, there is a single system call (to open the interface) , and compatibility of this system call with other systems was not tested.

### How to use

The following command will create the executables in the build folder:
```
cd build
cmake ..
cmake --build . --target proj2
```
### Usage:

```
-l,  --local_endpoint       Define the local endpoint
-b,  --bootstrap            Uses this peer as a bootstrap
-be, --bootstrap_endpoint   Define an endpoint to join an existing network
```

**Note** Any peer can serve as a bootstrap for another peer when the latter is joining the network, the flag `-b` means that this peer will start the network by itself.

### Usage Example:

To start a bootstrap peer:
```
./program -l 127.0.0.1 3331 -b
```

Join the network using the bootstrap endpoint:
```
./program -l 127.0.0.1 3332 -be 127.0.0.1 3331
```

 - The video available [here](doc/sdle.mp4) also has the examples of usage to test functionality.

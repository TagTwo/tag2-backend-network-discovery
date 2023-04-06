# TagTwo Network Discovery
[![Build Status](https://travis-ci.org/tagtwo/tagtwo-network-discovery.svg?branch=master)](https://travis-ci.org/tagtwo/tagtwo-network-discovery)
[![Coverage Status](https://coveralls.io/repos/github/tagtwo/tagtwo-network-discovery/badge.svg?branch=master)](https://coveralls.io/github/tagtwo/tagtwo-network-discovery?branch=master)
[![License](https://img.shields.io/badge/license-MIT-blue.svg)](

tagtwo-network-discovery is a service discovery solution designed to help you manage, monitor, and communicate with services in your distributed application. It consists of a controller, a C++ library with Python bindings, and a web-based monitoring service.

## Directory Structure
* `controller`: A Python application responsible for reading messages from the RabbitMQ queue (service-discovery) and sending messages to all active services on the "amqp.topic" with the routing key "service-answer". To install, run pip install -r requirements.txt.

* `service`: A C++ library with pybind11 bindings to support both C++ and Python services. Services should use this library for communication and management.
To install for Python services, run pip install ..
To install for C++ services, use add_subdirectory in your project and add tagtwo-service-discovery as a link target.

* `web`: A web service for monitoring service availability in your distributed application.

## Controller
The controller is responsible for reading messages from the RabbitMQ queue and forwarding them to all active services using the "amqp.topic" exchange type with the routing key "service-answer". To install the required dependencies for the controller, run:
```bash
pip install -r requirements.txt
```

## Service
The service library is a C++ library with Python bindings, enabling both C++ and Python services to utilize it for communication and management. To install the service library, follow the instructions below:


### For Python Services
Run the following command to install the service library:
```bash
pip install .
```

### For C++ Services
In your project's CMakeLists.txt, add the tagtwo-service-discovery as a subdirectory and link the target:

```cmake
add_subdirectory(path/to/tagtwo-service-discovery)
...
target_link_libraries(your_target PRIVATE tagtwo-service-discovery)
```

### Web
The web service is designed to monitor service availability in your distributed application. This service provides a user-friendly interface for observing and managing your services.

### License
This project is licensed under the terms of the MIT License, so you better not share it with anyone :>
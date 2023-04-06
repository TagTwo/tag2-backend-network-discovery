//
// Created by per on 4/6/23.
//
#include "TagTwo/Networking/NetworkService.h"
#include <pybind11/pybind11.h>
#include <pybind11/stl.h>
#define STRINGIFY(x) #x
#define MACRO_STRINGIFY(x) STRINGIFY(x)


namespace py = pybind11;

PYBIND11_MODULE(tagtwo_network_discovery_python, m) {
    using namespace TagTwo::Networking;

    py::class_<NetworkService>(m, "NetworkService")
            .def(py::init<std::string, std::string, int, int, bool>())
            .def("update_metadata", &NetworkService::update_metadata)
            .def("update_heartbeat", &NetworkService::update_heartbeat)
            .def("last_heartbeat", &NetworkService::last_heartbeat)
            .def("time_difference", &NetworkService::time_difference)
            .def("is_expired", &NetworkService::is_expired);

    py::class_<RabbitMQListener>(m, "RabbitMQListener")
            .def(py::init<std::string, std::vector<std::string>, int, bool>())
            .def("get_channel", &RabbitMQListener::get_channel)
            .def("get_existing_services", &RabbitMQListener::get_existing_services)
            .def("connect", &RabbitMQListener::connect)
            .def("generateUUID", &RabbitMQListener::generateUUID)
            .def("enable_heartbeat", &RabbitMQListener::enable_heartbeat)
            .def("disable_heartbeat", &RabbitMQListener::disable_heartbeat)
            .def("add_metadata", &RabbitMQListener::add_metadata);

    #ifdef VERSION_INFO
        m.attr("__version__") = MACRO_STRINGIFY(VERSION_INFO);
    #else
        m.attr("__version__") = "dev";
    #endif

}

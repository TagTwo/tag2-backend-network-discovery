//
// Created by per on 4/6/23.
//
#include "TagTwo/Networking/ServiceDiscoveryClient.h"
#include "TagTwo/Networking/ServiceDiscoveryRecord.h"
#include "pybind11_json/pybind11_json.hpp"
#include <pybind11/pybind11.h>
#include <pybind11/stl.h>
#define STRINGIFY(x) #x
#define MACRO_STRINGIFY(x) STRINGIFY(x)


using namespace TagTwo::Networking;
namespace py = pybind11;

PYBIND11_MODULE(tagtwo_network_discovery_python, m) {

    m.doc() = "TagTwo Network Discovery Python Bindings";

    py::class_<ServiceDiscoveryRecord>(m, "ServiceDiscoveryRecord")
            .def(py::init<std::string, std::string, int, int, bool>(),
                 py::arg("serviceUID"),
                 py::arg("serviceType"),
                 py::arg("heartbeat_timeout"),
                 py::arg("last_heartbeat"),
                 py::arg("debug"))
            .def("update_metadata", &ServiceDiscoveryRecord::update_metadata, py::arg("metadata"))
            .def("update_heartbeat", &ServiceDiscoveryRecord::update_heartbeat, py::arg("last_heartbeat"))
            .def("last_heartbeat", &ServiceDiscoveryRecord::last_heartbeat)
            .def("get_service_uid", &ServiceDiscoveryRecord::get_service_uid)
            .def("get_service_type", &ServiceDiscoveryRecord::get_service_type)
            .def("get_metadata", &ServiceDiscoveryRecord::get_metadata)
            .def("get_metadata_json", &ServiceDiscoveryRecord::get_metadata_json)
            .def("get_heartbeat_timeout", &ServiceDiscoveryRecord::get_heartbeat_timeout)
            .def("is_expired", &ServiceDiscoveryRecord::is_expired);

    py::class_<ServiceDiscoveryClient>(m, "ServiceDiscoveryClient")
            .def(py::init<std::string, std::string, std::string, int, int, int, bool>(),
                 py::arg("serviceName"),
                 py::arg("report_queue"),
                 py::arg("answer_routing_key"),
                 py::arg("heartbeat_timeout"),
                 py::arg("heartbeat_interval"),
                 py::arg("service_check_interval"),
                 py::arg("_debug"))
            .def("get_channel", &ServiceDiscoveryClient::get_channel)
            .def("get_existing_services", &ServiceDiscoveryClient::get_existing_services)
            .def("get_service_id", &ServiceDiscoveryClient::get_service_id)
            .def("connect", &ServiceDiscoveryClient::connect,
                 py::arg("host"),
                 py::arg("port"),
                 py::arg("username"),
                 py::arg("password"),
                 py::arg("vhost") = "")
            .def_static("generateUUID", &ServiceDiscoveryClient::generateUUID, py::arg("n_digits") = 32)
            .def("enable_heartbeat", &ServiceDiscoveryClient::enable_heartbeat)
            .def("disable_heartbeat", &ServiceDiscoveryClient::disable_heartbeat)
            .def("add_metadata", &ServiceDiscoveryClient::add_metadata, py::arg("key"), py::arg("data"));
    #ifdef VERSION_INFO
        m.attr("__version__") = MACRO_STRINGIFY(VERSION_INFO);
    #else
        m.attr("__version__") = "dev";
    #endif

}

//
// Created by per on 4/6/23.
//
#include "TagTwo/Networking/ServiceDiscovery/ServiceDiscoveryClient.h"
#include "TagTwo/Networking/ServiceDiscovery/ServiceDiscoveryRecord.h"
#include "TagTwo/Networking/Queue/NNGClient.h"
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
            .def(py::init<std::string, std::string, std::string, int, int, int, std::string, bool>(),
                 py::arg("serviceName"),
                 py::arg("report_queue"),
                 py::arg("answer_routing_key"),
                 py::arg("heartbeat_timeout"),
                 py::arg("heartbeat_interval"),
                 py::arg("service_check_interval"),
                 py::arg("service_id"),
                 py::arg("_debug"))
            .def("get_channel", &ServiceDiscoveryClient::get_channel)
            .def("get_existing_services", &ServiceDiscoveryClient::get_existing_services)
            .def("get_services", static_cast<std::vector<std::shared_ptr<TagTwo::Networking::ServiceDiscoveryRecord>> (ServiceDiscoveryClient::*)()>(&ServiceDiscoveryClient::get_services))
            .def("get_services", static_cast<std::vector<std::shared_ptr<TagTwo::Networking::ServiceDiscoveryRecord>> (ServiceDiscoveryClient::*)(std::string)>(&ServiceDiscoveryClient::get_services))
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
            .def("add_metadata_dict", &ServiceDiscoveryClient::add_metadata_dict, py::arg("key"), py::arg("data"))
            .def("add_metadata_str_list", &ServiceDiscoveryClient::add_metadata_str_list, py::arg("key"), py::arg("data"))
            .def("add_metadata_str", &ServiceDiscoveryClient::add_metadata_str, py::arg("key"), py::arg("data"))
            .def("add_metadata_int", &ServiceDiscoveryClient::add_metadata_int, py::arg("key"), py::arg("data"))
            .def("add_metadata_float", &ServiceDiscoveryClient::add_metadata_float, py::arg("key"), py::arg("data"));


    py::enum_<NNGClientType>(m, "NNGClientType")
            .value("PUSH", NNGClientType::PUSH)
            .value("REQ", NNGClientType::REQ)
            .value("PUB", NNGClientType::PUB)
            .export_values();

    py::class_<NNGClient>(m, "NNGClient")
            .def(py::init<std::string, int, NNGClientType, std::size_t, std::size_t>(),
                 py::arg("host"),
                 py::arg("port"),
                 py::arg("type"),
                 py::arg("reconnect_interval") = 5000,
                 py::arg("max_reconnect_failures") = 5)
            .def("isConnected", &NNGClient::isConnected)
            .def("listen", &NNGClient::listen)
            .def("connect", &NNGClient::connect)
            .def("getFailCounter", &NNGClient::getFailCounter)
            .def("getNngMaxFailCount", &NNGClient::getNngMaxFailCount)
            .def("getHost", &NNGClient::getHost)
            .def("getPort", &NNGClient::getPort)
            .def("send", &NNGClient::send)
            .def("receive", &NNGClient::receive);



    #ifdef VERSION_INFO
        m.attr("__version__") = MACRO_STRINGIFY(VERSION_INFO);
    #else
        m.attr("__version__") = "dev";
    #endif

}

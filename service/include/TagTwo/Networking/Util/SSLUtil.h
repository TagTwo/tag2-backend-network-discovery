//
// Created by per on 4/10/23.
//

#ifndef TAGTWO_NETWORK_DISCOVERY_SSLUTIL_H
#define TAGTWO_NETWORK_DISCOVERY_SSLUTIL_H
#include <openssl/rsa.h>
#include <openssl/x509.h>
#include <openssl/pem.h>
#include <openssl/bn.h>
#include <openssl/evp.h>
#include <openssl/dh.h>
#include <boost/asio/ssl/context.hpp>
namespace TagTwo::Networking::Util::SSLUtil {



    EVP_PKEY* generate_tmp_dh_params();
    EVP_PKEY* generate_private_key();
    X509* generate_self_signed_certificate(EVP_PKEY* pkey);
    void init_ssl(boost::asio::ssl::context& _ssl_context);




}

#endif //TAGTWO_NETWORK_DISCOVERY_SSLUTIL_H

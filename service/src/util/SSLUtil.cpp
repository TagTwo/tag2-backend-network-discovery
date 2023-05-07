//
// Created by per on 4/10/23.
//

#include <spdlog/spdlog.h>
#include "TagTwo/Networking/Util/SSLUtil.h"



EVP_PKEY *TagTwo::Networking::Util::SSLUtil::generate_tmp_dh_params() {
    EVP_PKEY_CTX* pctx = EVP_PKEY_CTX_new_id(EVP_PKEY_DH, nullptr);
    if (!pctx) {
        // Handle error
        return nullptr;
    }

    if (EVP_PKEY_paramgen_init(pctx) != 1) {
        // Handle error
        EVP_PKEY_CTX_free(pctx);
        return nullptr;
    }

    if (EVP_PKEY_CTX_set_dh_paramgen_prime_len(pctx, 2048) != 1) {
        // Handle error
        EVP_PKEY_CTX_free(pctx);
        return nullptr;
    }

    EVP_PKEY* params = nullptr;
    if (EVP_PKEY_paramgen(pctx, &params) != 1) {
        // Handle error
        EVP_PKEY_CTX_free(pctx);
        return nullptr;
    }

    EVP_PKEY_CTX_free(pctx);
    return params;
}

EVP_PKEY *TagTwo::Networking::Util::SSLUtil::generate_private_key() {
    EVP_PKEY_CTX* pctx = EVP_PKEY_CTX_new_id(EVP_PKEY_RSA, nullptr);
    if (!pctx) {
        // Handle error
        return nullptr;
    }

    if (EVP_PKEY_keygen_init(pctx) != 1) {
        // Handle error
        EVP_PKEY_CTX_free(pctx);
        return nullptr;
    }

    if (EVP_PKEY_CTX_set_rsa_keygen_bits(pctx, 2048) != 1) {
        // Handle error
        EVP_PKEY_CTX_free(pctx);
        return nullptr;
    }

    EVP_PKEY* pkey = nullptr;
    if (EVP_PKEY_keygen(pctx, &pkey) != 1) {
        // Handle error
        EVP_PKEY_CTX_free(pctx);
        return nullptr;
    }

    EVP_PKEY_CTX_free(pctx);
    return pkey;
}

X509 *TagTwo::Networking::Util::SSLUtil::generate_self_signed_certificate(EVP_PKEY *pkey) {
    X509* x509 = X509_new();
    if (!x509) {
        // Handle error
        return nullptr;
    }

    ASN1_INTEGER_set(X509_get_serialNumber(x509), 1);

    X509_gmtime_adj(X509_get_notBefore(x509), 0);
    X509_gmtime_adj(X509_get_notAfter(x509), 31536000L); // 1 year

    X509_set_pubkey(x509, pkey);

    X509_NAME* name = X509_get_subject_name(x509);

    // Replace these fields with your own values
    X509_NAME_add_entry_by_txt(name, "C", MBSTRING_ASC, (const unsigned char*)"US", -1, -1, 0);
    X509_NAME_add_entry_by_txt(name, "O", MBSTRING_ASC, (const unsigned char*)"YourOrganization", -1, -1, 0);
    X509_NAME_add_entry_by_txt(name, "CN", MBSTRING_ASC, (const unsigned char*)"YourCommonName", -1, -1, 0);

    X509_set_issuer_name(x509, name);

    if (X509_sign(x509, pkey, EVP_sha256()) == 0) {
        // Handle error
        X509_free(x509);
        return nullptr;
    }

    return x509;
}


void TagTwo::Networking::Util::SSLUtil::init_ssl(boost::asio::ssl::context& _ssl_context) {


    // Set SSL context options
    _ssl_context.set_options(
            boost::asio::ssl::context::default_workarounds |
            boost::asio::ssl::context::no_sslv2 |
            boost::asio::ssl::context::single_dh_use
    );

    // Generate private key and self-signed certificate
    EVP_PKEY* pkey = generate_private_key();
    X509* x509 = generate_self_signed_certificate(pkey);

    // Convert the private key to PEM format
    BIO *bio_pkey = BIO_new(BIO_s_mem());
    PEM_write_bio_PrivateKey(bio_pkey, pkey, nullptr, nullptr, 0, nullptr, nullptr);

    // Get the size of the private key PEM data and read it into a vector
    int pkey_pem_size = BIO_pending(bio_pkey);
    std::vector<unsigned char> pkey_pem_data(pkey_pem_size);
    BIO_read(bio_pkey, pkey_pem_data.data(), pkey_pem_size);
    BIO_free(bio_pkey);

    // Convert the X509 certificate to DER format
    int x509_der_size = i2d_X509(x509, nullptr);
    std::vector<unsigned char> x509_der_data(x509_der_size);
    unsigned char* x509_der_data_ptr = x509_der_data.data();
    i2d_X509(x509, &x509_der_data_ptr);

    // Use the certificate and private key in the SSL context
    _ssl_context.use_certificate(
            boost::asio::const_buffer(x509_der_data.data(), x509_der_size), boost::asio::ssl::context::file_format::asn1);
    _ssl_context.use_private_key(
            boost::asio::const_buffer(pkey_pem_data.data(), pkey_pem_size), boost::asio::ssl::context::file_format::pem);


    // Generate a temporary Diffie-Hellman key exchange parameters

    EVP_PKEY* dh_params = generate_tmp_dh_params();
    if (dh_params) {
        // Convert the DH parameters to PEM format
        BIO *bio_dh = BIO_new(BIO_s_mem());
        PEM_write_bio_Parameters(bio_dh, dh_params);

        // Get the size of the DH parameters PEM data and read it into a vector
        int dh_pem_size = BIO_pending(bio_dh);
        std::vector<unsigned char> dh_pem_data(dh_pem_size);
        BIO_read(bio_dh, dh_pem_data.data(), dh_pem_size);
        BIO_free(bio_dh);

        _ssl_context.use_tmp_dh(boost::asio::const_buffer(dh_pem_data.data(), dh_pem_size));
        EVP_PKEY_free(dh_params);
    } else {
        // Handle error (e.g., log a warning message)
        SPDLOG_ERROR("Failed to generate temporary DH parameters");
    }

    // Don't forget to free the resources
    EVP_PKEY_free(pkey);
    X509_free(x509);
    //EVP_PKEY_free(dh_params);

}


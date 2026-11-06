#pragma once
#include <iostream>
#include <fstream>

#include <vector>
#include <string>

#ifndef BYTE
#define BYTE unsigned char
#endif


enum SCRIP_TYPE {
    SCP_NULL = 0,
    SCP_FIELD_INVERSE,  // modular inverse
    SCP_FIELD_BASIC,    // modular +,-,* on 2 numbers
    SCP_ECC_ADD,        // ecc point add in modular P
    SCP_ECC_MUL,        // ecc point scalar multiply in modular P
    //SCP_SHA256,
};

using namespace std;
using namespace BigInt;
using namespace GLLIB;

using TEST_STR_VEC = std::vector<std::vector<std::string>>;

template <typename T>
struct ECC_PARAM {
    T* P;
    T* A;
    T* B;
    T* N;
    T* Gx;
    T* Gy;
};

template <typename T>
struct ECC_POINT {
    T x;
    T y;
};

bool parse_test_script(int script_type, const char* scriptFile, int query_start, int query_cases,
    std::vector<std::vector<std::string>>& test_vec);

void run_gl_test_mod_inv(TEST_STR_VEC& test_vec);
void run_gl_test_mod_ops(TEST_STR_VEC& test_vec);
void run_gl_test_ecc_add(TEST_STR_VEC& test_vec);
void run_gl_test_ecc_scl(TEST_STR_VEC& test_vec);


template<typename T>
T mod_sub(T& a, T& b, T& N);

template<typename T>
T mod_add(T& a, T& b, T& N);

template<typename T>
T mod_mult(T& a, T& b, T& N);

template<typename T>
T mod_divide(T& a, T& b, T& N);

template<typename T>
T mod_inv(T& a, T& N);

template<typename T>
ECC_POINT<T> ecc_point_add(ECC_PARAM<T>& ecc, T& px, T& py, T& qx, T& qy);

template<typename T>
ECC_POINT<T> ecc_point_double(ECC_PARAM<T>& ecc, T& px, T& py);

template<typename T>
ECC_POINT<T> ecc_point_scale(ECC_PARAM<T>& ecc, T& px, T& py, T& k);

template<typename T>
bool ecdsa_verify_sig(ECC_PARAM<T>& ecc, T& pubx, T& puby, T& r, T& s, T& msg);

template<typename T>
bool ecdsa_sign(ECC_PARAM<T>& ecc, T& priv, T& msg, T& r, T& s);

// for verify
//bigint mod_sub(bigint& a, bigint& b, bigint& N);
//bigint mod_add(bigint& a, bigint& b, bigint& N);
//bigint mod_mult(bigint& a, bigint& b, bigint& N);
//bigint mod_divide(bigint& a, bigint& b, bigint& N);
//bigint mod_inv(bigint& a, bigint& N);
//ECC_POINT<bigint> ecc_point_add(ECC_PARAM<bigint>& ecc, bigint& px, bigint& py, bigint& qx, bigint& qy);

string hex_to_dec_string(string &&n);
string normalize_hex_string(string& n);
string normalize_hex_string(string&& n);


// sha256 definitions
// implementation from: https://lucidar.me/en/dev-c-cpp/sha-256-in-c-cpp/
#include <stddef.h>
#include <stdint.h>

#define SHA256_HEX_SIZE (64 + 1)
#define SHA256_BYTES_SIZE 32

/*
 * Compute the SHA-256 checksum of a memory region given a pointer and
 * the size of that memory region.
 * The output is a hexadecimal string of 65 characters.
 * The last character will be the null-character.
 */
void sha256_hex(const void* src, size_t n_bytes, char* dst_hex65);

void sha256_bytes(const void* src, size_t n_bytes, void* dst_bytes32);

typedef struct sha256 {
    uint32_t state[8];
    uint8_t buffer[64];
    uint64_t n_bits;
    uint8_t buffer_counter;
} sha256;

/* Functions to compute streaming SHA-256 checksums. */
void sha256_init(struct sha256* sha);
void sha256_append(struct sha256* sha, const void* data, size_t n_bytes);
void sha256_finalize_hex(struct sha256* sha, char* dst_hex65);
void sha256_finalize_bytes(struct sha256* sha, void* dst_bytes32);

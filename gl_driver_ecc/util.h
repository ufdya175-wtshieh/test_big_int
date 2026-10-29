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
    // Gx, Gy...
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
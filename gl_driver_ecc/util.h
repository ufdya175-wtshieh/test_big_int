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

using TEST_STR_VEC = std::vector<std::vector<std::string>>;

bool parse_test_script(int script_type, const char* scriptFile, int query_start, int query_cases,
    std::vector<std::vector<std::string>>& test_vec);

void run_gl_test_mod_inv(TEST_STR_VEC& test_vec);
void run_gl_test_mod_ops(TEST_STR_VEC& test_vec);
void run_gl_test_ecc_add(TEST_STR_VEC& test_vec);
void run_gl_test_ecc_scl(TEST_STR_VEC& test_vec);

using namespace std;
using namespace BigInt;
using namespace GLLIB;

glbigint mod_sub(glbigint& a, glbigint& b, glbigint& N);
glbigint mod_add(glbigint& a, glbigint& b, glbigint& N);
glbigint mod_mult(glbigint& a, glbigint& b, glbigint& N);
glbigint mod_divide(glbigint& a, glbigint& b, glbigint& N);
glbigint mod_inv(glbigint& a, glbigint& N);

// for verify
bigint mod_sub(bigint& a, bigint& b, bigint& N);
bigint mod_add(bigint& a, bigint& b, bigint& N);
bigint mod_mult(bigint& a, bigint& b, bigint& N);
bigint mod_divide(bigint& a, bigint& b, bigint& N);
bigint mod_inv(bigint& a, bigint& N);
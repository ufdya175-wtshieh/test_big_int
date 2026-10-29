// gl_driver_ecc.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include <iostream>
#include <string>
#include <vector>
#include <array>
#include "bigint.h"
#include "GlBigInt.h"
#include "util.h"

#include <stdlib.h> // rand

using namespace std;
using namespace BigInt;
using namespace GLLIB;


#define ARRAY_SIZE(a)  (sizeof(a)/sizeof(a[0]))

// ECC params
string PRIME = "0xffffffff00000001000000000000000000000000ffffffffffffffffffffffff";
string A     = "0xffffffff00000001000000000000000000000000fffffffffffffffffffffffc";
string B     = "0x5AC635D8AA3A93E7B3EBBD55769886BC651D06B0CC53B0F63BCE3C3E27D2604B";
string ORDER = "0xFFFFFFFF00000000FFFFFFFFFFFFFFFFBCE6FAADA7179E84F3B9CAC2FC632551"; // ecc order, prime?
string Gx    = "0x6B17D1F2E12C4247F8BCE6E563A440F277037D812DEB33A0F4A13945D898C296";
string Gy    = "0x4FE342E2FE1A7F9B8EE7EB4A7C0F9E162BCE33576B315ECECBB6406837BF51F5";

// test vector of big number string
TEST_STR_VEC g_vec_inv;
TEST_STR_VEC g_vec_ops;
TEST_STR_VEC g_vec_add; // ECC point add
TEST_STR_VEC g_vec_scl; // ECC point scalar multiply

/*
*  ROADMAP
*   - Modular:
*       - inverse
*           - inverse with mod 'sx' term; Q: max -sx space requirement?
*       - '+, -, 1/x, *'
*   - ECC basic op:
*       - point add
*       - point mult
*           - dev HEX-big-int for shift-add scalar algo
*   - ECDSA:
*       - verify sign (all MOD=N ops)
*       - signing (=>need Point Add/Mult)
*/

template<typename T>  // only support glbigint, glhint
T Gl_BigInt_run_eclidean_mod(T& a, T& n);

void test_ecc_ops() {
    int i;

    // test ECC point addition
    string Px = "0xb18d33dd457f646ac75ba62cd2d21cb5922aae58f5aafa09b78d5bc5816194ee";
    string Py = "0xe94df3e38e041e91220463803ca8821824e0a8f882ea7c1a5ec97571c0a0cc7b";
    string Qx = "0xab7eee3a60797b28b911afb6e0e423e01b144fe71ba236d77dd6ada1718c5d39";
    string Qy = "0x0b6041cf92686dd3ae0ed9d7ba05815684a7cfc300a2a0848a3ba12bd68ccf8b";
    string Rx = "0x9e1bb8bb0dd32eff2dd4bc634c35b2bfc990a2df75eca6273c2158a31ee6a0e3";
    string Ry = "0xf8b9dd1df9a15847f782464a2a2a9e589aa72641ceff728bde64d51c081849f3";

    bigint big_px = Px;
    bigint big_py = Py;
    bigint big_qx = Qx;
    bigint big_qy = Qy;
    bigint big_P = PRIME;
    bigint big_A = A;
    bigint big_N = ORDER;
    ECC_POINT<bigint> big_pt;
    ECC_PARAM<bigint> ecc{ &big_P, &big_A, NULL, &big_N };

    big_pt = ecc_point_add<bigint>(ecc, big_px, big_py, big_qx, big_qy);
    std::cout << "Rx:" << big_pt.x << endl;
    std::cout << "Ry:" << big_pt.y << endl;

    glbigint gl_px = Px;
    glbigint gl_py = Py;
    glbigint gl_qx = Qx;
    glbigint gl_qy = Qy;
    glbigint gl_P = PRIME;
    glbigint gl_A = A;
    glbigint gl_N = ORDER;
    ECC_POINT<glbigint> gl_pt;
    ECC_PARAM<glbigint> gl_ecc{ &gl_P, &gl_A, NULL, &gl_N };

    gl_pt = ecc_point_add<glbigint>(gl_ecc, gl_px, gl_py, gl_qx, gl_qy);
    std::cout << "Rx:" << gl_pt.x << endl;
    std::cout << "Ry:" << gl_pt.y << endl;
}
void test_gl_hex_num() {
    glhint h1 = "0x450DCF";
    glhint h2 = "0x10005AF8";
    glhint h3;
    h3 = h1 + h2;
    cout << "+:" << h3 << endl;

    h1 = "98712366647555666";
    h2 = "9997771112234512234";
    h3 = h1 + h2;
    cout << "+:" << h3 << endl;
}
template<typename T>  // only support glbigint, glhint
T Gl_BigInt_run_eclidean_mod(T& a, T& n) {
    T sx = 1;
    T tx = 0;
    T rx, q, r;
    T A = a;
    T B = n;
    array<T, 2> ret;
    int idx = 0;
    string tmp;
    const bool is_gl_hex_type = std::is_same<T, glhint>::value;

    while (B > 0) {
        //q = A / B;
        //r = A - q * B;
        ret = T::divide(A, B);
        q = ret[0];
        r = ret[1];
        
#if 0
        if (false && is_gl_hex_type) {
            // use glbigint to transform deci base & display?
            // cout << "is hex!" << endl;
            tmp = hex_to_dec_string(string(A));
            cout << setw(4) << idx << " A:" << tmp << endl;
            tmp = hex_to_dec_string(string(B));
            cout << setw(4) << idx << " B:" << tmp << endl;

            tmp = hex_to_dec_string(string(q));
            cout << setw(4) << idx << " q:" << tmp << endl;
            tmp = hex_to_dec_string(string(r));
            cout << setw(4) << idx << " r:" << tmp << endl;
            tmp = hex_to_dec_string(string(sx));
            cout << setw(4) << idx << " s:" << tmp << endl;
            tmp = hex_to_dec_string(string(tx));
            cout << setw(4) << idx << " t:" << tmp << endl;
        }
        else {
            cout << setw(4) << idx << " A:" << A << endl;
            cout << setw(4) << idx << " B:" << B << endl;
            cout << setw(4) << idx << " q:" << q << endl;
            cout << setw(4) << idx << " r:" << r << endl;
            cout << setw(4) << idx << " s:" << sx << endl;
            cout << setw(4) << idx << " t:" << tx << endl;
        }
#endif

        rx = sx - q * tx;
        sx = tx;
        tx = rx;

        A = B;
        B = r;
        idx++;
    }

    if (sx < 0) {
        //printf("negative!\n");
        sx += n;
    }

    return sx;
}

template<typename T>
array<T, 2> BigInt_run_eclidean_mod(T& a, T& n) {
    T sx = 1;
    T tx = 0;
    T rx, q, r;
    T A = a;
    T B = n;
    int idx = 0;
    //cout << " A:" << A << endl;
    //cout << " B:" << B << endl;

    while (B > 0) {
        q = A / B;
        r = A - q * B;
        //cout << idx << " q:" << q << endl;
        //cout << idx << " r:" << r << endl;
        //cout << idx << " s:" << sx << endl;
        //cout << idx << " t:" << tx << endl;

        rx = sx - q * tx;
        //cout << idx << " _:" << rx << endl;
        sx = tx;
        tx = rx;

        A = B;
        B = r;
        idx++;
    }

    if (sx < 0)
        sx += n;

    return { sx, 0 };
}
template<typename T>

void BigInt_test_euclidean() {
    vector<array<T, 2>> tests;
    /*int py_tests[][2] = {
        { 240, 17},
        { 17150,  59 },
        { 4050,  47 },
        { 70, 41 },
        { 5145, 13},
        { 28, 41 },
        {9450, 11},
        {140, 23},
        {50, 97},
        {22050, 47}
    };

    // gen tests
    for (auto& item : py_tests) {
        tests.push_back({ item[0], item[1] });
    }*/

    // ECC test set
    T x = "0x2ea5943825f72124fb398b50d33ab176685162dc3995f0b39128edb108d07de4";
    T N = ORDER;
    tests.push_back({ x, N });

    //x = "0xf3057bdf6e374d831dca2904e9388039c1f19c98273e8c5cb17150384acc679e";
    //tests.push_back({ x, N });

    for (auto& item : tests) {
        auto res = BigInt_run_eclidean_mod<T>(item[0], item[1]);
        cout << item[0] << "," << item[1] << "=> inv =" << res[0] << endl;
        // verify?
        res[0] = item[0] * res[0];
        res[0] = res[0] % item[1];
        cout << " verify: " << res[0] << endl;
    }
}

void read_gl_test_script() {
    const char* fname_inv = R"(D:\MyDoc\Projects\GL3270_FW_sign\IKV_git\ikvtool\IKVSample\EccShaTestVector\test_vector\ECC_fieldInverse.test)";
    const char* fname_ops = R"(D:\MyDoc\Projects\GL3270_FW_sign\IKV_git\ikvtool\IKVSample\EccShaTestVector\test_vector\ECC_modular.test)";
    const char* fname_add = R"(D:\MyDoc\Projects\GL3270_FW_sign\IKV_git\ikvtool\IKVSample\EccShaTestVector\test_vector\ECC_pointAdd.test)";
    const char* fname_mul = R"(D:\MyDoc\Projects\GL3270_FW_sign\IKV_git\ikvtool\IKVSample\EccShaTestVector\test_vector\ECC_scalarMult.test)";

    auto print_test_vec = [](TEST_STR_VEC& test_vec) {
        size_t i, j;
        for (i = 0; i < test_vec.size(); i++) {
            for (j = 0; j < test_vec[i].size(); j++) {
                printf(" [%d][%d]: %s\n", (int)i, (int)j, test_vec[i][j].c_str());
            }
            printf("\n");
        }
    };

    // total = 10000 tests for each script
    //parse_test_script(SCP_FIELD_INVERSE, fname_inv, 40, 10, g_vec_inv);
    //print_test_vec(g_vec_inv);
    parse_test_script(SCP_FIELD_BASIC, fname_ops, 0, 10, g_vec_ops);
    //print_test_vec(g_vec_ops);
    parse_test_script(SCP_ECC_ADD, fname_add, 0, 10, g_vec_add);
    parse_test_script(SCP_ECC_MUL, fname_mul, 0, 10, g_vec_scl);
}
void run_gl_test_mod_inv(TEST_STR_VEC &test_vec) {
    // Y = X^(-1) mod N
    if (test_vec.size() == 0) {
        printf("No INV data, skip\n");
        return;
    }

    int i;
    glbigint gl_N = ORDER;
    glbigint gl_x1, gl_x2;

    glhint gl_hN = ORDER;
    glhint gl_h1, gl_h2;

    bigint big_N = ORDER;
    bigint big_x1;
    array<bigint, 2> big_ret;

    for (i = 0; i < (int)test_vec.size(); i++){
        // [0]:X [1]:Y=X^-1
        //gl_x1 = test_vec[i][0];
        //gl_x2 = Gl_BigInt_run_eclidean_mod(gl_x1, gl_N);
        //cout << setw(4) << i << ",gl:" << gl_x2 << endl;

        gl_h1 = test_vec[i][0];
        gl_h2 = Gl_BigInt_run_eclidean_mod(gl_h1, gl_hN);
        //cout << setw(4) << i << ", gh:" << gl_h2 << endl;
        //cout << setw(4) << i << ",ans:" << test_vec[i][1] << endl;
        {   // ans may contain leading 0's, but always 64 chars
            string gl = (string)gl_h2;
            string& ans = test_vec[i][1];
            gl.erase(0, 2);
            ans.erase(0, 2);
            size_t pos = ans.find_first_not_of('0', 0);
            if (pos != string::npos)
                ans.erase(0, pos);

            bool ok = gl == ans;
            cout << setw(4) << i << (ok ? " test ok" : " test fail") << endl;
        }

        //big_x1 = test_vec[i][0];
        //big_ret = BigInt_run_eclidean_mod(big_x1, big_N);
        //cout << setw(4) << i << ",bi:" << big_ret[0] << endl;

        //cout << endl;
    }
}
void run_gl_test_mod_ops(TEST_STR_VEC& test_vec) {
    // vec[i][5] = {x1, x2, z1, z2, z3}
    // z1 = x1 + x2
    // z2 = x1 - x2
    // z3 = x1 * x2, all mod N
    if (test_vec.size() == 0) {
        printf("No OPS data, skip\n");
        return;
    }

    int i = 0;
    bool ok = false;
    string s1, s2;
    glbigint gl_N = ORDER;
    glbigint gl_x1, gl_x2, gl_z1, gl_z2, gl_z3;

    glhint gh_N = ORDER;
    glhint gh_x1, gh_x2, gh_z1, gh_z2, gh_z3;

    bigint big_N = ORDER;
    bigint big_x1, big_x2, big_z1, big_z2, big_z3;

    const bool test_add = true;
    const bool test_sub = true;
    const bool test_mul = true;

    for (i = 0; i < (int)test_vec.size(); i++) {
        ok = true;
        gh_x1 = test_vec[i][0];
        gh_x2 = test_vec[i][1];

        if (test_add) {
            gh_z1 = gh_x1 + gh_x2;
            gh_z1 %= gh_N;
            s1 = normalize_hex_string(test_vec[i][2]);
            s2 = string(gh_z1);
            if (s1 != s2)
            {
                ok = false;
                cout << setw(4) << i << ",gh+:" << s1 << endl;
                cout << setw(4) << i << ",ans:" << s2 << endl;
            }

            //gl_x1 = test_vec[i][0];
            //gl_x2 = test_vec[i][1];
            //gl_z1 = gl_x1 + gl_x2;
            //gl_z1 %= gl_N;
            //
            //big_x1 = test_vec[i][0];
            //big_x2 = test_vec[i][1];
            //big_z1 = big_x1 + big_x2;
            //big_z1 %= big_N;
            //if (string(big_z1) != string(gl_z1)) {
            //    ok = false; 
            //    cout << setw(4) << i << ",gl+:" << gl_z1 << endl;
            //    cout << setw(4) << i << ",bi+:" << big_z1 << endl;
            //}
        }
        
        if (test_sub) {
            gh_z1 = gh_x1 - gh_x2;
            if (gh_z1 < 0)
                gh_z1 += gh_N;
            s1 = normalize_hex_string(test_vec[i][3]);
            s2 = string(gh_z1);
            if (s1 != s2)
            {
                ok = false;
                cout << setw(4) << i << ",gh-:" << s1 << endl;
                cout << setw(4) << i << ",ans:" << s2 << endl;
            }

            //gl_z1 = gl_x1 - gl_x2;
            //if (gl_z1 < 0)
            //    gl_z1 += gl_N;
            //big_z1 = big_x1 - big_x2;
            //if (big_z1 < 0)
            //    big_z1 += big_N;
            //if (string(gl_z1) != string(big_z1)) {
            //    ok = false;
            //    cout << setw(4) << i << ",gl-:" << gl_z1 << endl;
            //    cout << setw(4) << i << ",bi-:" << big_z1 << endl;
            //}
        }

        if (test_mul) {
            gh_z1 = gh_x1 * gh_x2;
            gh_z1 %= gh_N;
            s1 = normalize_hex_string(test_vec[i][4]);
            s2 = string(gh_z1);
            if (s1 != s2)
            {
                ok = false;
                cout << setw(4) << i << ",gh*:" << s1 << endl;
                cout << setw(4) << i << ",ans:" << s2 << endl;
            }
            //gl_z1 = gl_x1 * gl_x2;
            //gl_z1 %= gl_N;
            //big_z1 = big_x1 * big_x2;
            //big_z1 %= big_N;
            //if (string(gl_z1) != string(big_z1)) {
            //    ok = false;
            //    cout << setw(4) << i << ",gl*:" << gl_z1 << endl;
            //    cout << setw(4) << i << ",bi*:" << big_z1 << endl;
            //}
        }
        
        cout << setw(4) << i << (ok ? " test ok" : " test fail") << endl;
    }
}
void run_gl_test_ecc_add(TEST_STR_VEC& test_vec) {
    // test vector C(x, y) = A(x, y) + B(x, y)
    // vec[i][6] = {Ax, Ay, Bx, By, Cx, Cy}
    int i;

    glhint h_ax, h_ay, h_bx, h_by, h_cx, h_cy;
    glhint h_P = PRIME;
    glhint h_A = A;
    glhint h_N = ORDER;
    ECC_POINT<glhint> h_pt;
    ECC_PARAM<glhint> ecc_h{ &h_P, &h_A, NULL, &h_N };

    for (i = 0; i < (int)test_vec.size(); i++) {
        h_ax = test_vec[i][0];
        h_ay = test_vec[i][1];
        h_bx = test_vec[i][2];
        h_by = test_vec[i][3];

        h_pt = ecc_point_add<glhint>(ecc_h, h_ax, h_ay, h_bx, h_by);
        cout << setw(4) << i << ",grx:" << h_pt.x << endl;
        cout << setw(4) << i << ",gry:" << h_pt.y << endl;
        cout << setw(4) << i << ",anx:" << test_vec[i][4] << endl;
        cout << setw(4) << i << ",any:" << test_vec[i][5] << endl;
    }
}
void test_gl_bigint() {
    string n1 = "0x5750cc858ff413078f4b5da45e06f0a26e92b152f06879f82628b3e3760216031234";
    string n2 = "0x575078f4b5da45e06f0a26e92b152f06879f82628b3e376021603123477FFAABBCC01234";
   // string n2 = "0x40b84f7b886f39d6db864e37602e1bf4819000da794b4e63b1ae2ed2754060585678";
    string n3 = "0xFFFFFFFF00000000FFFFFFFFFFFFFFFFBCE6FAADA7179E84F3B9CAC2FC632551";
    string n4 = "0x547824886f39d";

    glbigint x1 = n1;
    glbigint x2 = n2;
    glbigint x3, x4;
    array<glbigint, 2> qr;
    x4 = n4;
    //x3 = x1 + x2;
    //cout << "+:" << x3 << endl;
    //x3 = x2 - x1;
    //cout << "-:" << x3 << endl;
    x3 = x2 * x4;
    cout << "*:" << x3 << endl;
    x3 = x2 / x4;
    cout << "/:" << x3 << endl;
    qr = glbigint::divide(x2, x4);
    cout << "q:" << qr[0] << endl;
    cout << "r:" << qr[1] << endl;

    bigint big1, big2, big3, big4;
    big1 = n1;
    big2 = n2;
    big4 = n4;
    //big3 = big1 + big2;
    //cout << "+:" << big3 << endl;
    //big3 = big2 - big1;
    //cout << "-:" << big3 << endl;
    big3 = big2 * big4;
    cout << "*:" << big3 << endl;
    big3 = big2 / big4;
    cout << "/:" << big3 << endl;
    big3 = big2 % big4;
    cout << "r:" << big3 << endl;

    glbigint x = "0x2ea5943825f72124fb398b50d33ab176685162dc3995f0b39128edb108d07de4";
    glbigint N = ORDER;
    x3 = Gl_BigInt_run_eclidean_mod(x, N);
    x4 = x3 * x;
    x4 = x4 % N;
    cout << " inv: " << x3 << endl;
    cout << " ->verify: " << x4 << endl;

    //x = "0xf3057bdf6e374d831dca2904e9388039c1f19c98273e8c5cb17150384acc679e";
    //tests.push_back({ x, N });
}

void debug_gl_int() {
    string t = "618112719502521962";
    string s = "-476388062129637389";
    string q = "1";
    string r = "13034429460984717302892898619004831460792164549273279953732";

    glbigint x1, x2, gt, gs, gq, gr;
    gt = t;
    gs = s;
    gq = q;
    gr = r;
    x1 = gq * gt;
    x2 = gs - x1;
    cout << "x1:" << x1 << endl;
    cout << "x2:" << x2 << endl;

    bigint b1, b2, bt, bs, bq, br;
    bt = t;
    bs = s;
    bq = q;
    br = r;
    //b1 = bs - bq * bt;
    b1 = bq * bt;
    b2 = bs - b1;
    cout << "b1:" << b1 << endl;
    cout << "b2:" << b2 << endl;
}
void debug_gl_int2() {
    // A/B=q,r error
    string sA = "115792089210356248762697446949407573529996955224135760342422259061068512044369";
    string sB = "3955100810131797683854636165629278411113263614927463269251570925909193695736";
    // q = 29
    // r = 1094165716534115930912998146158499607712310391239325534126702209701894868025
    //  or,0x
    glhint a = sA;
    glhint b = sB;
    glhint q, r;
    array<glhint, 2> ret;
    string tmp;

    ret = glhint::divide(a, b);


    cout << "q:" << ret[0] << endl;
    cout << "r:" << ret[1] << endl;
    tmp = hex_to_dec_string(string(ret[1]));
    cout << "_:" << tmp << " (deci)\n:";

    bigint b_a = sA;
    bigint b_b = sB;
    bigint b_q, b_r;

    b_q = b_a / b_b;
    b_r = b_a % b_b;
    cout << "q:" << b_q << endl;
    cout << "r:" << b_r << endl;

    int kkk = 0;
}
void debug_gl_hint() {
    // test vector 48 fail
    // bug: in parsing '0x00..', need to remove leading 0's after '0x'
    string data_in = "0x08be81fa83ea7b9038c86ca41d4860ec3f3d7993a77f49cc19d00424bebaa1f8";
    string data_out = "0x0315cfa0f16b37041c74ba4d04c29c6f037f1d344e76584fc144828ea0913aba";

    glbigint gl_N = ORDER;
    glbigint gl_x1, gl_x2;

    glhint gl_hN = ORDER;
    glhint gl_h1, gl_h2;


    gl_x1 = data_in;
    gl_x2 = Gl_BigInt_run_eclidean_mod(gl_x1, gl_N);
    cout << setw(4) << ",gl:" << gl_x2 << endl;

    gl_h1 = data_in;
    gl_h2 = Gl_BigInt_run_eclidean_mod(gl_h1, gl_hN);
    cout << setw(4) << ",gh:" << gl_h2 << endl;
}

int main()
{
#if 0
    bigint big1, big2, ans;
    big1 = "0x5750cc858ff413078f4b5da45e06f0a26e92b152f06879f82628b3e376021603";
    big2 = "0x40b84f7b886f39d6db864e37602e1bf4819000da794b4e63b1ae2ed275406058";
    
    ans = big1 + big2;
    ans = ans % ORDER;
    //big2 = "1234567890";

    cout << "big1:" << big1 << endl;
    cout << "big2:" << big2 << endl;
    std::cout << "(a+b)%N:" << ans << "\n";

    big1 = "0x2ea5943825f72124fb398b50d33ab176685162dc3995f0b39128edb108d07de4";
    big2 = "0x97fe26fc0ee827bfe7d50ba5f39445c9a4fbf44f036dcccca752003405d34f02";
    ans = big1 * big2;
    ans = ans % ORDER;
    cout << "(a*b)%N:" << ans << endl;
#endif

#if 0
    /* big1: [1, 5], MS.B. endian
    *  big2: [900000000]
    *  ans: [900000004, 500000000]
    */
    big1 = "1000000005";
    big2 = "900000000";
    ans = big1 * big2;
    cout << "test" << ans << endl;
#endif

    read_gl_test_script();

    //run_gl_test_mod_inv(g_vec_inv);
    //run_gl_test_mod_ops(g_vec_ops);
    run_gl_test_ecc_add(g_vec_add);
 
    //test_gl_bigint();
    test_ecc_ops();
    //test_gl_hex_num();

    //printf("using bigint:\n");
    //BigInt_test_euclidean<bigint>();

    //printf("using GL Int:\n");
    //BigInt_test_euclidean<glhint>();

    // ********* debugging session ********
    //debug_gl_int();
    //debug_gl_int2();
    //debug_gl_hint();
}

// Run program: Ctrl + F5 or Debug > Start Without Debugging menu
// Debug program: F5 or Debug > Start Debugging menu

// Tips for Getting Started: 
//   1. Use the Solution Explorer window to add/manage files
//   2. Use the Team Explorer window to connect to source control
//   3. Use the Output window to see build output and other messages
//   4. Use the Error List window to view errors
//   5. Go to Project > Add New Item to create new code files, or Project > Add Existing Item to add existing code files to the project
//   6. In the future, to open this project again, go to File > Open > Project and select the .sln file

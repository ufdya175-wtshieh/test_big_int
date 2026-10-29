
#include <stdio.h>
#include <iostream>
#include <string>
#include <vector>
#include <array>

#include "bigint.h"
#include "GlBigInt.h"
#include "util.h"

using namespace std;
using namespace BigInt;
using namespace GLLIB;

bool parse_test_script(int script_type, const char* scriptFile, int query_start, int query_cases, 
        std::vector<std::vector<std::string>>& test_vec) {
    /* parse scriptFile, to get from line 'query_start', for max of 'query_len' cases
    *  query_cases = 0 for parsing all cases (may upto 100000 cases)
    * 
    *  output:
    *   test_vec[cases][in, out numbers; according to test type]
    */
    //const int VEC_CHAR_NUM = _CURVE_SIZE_IN_BYTE_;
    std::ifstream fs;
    std::string line;
    int n_in = 0, n_out = 0, n_all = 0;
    int parse_value = 0;
    int i, vector_id = 0;
    int ti = 0, tn = 0;
    const char* s, * p;

    //BYTE nums[6][32]{ 0 };
    //BYTE* nums[6]{ 0 };
    vector< vector<BYTE> > nums(6, vector<BYTE>(32, 0));
    const int MAX_SHA_DATA = 256; // max sha256 input data 200 bytes
    int sha_buf_size = 0;

    fs.open(scriptFile, ios::in);
    if (!fs.is_open()) {
        printf("%s() can't open file %s\n", __FUNCTION__, scriptFile);
        return false;
    }

    switch (script_type) {
    case SCP_FIELD_INVERSE:
        n_in = 1;
        n_out = 1;
        break;
    case SCP_FIELD_BASIC: //x,y,+,-,*
        n_in = 2;
        n_out = 3;
        break;
    case SCP_ECC_ADD:
        n_in = 4;
        n_out = 2;
        break;
    case SCP_ECC_MUL:
        n_in = 3;
        n_out = 2;
        break;
    //case SCP_SHA256:
    //    //max test data = 200 bytes
    //    n_in = 1;
    //    nums[0].resize(MAX_SHA_DATA);
    //    n_out = 1;
    //    break;
    default:
        return false;
    };
    n_all = n_in + n_out;

    auto is_valid_hex_str = [](int script_type, int i, string& line) -> bool {
        //if (script_type == SCP_SHA256 && i == 0) {
        //    // allow any printable(32~127) for sha data?
        //    return true;
        //}
        if (line.size() != 64)
            return false;
        for (auto& c : line)
            if (!((c >= '0' && c <= '9') || (c >= 'a' && c <= 'f') || (c >= 'A' && c <= 'F')))
                return false;
        return true;
    };
    // str[0]=msb, num[0]=lsb
    auto convert_hex_rev = [](string& line, BYTE num[32]) {
        int i;
        char buf[4]{ 0 };
        for (i = 0; i < 32; i++) {
            buf[0] = line[i * 2];
            buf[1] = line[i * 2 + 1];
            num[31 - i] = (BYTE)strtol(buf, NULL, 16);
        }
    };

    ti = tn = 0;
    parse_value = 0;
    test_vec.resize(query_cases);
    int ln = -1;
    while (1) {
        if (!getline(fs, line))
            break;
        ln++;
    next:
        // search for "#test vector n" and grab next n numbers
        s = line.c_str();
        if (!parse_value) {
            if (line.find('#') == string::npos)
                continue;
            p = strstr(s, "test vector");
            if (!p || sscanf_s(p, "test vector %d", &vector_id) != 1)
                continue;
            printf("parse line:%s\n", s);
            printf(" reading vec %d..\n", vector_id);
            parse_value = 1;
            i = 0;
        }
        else {
            if (!is_valid_hex_str(script_type, i, line)) {
                printf(" vector %d (line:%d) invalid, skip\n", vector_id, ln);
                parse_value = 0;
                goto next;
            }
            if (i < n_all) {
                // for sha, data is at first line as ascii text(?); other cases are all numbers
                //if (cmd == SCP_SHA256 && i == 0) {
                //    sha_buf_size = min(200, line.size());
                //    nums[i].resize(sha_buf_size);
                //    memcpy(&nums[0][0], line.c_str(), sha_buf_size);
                //    printf("  vec %d %d llen=%d\n", i, nums[0].size(), line.size());
                //}
                //else {
                //    convert_hex_rev(line, &nums[i][0]);
                //}
                if (tn >= query_start) {
                    printf(" [%d][%d]: %s\n", tn, i, line.c_str());
                    test_vec[ti].push_back(string("0x") + line); // string is Hex format
                }
                i++;
            }
            if (i == n_all) {
                //printf(" test data {%02X %02X .. %02X}\n", nums[0][31], nums[0][30], nums[i-1][0]);
                if (tn >= query_start)
                    ti++;
                
                tn++;
                
                if (tn >= (query_start + query_cases)) {
                    printf(" parse done, skip else\n");
                    break;
                }
                parse_value = 0;
            }
        }
    }

    fs.close();
    return true;
}

template<typename T>
T mod_sub(T& a, T& b, T& N) {
    // all a,b < N
    T ret = a - b;
    if (ret < 0)
        ret += N;
    return ret;
}

template<typename T>
T mod_add(T& a, T& b, T& N) {
    // all a,b < N
    T ret = a + b;
    if (ret > N)
        ret -= N;
    return ret;
}

template<typename T>
T mod_mult(T& a, T& b, T& N) {
    // must a>=0 and b>=0
    T ret = a * b;
    ret %= N;
    return ret;
}

template<typename T>
T mod_divide(T& a, T& b, T& N) {
    // a / b (mod N) = a * b^-1 (mod N)
    T inv = mod_inv(b, N);
    T ret = mod_mult(a, inv, N);
    return ret;
}

template<typename T>
T mod_inv(T& a, T& N) {
    T sx = 1;
    T tx = 0;
    T rx, q, r;
    T A = a;
    T B = N;
    array<T, 2> ret;

    while (B > 0) {
        if constexpr (std::is_same<T, bigint>::value) {
            q = A / B;
            r = A - q * B;
        }
        else {
            // glbigint, glhint
            ret = T::divide(A, B);
            q = ret[0];
            r = ret[1];
        }

        rx = sx - q * tx;
        sx = tx;
        tx = rx;

        A = B;
        B = r;
    }

    if (sx < 0)
        sx += N;

    return sx;
}
template<typename T>
ECC_POINT<T> ecc_point_add(ECC_PARAM<T>& ecc, T& px, T& py, T& qx, T& qy) {
    T lam, t0, t1;
    T big_rx, big_ry;
    T& big_P = *ecc.P;

    // add, for P¡ÚQ
    // £f = (qy-py) / (qx-px)
    // zx = £f*£f - px - qx
    // zy = £f*(px - zx) - py
    t0 = mod_sub(qy, py, big_P);
    t1 = mod_sub(qx, px, big_P);
    t1 = mod_inv(t1, big_P);  // t0 / t1
    lam = (t0 * t1) % (big_P);

    t0 = mod_mult(lam, lam, big_P);
    t0 = mod_sub(t0, px, big_P);
    big_rx = mod_sub(t0, qx, big_P);

    t1 = mod_sub(px, big_rx, big_P);
    t1 = mod_mult(t1, lam, big_P);
    big_ry = mod_sub(t1, py, big_P);

    return { big_rx, big_ry };
}

// issue: fix unresolved external symbol, to instantiate template func in .obj
template ECC_POINT<bigint> ecc_point_add(ECC_PARAM<bigint>& ecc, bigint& px, bigint& py, bigint& qx, bigint& qy);
template ECC_POINT<glbigint> ecc_point_add(ECC_PARAM<glbigint>& ecc, glbigint& px, glbigint& py, glbigint& qx, glbigint& qy);
template ECC_POINT<glhint> ecc_point_add(ECC_PARAM<glhint>& ecc, glhint& px, glhint& py, glhint& qx, glhint& qy);

/*
bigint mod_sub(bigint& a, bigint& b, bigint& N) {
    // all a,b < N
    bigint ret = a - b;
    if (ret < 0)
        ret += N;
    return ret;
}
bigint mod_add(bigint& a, bigint& b, bigint& N) {
    // all a,b < N
    bigint ret = a + b;
    if (ret > N)
        ret -= N;
    return ret;
}
bigint mod_mult(bigint& a, bigint& b, bigint& N) {
    // must a>=0 and b>=0
    bigint ret = a * b;
    ret %= N;
    return ret;
}
bigint mod_divide(bigint& a, bigint& b, bigint& N) {
    // a / b (mod N) = a * b^-1 (mod N)
    bigint inv = mod_inv(b, N);
    bigint ret = mod_mult(a, inv, N);
    return ret;
}
bigint mod_inv(bigint& a, bigint& N) {
    bigint sx = 1;
    bigint tx = 0;
    bigint rx, q, r;
    bigint A = a;
    bigint B = N;
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
        sx += N;

    return sx;
}*/

/*
ECC_POINT<bigint> ecc_point_add(ECC_PARAM<bigint>& ecc, bigint& px, bigint& py, bigint &qx, bigint &qy) {
    bigint lam, t0, t1;
    bigint big_rx, big_ry;
    bigint& big_P = *ecc.P;

    // add, for P¡ÚQ
    // £f = (qy-py) / (qx-px)
    // zx = £f*£f - px - qx
    // zy = £f*(px - zx) - py
    t0 = mod_sub(qy, py, big_P);
    t1 = mod_sub(qx, px, big_P);
    t1 = mod_inv(t1, big_P);  // t0 / t1
    lam = (t0 * t1) % (big_P);

    t0 = mod_mult(lam, lam, big_P);
    t0 = mod_sub(t0, px, big_P);
    big_rx = mod_sub(t0, qx, big_P);

    t1 = mod_sub(px, big_rx, big_P);
    t1 = mod_mult(t1, lam, big_P);
    big_ry = mod_sub(t1, py, big_P);

    return { big_rx, big_ry };
}*/

string hex_to_dec_string(string&& n) {
    bigint x(n);
    string out(x);
    return out;
}

string normalize_hex_string(string& n) {
    // must prefix with '0x'
    // remove leading 0's after '0x'
    // ex. 0x00123 => 0x123
    if (n.length() < 2 || (n[0] != '0' || n[1] != 'x'))
        return "0x0";  // error
    int zeros = 0;
    while ((zeros + 2) < n.length() && n[zeros + 2] == '0')
        zeros++;
    return "0x" + n.substr(zeros + 2);
}
string normalize_hex_string(string&& n) {
    string tmp = n;
    return normalize_hex_string(tmp);
}
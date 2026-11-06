
#include <stdio.h>
#include <iostream>
#include <string>
#include <vector>
#include <array>

#include "bigint.h"
#include "GlBigInt.h"
#include "util.h"

//#include "sha256.h" // in util.h

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
            //printf("parse line:%s\n", s);
            //printf(" reading vec %d..\n", vector_id);
            parse_value = 1;
            i = 0;
        }
        else {
            if (!is_valid_hex_str(script_type, i, line)) {
                //printf(" vector %d (line:%d) invalid, skip\n", vector_id, ln);
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
                    //printf(" [%d][%d]: %s\n", tn, i, line.c_str());
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

    //if (px == qx && py == qy)
    //    return ecc_point_double(ecc, px, py);
    
    // add, for P¡ÚQ case
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

template<typename T>
ECC_POINT<T> ecc_point_double(ECC_PARAM<T>& ecc, T& px, T& py) {
    T lam, t0, t1;
    T big_rx, big_ry;
    T& big_P = *ecc.P;
    T& big_A = *ecc.A;

    // add, for P=Q case
    // £f = (3*px*px+a) / (2py)
    // zx = £f*£f - px - px
    // zy = £f*(px - zx) - py
    t0 = mod_mult(px, px, big_P);
    t1 = mod_add(t0, t0, big_P);
    t1 = mod_add(t1, t0, big_P); //3x ^ 2
    t1 = mod_add(t1, big_A, big_P);

    t0 = mod_add(py, py, big_P);
    t0 = mod_inv(t0, big_P);

    lam = mod_mult(t0, t1, big_P);

    t0 = mod_mult(lam, lam, big_P);
    t0 = mod_sub(t0, px, big_P);
    big_rx = mod_sub(t0, px, big_P);

    t1 = mod_sub(px, big_rx, big_P);
    t1 = mod_mult(t1, lam, big_P);
    big_ry = mod_sub(t1, py, big_P);

    return { big_rx, big_ry };
}

template<typename T>
ECC_POINT<T> ecc_point_scale(ECC_PARAM<T>& ecc, T& px, T& py, T& k) {
    ECC_POINT<T> tmp;
    int idx{ 0 }, context{ 0 }, bit{ 0 };

    // note: Identity 0 is invalid number
    if (!glhint::c_next_bit(k, &context, NULL)) {
        printf("init enum bit fail\n");
        exit(1);
        return tmp;
    }

    tmp.x = px;
    tmp.y = py;

    idx = 0;
    glhint::c_next_bit(k, &context, &bit);
    idx++;
    // assert(bit == 1)

    //count_reset();
    // enum bit [1:_]
    while (glhint::c_next_bit(k, &context, &bit)) {
        idx++;
        tmp = ecc_point_double(ecc, tmp.x, tmp.y);
        if (bit) {
            tmp = ecc_point_add(ecc, tmp.x, tmp.y, px, py);
        }
    }

    return tmp;
}

template<typename T>
bool ecdsa_verify_sig(ECC_PARAM<T>& ecc, T& pubx, T& puby, T& r, T& s, T& msg) {
    ECC_POINT<T> t1, t2, P;
    T w, u1, u2;
    //w = s ^ -1
    //u1 = w * h(x)
    //u2 = w * r
    //P = u1 * G + u2 * B ..point scaling
    //P.x == r ?

    w = mod_inv(s, *ecc.N);
    u1 = mod_mult(w, msg, *ecc.N);
    u2 = mod_mult(w, r, *ecc.N);
    t1 = ecc_point_scale(ecc, *ecc.Gx, *ecc.Gy, u1);
    t2 = ecc_point_scale(ecc, pubx, puby, u2);
    P = ecc_point_add(ecc, t1.x, t1.y, t2.x, t2.y);
    cout << "Px:" << P.x << endl;
    cout << " r:" << r << endl;

    bool ok = P.x == r;
    cout << (ok ? " verify OK" : " verify FAIL") << endl;
    return ok;
}

// sign msg with private key 'priv', output signatures as 'r' & 's'
template<typename T>
bool ecdsa_sign(ECC_PARAM<T>& ecc, T& priv, T& msg, T& r, T& s) {
    T ke, ki, tmp;
    ECC_POINT<T> q;

    // ke: random number
    // qx = x of Q = k*G
    // s = (msg + d*qx) / k

    // fake or gen random..
    ke = "0x5010101010101010101010101010101010101010101010101010101010101010";

    q = ecc_point_scale(ecc, *ecc.Gx, *ecc.Gy, ke);
    ki = mod_inv(ke, *ecc.N);
    tmp = mod_mult(priv, q.x, *ecc.N);
    tmp = mod_add(tmp, msg, *ecc.N);
    s = mod_mult(tmp, ki, *ecc.N);
    r = q.x;

    return true;
}

// issue: fix unresolved external symbol, to instantiate template func in .obj
template ECC_POINT<bigint> ecc_point_add(ECC_PARAM<bigint>& ecc, bigint& px, bigint& py, bigint& qx, bigint& qy);
template ECC_POINT<glbigint> ecc_point_add(ECC_PARAM<glbigint>& ecc, glbigint& px, glbigint& py, glbigint& qx, glbigint& qy);
template ECC_POINT<glhint> ecc_point_add(ECC_PARAM<glhint>& ecc, glhint& px, glhint& py, glhint& qx, glhint& qy);
template ECC_POINT<glhint> ecc_point_double(ECC_PARAM<glhint>& ecc, glhint& px, glhint& py);
template ECC_POINT<glhint> ecc_point_scale(ECC_PARAM<glhint>& ecc, glhint& px, glhint& py, glhint& k);
template bool ecdsa_verify_sig(ECC_PARAM<glhint>& ecc, glhint& pubx, glhint& puby, glhint& r, glhint& s, glhint& msg);
template bool ecdsa_sign(ECC_PARAM<glhint>& ecc, glhint& priv, glhint& msg, glhint& r, glhint& s);

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
    if (zeros)
        return "0x" + n.substr(zeros + 2);
    else
        return n;
}
string normalize_hex_string(string&& n) {
    string tmp = n;
    return normalize_hex_string(tmp);
}

// SHA256 implementations
// Rotate right
static inline uint32_t rotr(uint32_t x, int n) {
    return (x >> n) | (x << (32 - n));
}

static inline uint32_t step1(uint32_t e, uint32_t f, uint32_t g) {
    return (rotr(e, 6) ^ rotr(e, 11) ^ rotr(e, 25)) + ((e & f) ^ ((~e) & g));
}

static inline uint32_t step2(uint32_t a, uint32_t b, uint32_t c) {
    return (rotr(a, 2) ^ rotr(a, 13) ^ rotr(a, 22)) + ((a & b) ^ (a & c) ^ (b & c));
}

static inline void update_w(uint32_t* w, int i, const uint8_t* buffer) {
    int j;
    for (j = 0; j < 16; j++) {
        if (i < 16) {
            w[j] =
                ((uint32_t)buffer[0] << 24) |
                ((uint32_t)buffer[1] << 16) |
                ((uint32_t)buffer[2] << 8) |
                ((uint32_t)buffer[3]);
            buffer += 4;
        }
        else {
            uint32_t a = w[(j + 1) & 15];
            uint32_t b = w[(j + 14) & 15];
            uint32_t s0 = (rotr(a, 7) ^ rotr(a, 18) ^ (a >> 3));
            uint32_t s1 = (rotr(b, 17) ^ rotr(b, 19) ^ (b >> 10));
            w[j] += w[(j + 9) & 15] + s0 + s1;
        }
    }
}

static void sha256_block(struct sha256* sha) {
    // State of the program
    uint32_t* state = sha->state;

    // Declare the K constant
    static const uint32_t k[8 * 8] = {
        0x428a2f98, 0x71374491, 0xb5c0fbcf, 0xe9b5dba5,
        0x3956c25b, 0x59f111f1, 0x923f82a4, 0xab1c5ed5,
        0xd807aa98, 0x12835b01, 0x243185be, 0x550c7dc3,
        0x72be5d74, 0x80deb1fe, 0x9bdc06a7, 0xc19bf174,
        0xe49b69c1, 0xefbe4786, 0x0fc19dc6, 0x240ca1cc,
        0x2de92c6f, 0x4a7484aa, 0x5cb0a9dc, 0x76f988da,
        0x983e5152, 0xa831c66d, 0xb00327c8, 0xbf597fc7,
        0xc6e00bf3, 0xd5a79147, 0x06ca6351, 0x14292967,
        0x27b70a85, 0x2e1b2138, 0x4d2c6dfc, 0x53380d13,
        0x650a7354, 0x766a0abb, 0x81c2c92e, 0x92722c85,
        0xa2bfe8a1, 0xa81a664b, 0xc24b8b70, 0xc76c51a3,
        0xd192e819, 0xd6990624, 0xf40e3585, 0x106aa070,
        0x19a4c116, 0x1e376c08, 0x2748774c, 0x34b0bcb5,
        0x391c0cb3, 0x4ed8aa4a, 0x5b9cca4f, 0x682e6ff3,
        0x748f82ee, 0x78a5636f, 0x84c87814, 0x8cc70208,
        0x90befffa, 0xa4506ceb, 0xbef9a3f7, 0xc67178f2,
    };

    uint32_t a = state[0];
    uint32_t b = state[1];
    uint32_t c = state[2];
    uint32_t d = state[3];
    uint32_t e = state[4];
    uint32_t f = state[5];
    uint32_t g = state[6];
    uint32_t h = state[7];

    uint32_t w[16];

    int i, j;
    for (i = 0; i < 64; i += 16) {
        update_w(w, i, sha->buffer);

        for (j = 0; j < 16; j += 4) {
            uint32_t temp;
            temp = h + step1(e, f, g) + k[i + j + 0] + w[j + 0];
            h = temp + d;
            d = temp + step2(a, b, c);
            temp = g + step1(h, e, f) + k[i + j + 1] + w[j + 1];
            g = temp + c;
            c = temp + step2(d, a, b);
            temp = f + step1(g, h, e) + k[i + j + 2] + w[j + 2];
            f = temp + b;
            b = temp + step2(c, d, a);
            temp = e + step1(f, g, h) + k[i + j + 3] + w[j + 3];
            e = temp + a;
            a = temp + step2(b, c, d);
        }
    }

    state[0] += a;
    state[1] += b;
    state[2] += c;
    state[3] += d;
    state[4] += e;
    state[5] += f;
    state[6] += g;
    state[7] += h;
}

void sha256_init(struct sha256* sha) {
    sha->state[0] = 0x6a09e667;
    sha->state[1] = 0xbb67ae85;
    sha->state[2] = 0x3c6ef372;
    sha->state[3] = 0xa54ff53a;
    sha->state[4] = 0x510e527f;
    sha->state[5] = 0x9b05688c;
    sha->state[6] = 0x1f83d9ab;
    sha->state[7] = 0x5be0cd19;
    sha->n_bits = 0;
    sha->buffer_counter = 0;
}

void sha256_append_byte(struct sha256* sha, uint8_t byte) {
    sha->buffer[sha->buffer_counter++] = byte;
    sha->n_bits += 8;

    if (sha->buffer_counter == 64) {
        sha->buffer_counter = 0;
        sha256_block(sha);
    }
}

void sha256_append(struct sha256* sha, const void* src, size_t n_bytes) {
    const uint8_t* bytes = (const uint8_t*)src;
    size_t i;

    for (i = 0; i < n_bytes; i++) {
        sha256_append_byte(sha, bytes[i]);
    }
}

void sha256_finalize(struct sha256* sha) {
    int i;
    uint64_t n_bits = sha->n_bits;

    sha256_append_byte(sha, 0x80);

    while (sha->buffer_counter != 56) {
        sha256_append_byte(sha, 0);
    }

    for (i = 7; i >= 0; i--) {
        uint8_t byte = (n_bits >> 8 * i) & 0xff;
        sha256_append_byte(sha, byte);
    }
}

void sha256_finalize_hex(struct sha256* sha, char* dst_hex65) {
    int i, j;
    sha256_finalize(sha);

    for (i = 0; i < 8; i++) {
        for (j = 7; j >= 0; j--) {
            uint8_t nibble = (sha->state[i] >> j * 4) & 0xf;
            *dst_hex65++ = "0123456789abcdef"[nibble];
        }
    }

    *dst_hex65 = '\0';
}

void sha256_finalize_bytes(struct sha256* sha, void* dst_bytes32) {
    uint8_t* ptr = (uint8_t*)dst_bytes32;
    int i, j;
    sha256_finalize(sha);

    for (i = 0; i < 8; i++) {
        for (j = 3; j >= 0; j--) {
            *ptr++ = (sha->state[i] >> j * 8) & 0xff;
        }
    }
}

void sha256_hex(const void* src, size_t n_bytes, char* dst_hex65) {
    struct sha256 sha;
    sha256_init(&sha);
    sha256_append(&sha, src, n_bytes);
    sha256_finalize_hex(&sha, dst_hex65);
}

void sha256_bytes(const void* src, size_t n_bytes, void* dst_bytes32) {
    struct sha256 sha;
    sha256_init(&sha);
    sha256_append(&sha, src, n_bytes);
    sha256_finalize_bytes(&sha, dst_bytes32);
}
#pragma once

#include <string>
#include <sstream>
#include <utility>
#include <vector>
//#include <cmath>
#include <limits>
//#include <stdexcept>
#include <iostream>
//#include <functional>
#include <algorithm>
#include <iomanip>

#ifndef BYTE
#define BYTE unsigned char
#endif

namespace GLLIB {
    class glbigint {
    public:
        static constexpr auto MAX_SIZE = 1000000000LL;
        //static constexpr auto MAX_SIZE = 100LL;
        static constexpr auto CHUNK_SIZE = 9L;  // max_size's digits

        bool is_neg{ false };
        //int chnk_num;
        std::vector<long long> vec;

        // constructors
        glbigint() : vec({ 0 }) {}
        glbigint(std::vector<long long> _vec) { // need to set sign later
            vec = std::move(_vec);
        }
        glbigint(std::string s) {
            if (!is_bigint(s)) {
                throw::std::runtime_error("Invalid Big Int Format");
            }
            if (s[0] == '-') {
                *this = glbigint(s.substr(1));
                is_neg = true;
            }
            else if (s[0] == '0' && s[1] == 'x') {
                // hex s[2:]
                is_neg = false;
                vec = hex_to_vector(s);
            }
            else {
                is_neg = false;
                vec = string_to_vector(s);
            }
        }
        glbigint(const char* cn) : glbigint(std::string(cn)) {}

        glbigint(const int n) : glbigint(static_cast<long long>(n)) {}
        glbigint(const double n) : glbigint(static_cast<long long>(n)) {}
        glbigint(const long long n) {
            is_neg = n < 0;
            if (n >= MAX_SIZE) {
                vec.emplace_back(n / MAX_SIZE);
                vec.emplace_back(n % MAX_SIZE);
            }
            else{
                vec.emplace_back(n);
            }
        }

        // operators
        glbigint& operator=(const glbigint& other) {
            if (this == &other)
                return *this;
            this->is_neg = other.is_neg;
            this->vec = other.vec;

            return *this;
        }

        glbigint operator+(const glbigint& rhs) const {
            glbigint res = *this;
            res += rhs;
            return res;
        }
        glbigint operator+=(const glbigint& rhs) {
            if (*this == 0 && rhs == 0)
                return *this;

            *this = add(*this, rhs);
            return *this;
        }

        glbigint operator-=(const glbigint& rhs) {
            if (rhs == 0) { return *this; }
            if (*this == rhs) {
                *this = 0;
                return *this;
            }
            *this = subtract(*this, rhs);
            return *this;
        }
        glbigint operator-(const glbigint& rhs) const {
            glbigint result = *this;
            result -= rhs;
            return result;
        }

        glbigint operator*=(const glbigint& rhs) {
            *this = multiply(*this, rhs);
            return *this;
        }

        glbigint operator*(const glbigint& rhs) const {
            glbigint result = *this;
            result *= rhs;
            return result;
        }

        glbigint operator/=(const glbigint& rhs) {
            *this = divide(*this, rhs)[0];
            return *this;
        }
        glbigint operator/(const glbigint& rhs) const {
            glbigint result = *this;
            result /= rhs;
            return result;
        }

        glbigint operator%=(const glbigint& rhs) {
            auto res = divide(*this, rhs);
            *this = res[1];
            return *this;
        }
        glbigint operator%(const glbigint& rhs) const {
            glbigint result = *this;
            result %= rhs;
            return result;
        }

        friend bool operator==(const glbigint& l, const glbigint& r) {
            if (l.is_neg != r.is_neg) {
                return false;
            }
            return l.vec == r.vec;
        }

        friend bool operator!=(const glbigint& l, const glbigint& r) {
            return !(l == r);
        }

        friend bool operator<(const glbigint& lhs, const glbigint& rhs) {
            return less_than(lhs, rhs);
        }
        friend bool operator>(const glbigint& l, const glbigint& r) {
            return r < l;
        }
        friend bool operator<=(const glbigint& l, const glbigint& r) {
            return r >= l;
        }
        friend bool operator>=(const glbigint& l, const glbigint& r) {
            return !(l < r);
        }

        static glbigint abs(const glbigint& s) {
            if (!is_negative(s)) return s;
            glbigint temp = s;
            temp.is_neg = false;
            return temp;
        }
        static glbigint abs(glbigint&& s) {
            s.is_neg = false;
            return s;
        }
        static bool is_negative(const glbigint& input) {
            return input.is_neg;
        }
        static glbigint negate(const glbigint& input) {
            glbigint temp = input;
            temp.is_neg = !temp.is_neg;
            return temp;
        } 
        static glbigint negate(glbigint&& input) {
            input.is_neg = !input.is_neg;
            return input;
        }

        static bool less_than(const glbigint& lhs, const glbigint& rhs);
        static int count_digits(const glbigint&);

        // **** C-like part ****
        // TO port to kmdf driver
        /* // 2^256*2^256, max=2^512, one vec[i] with 10^9, log10(2^512)=154.1, use 9*18=162 dec-digis
        *  // 
        * struct {
        *   BYTE is_neg;
        *   BYTE digits;
        *   BYTE reserved; == const 162 digits = 18 elms
        *   int64_t  vec[18];
        * */
        static bool c_compare_eq(const glbigint& lhs, const glbigint& rhs) {
            return false;
        }
        static bool c_compare_lt(const glbigint& lhs, const glbigint& rhs) {
            // TBD.
            return false;
        }
        static bool c_compare_ge(const glbigint& lhs, const glbigint& rhs) {
            return !(c_compare_lt(lhs, rhs));
        }
        static bool c_compare_le(const glbigint& lhs, const glbigint& rhs) {
            return c_compare_ge(rhs, lhs);
        }
        static void c_from_msb_hex_bytes(glbigint& num, const BYTE *data, int len, bool is_negative);  // 0x1234 => data={12,34},len=2
        static void c_from_msb_dec_chars(glbigint& num, const BYTE* data, int len, bool is_negative);  // 10245 => data={1,0,2,4,5},len=5
        static int  c_count_digits(const glbigint&);

        // convert
        explicit operator int() const {
            return static_cast<int>(vec.back());
        }
        explicit operator long long() const {
            return static_cast<long long>(vec.back());
        }
        explicit operator std::string() const {
            return (is_neg ? "-" : "") + vector_to_string(vec);
        }

        friend std::ostream& operator<<(std::ostream& os, const glbigint& num) {
            os << std::string(num);
            return os;
        }

        // helpers
        static glbigint add(const glbigint&, const glbigint&);
        static glbigint subtract(const glbigint&, const glbigint&);
        static glbigint multiply(const glbigint&, const glbigint&);
        static std::array<glbigint, 2> divide(const glbigint&, const glbigint&);
        //static array<glbigint, 2> div_mod(const glbigint&, const glbigint&);
        static glbigint pow10(int exponent);
        static bool is_bigint(const std::string&);
        static std::vector<long long> string_to_vector(std::string input);
        static std::vector<long long> hex_to_vector(std::string input);
        static std::string vector_to_string(const std::vector<long long>& input);

        // internal use
        //  trim front 0's, pass by value?
        static glbigint trim(glbigint num) {
            int ei = 0, en = 0;
            glbigint temp;
            while (en < num.vec.size() - 1 && num.vec[en] == 0) {
                en++;
            }
            
            temp.vec.resize(num.vec.size() - en);
            for (ei = 0; ei < temp.vec.size(); ei++)
                temp.vec[ei] = num.vec[ei + en];
            temp.is_neg = num.is_neg;
            return temp;
        }
    };

    inline bool glbigint::less_than(const glbigint& lhs, const glbigint& rhs) {
        if (is_negative(lhs) && is_negative(rhs)) {
            return less_than(abs(rhs), abs(lhs));
        }

        if (is_negative(lhs) || is_negative(rhs)) {
            return is_negative(lhs);
        }

        if (lhs.vec.size() == rhs.vec.size()) {
            return lhs.vec < rhs.vec;
        }

        return lhs.vec.size() < rhs.vec.size();
    }

    inline glbigint glbigint::add(const glbigint& lhs, const glbigint& rhs) {
        bool negate_answer = false;
        // Ensure both are positive
        if (is_negative(lhs) && is_negative(rhs)) negate_answer = true;
        else if (is_negative(lhs)) return subtract(rhs, abs(lhs));
        else if (is_negative(rhs)) return subtract(lhs, abs(rhs));

        // Ensure LHS is larger than RHS
        if (lhs.vec.size() < rhs.vec.size()) return add(rhs, lhs);

        // Prepare result vector with enough space (max size + 1 for potential carry)
        // MSByte endian
        std::vector<long long> result;
        result.resize(lhs.vec.size() + 1);  // direct access by [] after reserve() will fail too

        long long carry = 0;
        int ln, li, rn, ri, res_i;
        ln = (int)lhs.vec.size();
        rn = (int)rhs.vec.size();
        res_i = 0;

        li = ln - 1;
        ri = rn - 1;
        while (li >= 0) {
            long long _sum = lhs.vec[li] + carry;
            if (ri >= 0) {
                _sum += rhs.vec[ri];
                ri--;
            }

            if (_sum >= MAX_SIZE) {
                _sum -= MAX_SIZE;
                carry = 1;
            }
            else {
                carry = 0;
            }

            result[res_i] = _sum;
            res_i++;
            //result.push_back(_sum);

            li--;
        }

        if (carry) {
            result[res_i] = carry;
            res_i++;
            carry = 0;
        }

        result.resize(res_i);
        std::reverse(result.begin(), result.end());
        glbigint result_bigint{ std::move(result) };
        
        return negate_answer ? negate(result_bigint) : result_bigint;
        //return result_bigint;
    }

    inline glbigint glbigint::subtract(const glbigint& lhs, const glbigint& rhs) {
        // Ensure LHS is larger than RHS, and both are positive
        // (-A) - (-B) == B - A
        if (is_negative(lhs) && is_negative(rhs)) {
            return subtract(abs(rhs), abs(lhs));
        }
        // (A) - (-B) == A + B
        if (is_negative(rhs)) {
            return add(lhs, abs(rhs));
        }
        // (-A) - (B) == -(A + B)
        if (is_negative(lhs)) {
            return negate(add(abs(lhs), rhs));
        }
        if (lhs < rhs) {
            return negate(subtract(rhs, lhs));
        }

        std::vector<long long> result;
        //result.reserve(lhs.vec.size());
        result.resize(lhs.vec.size());
        long long borrow = 0;
        int li, ln, ri, rn, res_i;

        ln = (int)lhs.vec.size();
        li = ln - 1;
        rn = (int)rhs.vec.size();
        ri = rn - 1;
        res_i = 0;
        while (li >= 0) {
            long long rval{ 0 }, diff{ 0 };
            
            if (ri >= 0) {
                rval = rhs.vec[ri];
                ri--;
            }

            diff = lhs.vec[li] - rval - borrow;

            if (diff < 0) {
                diff += MAX_SIZE;
                borrow = 1;
            }
            else {
                borrow = 0;
            }

            //result.push_back(diff);
            result[res_i] = diff;
            res_i++;

            li--;
        }

        result.resize(res_i);

        /*auto it_l = lhs.vec.rbegin();
        auto it_r = rhs.vec.rbegin();

        while (it_l != lhs.vec.rend()) {
            const long long l_val = *it_l;
            const long long r_val = (it_r != rhs.vec.rend()) ? *it_r : 0;

            long long diff = l_val - r_val - borrow;
            if (diff < 0) {
                diff += MAX_SIZE;
                borrow = 1;
            }
            else {
                borrow = 0;
            }
            result.push_back(diff);

            ++it_l;
            if (it_r != rhs.vec.rend()) ++it_r;
        }*/

        std::reverse(result.begin(), result.end());
        return trim(glbigint(std::move(result)));
    }

    inline glbigint glbigint::multiply(const glbigint& lhs, const glbigint& rhs) {
        if (lhs == 0 || rhs == 0) return 0;
        if (lhs == 1) return rhs;
        if (rhs == 1) return lhs;

        if (is_negative(lhs) && is_negative(rhs)) {
            return (abs(lhs) * abs(rhs));
        }
        if (is_negative(lhs) || is_negative(rhs)) {
            return negate(abs(lhs) * abs(rhs));
        }
        if (lhs < rhs) {
            return multiply(rhs, lhs);
        }

        std::vector<long long> result(lhs.vec.size() + rhs.vec.size(), 0);
        int li, ln, ri, rn, res_i, res_h, res_n;
        long long mul{ 0 };

        res_n = result.size();
        ln = lhs.vec.size();
        rn = rhs.vec.size();
        for (li = ln - 1; li >= 0; li--) {
            for (ri = rn - 1; ri >= 0; ri--) {
                // calc product and result's index
                mul = lhs.vec[li] * rhs.vec[ri];
                res_i = res_n - 1 - ((ln - 1 - li) + (rn - 1 - ri));
                res_h = res_i - 1;

                // add to result vector
                result[res_i] += mul % MAX_SIZE;
                if (res_h >= 0) {
                    result[res_h] += mul / MAX_SIZE;
                }

                // possible carry
                if (result[res_i] >= MAX_SIZE) {
                    if (res_h >= 0) {
                        result[res_h] += result[res_i] / MAX_SIZE;
                    }
                    result[res_i] %= MAX_SIZE;
                }
            }
        }

        // Handle carries for remaining positions
        for (res_i = res_n - 1; res_i > 0; res_i--) {
            if (result[res_i] >= MAX_SIZE) {
                result[res_i - 1] += result[res_i] / MAX_SIZE;
                result[res_i] %= MAX_SIZE;
            }
        }

        /*
        for (auto it_lhs = lhs.vec.rbegin(); it_lhs != lhs.vec.rend(); ++it_lhs) {
            for (auto it_rhs = rhs.vec.rbegin(); it_rhs != rhs.vec.rend(); ++it_rhs) {
                // Calculate the product and the corresponding indices in the result vector
                long long mul = (*it_lhs) * (*it_rhs);
                auto pos_low_it = result.rbegin() + (std::distance(lhs.vec.rbegin(), it_lhs) + std::distance(
                    rhs.vec.rbegin(), it_rhs));
                auto pos_high_it = pos_low_it + 1;

                // Add the product to the result vector
                *pos_low_it += mul % MAX_SIZE;
                if (pos_high_it != result.rend()) {
                    *pos_high_it += mul / MAX_SIZE;
                }

                // Handle carry
                if (*pos_low_it >= MAX_SIZE) {
                    if (pos_high_it != result.rend()) {
                        *pos_high_it += *pos_low_it / MAX_SIZE;
                    }
                    *pos_low_it %= MAX_SIZE;
                }
            }
        }

        // Handle carries for remaining positions
        for (auto r_iter = result.rbegin(); r_iter != result.rend() - 1; ++r_iter) {
            if (*r_iter >= MAX_SIZE) {
                *(r_iter + 1) += *r_iter / MAX_SIZE;
                *r_iter %= MAX_SIZE;
            }
        }*/

        return trim(result);
    }

    inline std::array<glbigint, 2> glbigint::divide(const glbigint& numerator, const glbigint& denominator) {
        if (denominator == 0) {
            throw std::domain_error("Attempted to divide by zero.");
        }
        if (numerator == denominator) return { 1,0 };
        if (denominator == 1) return { numerator, 0 };
        if (numerator == 0) return { 0, 0 };

        if (is_negative(numerator) && is_negative(denominator)) {
            return divide(abs(numerator), abs(denominator));
        }
        if (is_negative(numerator) || is_negative(denominator)) {
            // no valid definition for negative remainder?
            //return negate(divide(abs(numerator), abs(denominator)));
            auto result = divide(abs(numerator), abs(denominator));
            return { negate(result[0]), result[1] };
        }
        if (numerator < denominator) {
            return { 0, numerator };
        }
        if (numerator.vec.size() <= 1) {
            return { numerator.vec.back() / denominator.vec.back(), numerator.vec.back() % denominator.vec.back() };
        }

        glbigint remainder = numerator;
        glbigint quotient = 0;

        while (remainder >= denominator) {
            //int count = count_digits(remainder) - count_digits(denominator) - 1;
            int count = c_count_digits(remainder) - c_count_digits(denominator) - 1;

            // Prevent negative exponents which cause pow() to return 0
            if (count < 0) {
                count = 0;
            }

            glbigint numerator_size = pow10(count);  //pow(10, count);
            glbigint temp = denominator * numerator_size;

            // Repeatedly subtract the chunk from the remainder
            while (remainder >= temp) {
                remainder -= temp;
                quotient += numerator_size;
            }
        }

        return { quotient, remainder };
    }

    inline glbigint glbigint::pow10(int exponent) {
        glbigint result;
        std::vector<BYTE> data(exponent + 1, 0);
        if (exponent < 0) {
            c_from_msb_dec_chars(result, &data[0], 1, false);
        }
        else if (exponent == 0) {
            data[0] = 1;
            c_from_msb_dec_chars(result, &data[0], 1, false);
        }
        else if (exponent == 1) {
            data[0] = 1;
            data[1] = 0;
            c_from_msb_dec_chars(result, &data[0], 2, false);
        }
        else {
            data[0] = 1;
            c_from_msb_dec_chars(result, &data[0], exponent + 1, false);
        }
        return result;
    }

    inline bool glbigint::is_bigint(const std::string& s) {
        if (s.empty())
            return false;

        // Check for leading negative sign
        if (s[0] == '-') {
            if (s.length() == 1) return false;
            // Recursively validate the rest as a positive bigint
            // Note: leading zeros check still applies to the magnitude
            return is_bigint(s.substr(1));
        }

        // Check for hex string "0x..."
        if (s.length() > 2 && s[0] == '0' && s[1] == 'x') {
            return s.find_first_not_of("0123456789abcdefABCDEF", 2) == std::string::npos;
        }

        // Reject leading zeros for decimal numbers
        if (s.length() > 1 && s[0] == '0')
            return false;

        return s.find_first_not_of("0123456789", 0) == std::string::npos;
    }

    inline std::vector<long long> glbigint::string_to_vector(std::string input) {
        // Break into chunks of 9 characters
        std::vector<long long> result;
        constexpr int chunk_size = 9;
        const int size = input.size();

        if (size > chunk_size) {
            // Pad the length to get appropriate sized chunks
            if (int mod = size % chunk_size; mod != 0) {
                input.insert(0, chunk_size - mod, '0');
            }
        }
        for (int i = 0; i < input.size(); i += chunk_size) {
            std::string temp_str = input.substr(i, chunk_size);
            result.emplace_back(stoll(temp_str));
        }

        return result;
    }

    inline std::vector<long long> glbigint::hex_to_vector(std::string input) {
        // Strip "0x" prefix
        const std::string hex = input.substr(2);

        std::vector<long long> result = { 0 };

        for (const char c : hex) {
            unsigned long long digit;
            if (c >= '0' && c <= '9')      digit = c - '0';
            else if (c >= 'a' && c <= 'f') digit = c - 'a' + 10;
            else if (c >= 'A' && c <= 'F') digit = c - 'A' + 10;
            else throw std::runtime_error("Invalid hex character.");

            // Multiply current value by 16 and add digit.
            // Max intermediate value: (MAX_SIZE-1)*16+15 = 15,999,999,999
            // which fits in unsigned long long.
            unsigned long long carry = digit;
            for (int i = static_cast<int>(result.size()) - 1; i >= 0; --i) {
                const unsigned long long val = static_cast<unsigned long long>(result[i]) * 16 + carry;
                result[i] = static_cast<long long>(val % static_cast<unsigned long long>(MAX_SIZE));
                carry = val / static_cast<unsigned long long>(MAX_SIZE);
            }
            // carry <= 15 < MAX_SIZE, so this loop runs at most once
            while (carry > 0) {
                result.insert(result.begin(), static_cast<long long>(carry % static_cast<unsigned long long>(MAX_SIZE)));
                carry /= static_cast<unsigned long long>(MAX_SIZE);
            }
        }

        return result;
    }

    inline std::string glbigint::vector_to_string(const std::vector<long long>& input) {
        std::stringstream ss;
        bool first = true;
        for (const auto partial : input) {
            if (first) {
                ss << partial; // No padding for the first number
                first = false;
            }
            else {
                ss << std::setw(9) << std::setfill('0') << partial; // Pad to 9 digits
            }
        }
        return ss.str();
    }

    inline int glbigint::count_digits(const glbigint& input) {
        std::string my_string = vector_to_string(input.vec);
        return static_cast<int>(my_string.length());
    }

    // **** C-like Func implement ****
    inline void glbigint::c_from_msb_hex_bytes(glbigint &num, const BYTE* data, int len, bool is_negative) {
        // each BYTE[] = a single 4-bit (0~F) number
        // refer to original 'string to vector'
        // turn data[] to num
        throw::std::runtime_error("TBD.!!");
    }
    inline void glbigint::c_from_msb_dec_chars(glbigint& num, const BYTE* data, int len, bool is_negative) {
        // each BYTE[] = a single decimal digit 0~9 number
        int chunk_size = CHUNK_SIZE;
        int i, j, di, shift, chunk_num;
        long long val;
        
        chunk_num = (len + chunk_size - 1) / chunk_size;
        num.vec.resize(chunk_num);
        val = 0;
        di = 0;

        shift = len % chunk_size;
        if (shift == 0)
            shift = chunk_size;

        for (i = 0; i < chunk_num; i++) {

            for (j = 0; j < shift; j++, di++)
                val = val * 10 + (long long)data[di];
            
            num.vec[i] = val;
            shift = chunk_size;
            val = 0;
        }
    }

    inline int glbigint::c_count_digits(const glbigint& num) {
        // only meaningful if num positive
        if (num.vec.size() == 0) return 0; // invalid case

        long long val = num.vec[0];
        int digi = 0;

        while (val) {
            val /= 10;
            digi++;
        }
        if (num.vec.size() > 1)
            digi += ((int)num.vec.size() - 1) * CHUNK_SIZE;
        return digi;
    }

    // ***** Big number, hex base implementation *****
    class glhint {
    public:
        static constexpr auto MAX_SIZE = 0x1000LL;
        //static constexpr auto MAX_SIZE = 100LL;
        static constexpr auto CHUNK_SIZE = 3L;  // max_size's digits

        bool is_neg{ false };
        //int chnk_num;
        std::vector<long long> vec;

        // constructors
        glhint() : vec({ 0 }) {}
        glhint(std::vector<long long> _vec) { // need to set sign later
            vec = std::move(_vec);
        }
        glhint(std::string s) {
            if (!is_bigint(s)) {
                throw::std::runtime_error("Invalid Big Int Format");
            }
            if (s[0] == '-') {
                *this = glhint(s.substr(1));
                is_neg = true;
            }
            else if (s[0] == '0' && s[1] == 'x') {
                // hex s[2:]
                is_neg = false;
                vec = hex_to_vector(s);
            }
            else {
                is_neg = false;
                vec = string_to_vector(s);
            }
        }
        glhint(const char* cn) : glhint(std::string(cn)) {}

        glhint(const int n) : glhint(static_cast<long long>(n)) {}
        glhint(const double n) : glhint(static_cast<long long>(n)) {}
        glhint(const long long n) {
            is_neg = n < 0;
            if (n >= MAX_SIZE) {
                vec.emplace_back(n / MAX_SIZE);
                vec.emplace_back(n % MAX_SIZE);
            }
            else {
                vec.emplace_back(n);
            }
        }

        // operators
        glhint& operator=(const glhint& other) {
            if (this == &other)
                return *this;
            this->is_neg = other.is_neg;
            this->vec = other.vec;

            return *this;
        }

        glhint operator+(const glhint& rhs) const {
            glhint res = *this;
            res += rhs;
            return res;
        }
        glhint operator+=(const glhint& rhs) {
            if (*this == 0 && rhs == 0)
                return *this;

            *this = add(*this, rhs);
            return *this;
        }

        glhint operator-=(const glhint& rhs) {
            if (rhs == 0) { return *this; }
            if (*this == rhs) {
                *this = 0;
                return *this;
            }
            *this = subtract(*this, rhs);
            return *this;
        }
        glhint operator-(const glhint& rhs) const {
            glhint result = *this;
            result -= rhs;
            return result;
        }

        glhint operator*=(const glhint& rhs) {
            *this = multiply(*this, rhs);
            return *this;
        }

        glhint operator*(const glhint& rhs) const {
            glhint result = *this;
            result *= rhs;
            return result;
        }

        glhint operator/=(const glhint& rhs) {
            *this = divide(*this, rhs)[0];
            return *this;
        }
        glhint operator/(const glhint& rhs) const {
            glhint result = *this;
            result /= rhs;
            return result;
        }

        glhint operator%=(const glhint& rhs) {
            auto res = divide(*this, rhs);
            *this = res[1];
            return *this;
        }
        glhint operator%(const glhint& rhs) const {
            glhint result = *this;
            result %= rhs;
            return result;
        }

        friend bool operator==(const glhint& l, const glhint& r) {
            if (l.is_neg != r.is_neg) {
                return false;
            }
            return l.vec == r.vec;
        }

        friend bool operator!=(const glhint& l, const glhint& r) {
            return !(l == r);
        }

        friend bool operator<(const glhint& lhs, const glhint& rhs) {
            return less_than(lhs, rhs);
        }
        friend bool operator>(const glhint& l, const glhint& r) {
            return r < l;
        }
        friend bool operator<=(const glhint& l, const glhint& r) {
            return r >= l;
        }
        friend bool operator>=(const glhint& l, const glhint& r) {
            return !(l < r);
        }

        static glhint abs(const glhint& s) {
            if (!is_negative(s)) return s;
            glhint temp = s;
            temp.is_neg = false;
            return temp;
        }
        static glhint abs(glhint&& s) {
            s.is_neg = false;
            return s;
        }
        static bool is_negative(const glhint& input) {
            return input.is_neg;
        }
        static glhint negate(const glhint& input) {
            glhint temp = input;
            temp.is_neg = !temp.is_neg;
            return temp;
        }
        static glhint negate(glhint&& input) {
            input.is_neg = !input.is_neg;
            return input;
        }

        static bool less_than(const glhint& lhs, const glhint& rhs);
        static int count_digits(const glhint&);

        // **** C-like part ****
        // TO port to kmdf driver
        /* // 2^256*2^256, max=2^512, one vec[i] with 10^9, log10(2^512)=154.1, use 9*18=162 dec-digis
        *  //
        * struct {
        *   BYTE is_neg;
        *   BYTE digits;
        *   BYTE reserved; == const 162 digits = 18 elms
        *   int64_t  vec[18];
        * */
        static bool c_compare_eq(const glhint& lhs, const glhint& rhs) {
            return false;
        }
        static bool c_compare_lt(const glhint& lhs, const glhint& rhs) {
            // TBD.
            return false;
        }
        static bool c_compare_ge(const glhint& lhs, const glhint& rhs) {
            return !(c_compare_lt(lhs, rhs));
        }
        static bool c_compare_le(const glhint& lhs, const glhint& rhs) {
            return c_compare_ge(rhs, lhs);
        }
        static void c_from_msb_hex_chars(glhint& num, const BYTE* data, int len, bool is_negative);  // 0x12AB => data={1,2,A,B},len=2
        static void c_from_msb_dec_chars(glhint& num, const BYTE* data, int len, bool is_negative);  // 10245 => data={1,0,2,4,5},len=5
        static int  c_count_digits(const glhint&);

        // convert
        explicit operator int() const {
            return static_cast<int>(vec.back());
        }
        explicit operator long long() const {
            return static_cast<long long>(vec.back());
        }
        explicit operator std::string() const {
            return (is_neg ? "-" : "") + vector_to_string(vec);
        }

        friend std::ostream& operator<<(std::ostream& os, const glhint& num) {
            os << std::string(num);
            return os;
        }

        // helpers
        static glhint add(const glhint&, const glhint&);
        static glhint subtract(const glhint&, const glhint&);
        static glhint multiply(const glhint&, const glhint&);
        static std::array<glhint, 2> divide(const glhint&, const glhint&);
        //static array<glhint, 2> div_mod(const glhint&, const glhint&);
        //static glhint pow10(int exponent);
        static glhint pow16(int exponent);
        static bool is_bigint(const std::string&);
        static std::vector<long long> string_to_vector(std::string input);
        static std::vector<long long> hex_to_vector(std::string input);
        static std::string vector_to_string(const std::vector<long long>& input);

        // internal use
        //  trim front 0's, pass by value?
        static glhint trim(glhint num) {
            int ei = 0, en = 0;
            glhint temp;
            while (en < num.vec.size() - 1 && num.vec[en] == 0) {
                en++;
            }

            temp.vec.resize(num.vec.size() - en);
            for (ei = 0; ei < temp.vec.size(); ei++)
                temp.vec[ei] = num.vec[ei + en];
            temp.is_neg = num.is_neg;
            return temp;
        }
    };

    inline bool glhint::less_than(const glhint& lhs, const glhint& rhs) {
        if (is_negative(lhs) && is_negative(rhs)) {
            return less_than(abs(rhs), abs(lhs));
        }

        if (is_negative(lhs) || is_negative(rhs)) {
            return is_negative(lhs);
        }

        if (lhs.vec.size() == rhs.vec.size()) {
            return lhs.vec < rhs.vec;
        }

        // todo: lexi-order compare
        return lhs.vec.size() < rhs.vec.size();
    }

    inline glhint glhint::add(const glhint& lhs, const glhint& rhs) {
        bool negate_answer = false;
        // Ensure both are positive
        if (is_negative(lhs) && is_negative(rhs)) negate_answer = true;
        else if (is_negative(lhs)) return subtract(rhs, abs(lhs));
        else if (is_negative(rhs)) return subtract(lhs, abs(rhs));

        // Ensure LHS is larger than RHS
        if (lhs.vec.size() < rhs.vec.size()) return add(rhs, lhs);

        // Prepare result vector with enough space (max size + 1 for potential carry)
        // MSByte endian
        std::vector<long long> result;
        result.resize(lhs.vec.size() + 1);  // direct access by [] after reserve() will fail too

        long long carry = 0;
        int ln, li, rn, ri, res_i;
        ln = (int)lhs.vec.size();
        rn = (int)rhs.vec.size();
        res_i = 0;

        li = ln - 1;
        ri = rn - 1;
        while (li >= 0) {
            long long _sum = lhs.vec[li] + carry;
            if (ri >= 0) {
                _sum += rhs.vec[ri];
                ri--;
            }

            if (_sum >= MAX_SIZE) {
                _sum -= MAX_SIZE;
                carry = 1;
            }
            else {
                carry = 0;
            }

            result[res_i] = _sum;
            res_i++;
            //result.push_back(_sum);

            li--;
        }

        if (carry) {
            result[res_i] = carry;
            res_i++;
            carry = 0;
        }

        result.resize(res_i);
        std::reverse(result.begin(), result.end());
        glhint result_bigint{ std::move(result) };

        return negate_answer ? negate(result_bigint) : result_bigint;
        //return result_bigint;
    }
    inline glhint glhint::subtract(const glhint& lhs, const glhint& rhs) {
        // Ensure LHS is larger than RHS, and both are positive
        // (-A) - (-B) == B - A
        if (is_negative(lhs) && is_negative(rhs)) {
            return subtract(abs(rhs), abs(lhs));
        }
        // (A) - (-B) == A + B
        if (is_negative(rhs)) {
            return add(lhs, abs(rhs));
        }
        // (-A) - (B) == -(A + B)
        if (is_negative(lhs)) {
            return negate(add(abs(lhs), rhs));
        }
        if (lhs < rhs) {
            return negate(subtract(rhs, lhs));
        }

        std::vector<long long> result;
        //result.reserve(lhs.vec.size());
        result.resize(lhs.vec.size());
        long long borrow = 0;
        int li, ln, ri, rn, res_i;

        ln = (int)lhs.vec.size();
        li = ln - 1;
        rn = (int)rhs.vec.size();
        ri = rn - 1;
        res_i = 0;
        while (li >= 0) {
            long long rval{ 0 }, diff{ 0 };

            if (ri >= 0) {
                rval = rhs.vec[ri];
                ri--;
            }

            diff = lhs.vec[li] - rval - borrow;

            if (diff < 0) {
                diff += MAX_SIZE;
                borrow = 1;
            }
            else {
                borrow = 0;
            }

            //result.push_back(diff);
            result[res_i] = diff;
            res_i++;

            li--;
        }

        result.resize(res_i);

        std::reverse(result.begin(), result.end());
        return trim(glhint(std::move(result)));
    }

    inline glhint glhint::multiply(const glhint& lhs, const glhint& rhs) {
        if (lhs == 0 || rhs == 0) return 0;
        if (lhs == 1) return rhs;
        if (rhs == 1) return lhs;

        if (is_negative(lhs) && is_negative(rhs)) {
            return (abs(lhs) * abs(rhs));
        }
        if (is_negative(lhs) || is_negative(rhs)) {
            return negate(abs(lhs) * abs(rhs));
        }
        if (lhs < rhs) {
            return multiply(rhs, lhs);
        }

        std::vector<long long> result(lhs.vec.size() + rhs.vec.size(), 0);
        int li, ln, ri, rn, res_i, res_h, res_n;
        long long mul{ 0 };

        res_n = result.size();
        ln = lhs.vec.size();
        rn = rhs.vec.size();
        for (li = ln - 1; li >= 0; li--) {
            for (ri = rn - 1; ri >= 0; ri--) {
                // calc product and result's index
                mul = lhs.vec[li] * rhs.vec[ri];
                res_i = res_n - 1 - ((ln - 1 - li) + (rn - 1 - ri));
                res_h = res_i - 1;

                // add to result vector
                result[res_i] += mul % MAX_SIZE;
                if (res_h >= 0) {
                    result[res_h] += mul / MAX_SIZE;
                }

                // possible carry
                if (result[res_i] >= MAX_SIZE) {
                    if (res_h >= 0) {
                        result[res_h] += result[res_i] / MAX_SIZE;
                    }
                    result[res_i] %= MAX_SIZE;
                }
            }
        }

        // Handle carries for remaining positions
        for (res_i = res_n - 1; res_i > 0; res_i--) {
            if (result[res_i] >= MAX_SIZE) {
                result[res_i - 1] += result[res_i] / MAX_SIZE;
                result[res_i] %= MAX_SIZE;
            }
        }

        return trim(result);
    }

    inline std::array<glhint, 2> glhint::divide(const glhint& numerator, const glhint& denominator) {
        if (denominator == 0) {
            throw std::domain_error("Attempted to divide by zero.");
        }
        if (numerator == denominator) return { 1,0 };
        if (denominator == 1) return { numerator, 0 };
        if (numerator == 0) return { 0, 0 };

        if (is_negative(numerator) && is_negative(denominator)) {
            return divide(abs(numerator), abs(denominator));
        }
        if (is_negative(numerator) || is_negative(denominator)) {
            // no valid definition for negative remainder?
            //return negate(divide(abs(numerator), abs(denominator)));
            auto result = divide(abs(numerator), abs(denominator));
            return { negate(result[0]), result[1] };
        }
        if (numerator < denominator) {
            return { 0, numerator };
        }
        if (numerator.vec.size() <= 1) {
            return { numerator.vec.back() / denominator.vec.back(), numerator.vec.back() % denominator.vec.back() };
        }

        glhint remainder = numerator;
        glhint quotient = 0;
        
        while (remainder >= denominator) {
            //int count = count_digits(remainder) - count_digits(denominator) - 1;
            int count = c_count_digits(remainder) - c_count_digits(denominator) - 1;

            // Prevent negative exponents which cause pow() to return 0
            if (count < 0) {
                count = 0;
            }

            glhint numerator_size = pow16(count);
            glhint temp = denominator * numerator_size;

            // Repeatedly subtract the chunk from the remainder
            while (remainder >= temp) {
                remainder -= temp;
                //std::cout << remainder << "\n";
                quotient += numerator_size;
            }
        }
        //std::cout << denominator << " (de)\n";

        return { quotient, remainder };
    }

    inline glhint glhint::pow16(int exponent) {
        glhint result;
        std::vector<BYTE> data(exponent + 1, 0);
        if (exponent < 0) {
            c_from_msb_hex_chars(result, &data[0], 1, false);
        }
        else if (exponent == 0) {
            data[0] = 1;
            c_from_msb_hex_chars(result, &data[0], 1, false);
        }
        else if (exponent == 1) {
            data[0] = 1;
            data[1] = 0;
            c_from_msb_hex_chars(result, &data[0], 2, false);
        }
        else {
            data[0] = 1;
            c_from_msb_hex_chars(result, &data[0], exponent + 1, false);
        }
        return result;
    }
    inline bool glhint::is_bigint(const std::string& s) {
        return glbigint::is_bigint(s);
    }
    inline std::vector<long long> glhint::string_to_vector(std::string input) {
        
        std::vector<long long> result = { 0 };

        for (const char c : input) {
            unsigned long long digit;
            if (c >= '0' && c <= '9')      digit = c - '0';
            else throw std::runtime_error("Invalid hex character.");

            // Multiply current value by 16 and add digit.
            // Max intermediate value: (MAX_SIZE-1)*16+15 = 15,999,999,999
            // which fits in unsigned long long.
            unsigned long long carry = digit;
            for (int i = static_cast<int>(result.size()) - 1; i >= 0; --i) {
                const unsigned long long val = static_cast<unsigned long long>(result[i]) * 10 + carry;
                result[i] = static_cast<long long>(val % static_cast<unsigned long long>(MAX_SIZE));
                carry = val / static_cast<unsigned long long>(MAX_SIZE);
            }
            // carry <= 9 < MAX_SIZE, so this loop runs at most once
            while (carry > 0) {
                result.insert(result.begin(), static_cast<long long>(carry % static_cast<unsigned long long>(MAX_SIZE)));
                carry /= static_cast<unsigned long long>(MAX_SIZE);
            }
        }

        return result;
    }

    inline std::vector<long long> glhint::hex_to_vector(std::string input) {
        // Strip "0x" prefix, and possible leading 0's
        // ex. 0x0123 => 0x123
        int zeros = 0;
        while ((zeros + 2) < input.length() && input[zeros + 2] == '0')
            zeros++;

        std::string hex = input.substr(2 + zeros);

        // Break into chunks of 9 characters
        std::vector<long long> result;
        constexpr int chunk_size = CHUNK_SIZE;
        const int size = hex.size();

        if (size > chunk_size) {
            // Pad the length to get appropriate sized chunks
            if (int mod = size % chunk_size; mod != 0) {
                hex.insert(0, chunk_size - mod, '0');
            }
        }
        for (int i = 0; i < hex.size(); i += chunk_size) {
            std::string temp_str = hex.substr(i, chunk_size);
            result.emplace_back(stoll(temp_str, nullptr, 16));
        }

        return result;
    }
    inline std::string glhint::vector_to_string(const std::vector<long long>& input) {
        std::stringstream ss;
        bool first = true;
        for (const auto val : input) {
            if (first) {
                ss << "0x" << std::hex << val;
                first = false;
            }
            else {
                ss << std::setw(CHUNK_SIZE) << std::hex << std::setfill('0') << val;
            }
        }
        return ss.str();
    }
    // **** C-like Func implement ****
    inline void glhint::c_from_msb_hex_chars(glhint& num, const BYTE* data, int len, bool is_negative) {
        // each BYTE[] = a single 4-bit (0~F) number
        // refer to original 'string to vector'
        // turn data[] to num
        int chunk_size = CHUNK_SIZE;
        int i, j, di, shift, chunk_num;
        long long val;

        chunk_num = (len + chunk_size - 1) / chunk_size;
        num.vec.resize(chunk_num);
        val = 0;
        di = 0;

        shift = len % chunk_size;
        if (shift == 0)
            shift = chunk_size;

        for (i = 0; i < chunk_num; i++) {

            for (j = 0; j < shift; j++, di++)
                val = val * 16 + (long long)data[di];

            num.vec[i] = val;
            shift = chunk_size;
            val = 0;
        }
    }
    inline void glhint::c_from_msb_dec_chars(glhint& num, const BYTE* data, int len, bool is_negative) {
        // each BYTE[] = a single decimal digit 0~9 number
        throw::std::runtime_error("TBD.!!");
    }

    inline int glhint::c_count_digits(const glhint& num) {
        // only meaningful if num positive
        if (num.vec.size() == 0) return 0; // invalid case

        long long val = num.vec[0];
        int digi = 0;

        while (val) {
            val /= 16;
            digi++;
        }
        if (num.vec.size() > 1)
            digi += ((int)num.vec.size() - 1) * CHUNK_SIZE;
        return digi;
    }
}


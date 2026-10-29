#!/usr/bin/env python
# -*- coding: utf-8 -*-
# Copyright (c) 2026 Genesys Logic. All rights reserved.
# Project: test big Int operations

import os
import sys
import numpy as np
import random

# nistp256 parameters
ECC_P  = 0xFFFFFFFF00000001000000000000000000000000FFFFFFFFFFFFFFFFFFFFFFFF
#ECC_P  = 0xFFFFFFFF00000000FFFFFFFFFFFFFFFFBCE6FAADA7179E84F3B9CAC2FC632551 #fake P
ECC_A  = 0xFFFFFFFF00000001000000000000000000000000FFFFFFFFFFFFFFFFFFFFFFFC
ECC_B  = 0x5AC635D8AA3A93E7B3EBBD55769886BC651D06B0CC53B0F63BCE3C3E27D2604B
ECC_N  = 0xFFFFFFFF00000000FFFFFFFFFFFFFFFFBCE6FAADA7179E84F3B9CAC2FC632551
ECC_Gx = 0x6B17D1F2E12C4247F8BCE6E563A440F277037D812DEB33A0F4A13945D898C296
ECC_Gy = 0x4FE342E2FE1A7F9B8EE7EB4A7C0F9E162BCE33576B315ECECBB6406837BF51F5

ECC_P256 = dict({
    'P' : ECC_P,
    'A' : ECC_A,
    'B' : ECC_B,
    'N' : ECC_N,
    'Gx' : ECC_Gx,
    'Gy' : ECC_Gy
})

# y^2=x^3+2x+2 (mod 17), N = 19
ECC_SIMPLE = dict({
    'P' : 17,
    'A' : 2,
    'B' : 2,
    'N' : 19,
    'Gx' : 5,
    'Gy' : 1
})

def mod_add(a, b, N):
    # (a + b) % N
    ret = a + b
    if ret > N:
        ret -= N
    return ret

def mod_sub(a, b, N):
    # (a - b) % N
    ret = a - b
    if ret < 0:
        ret += N
    return ret

def mod_mult(a, b, N):
    # must ensure a>0, b>0
    if a < 0 or b < 0:
        print('invalid mult range')
        sys.exit()
        
    ret = a * b
    ret %= N
    return ret

def ecc_point_add(ecc, px, py, qx, qy):
    # ecc: curve parameters
    # R = P + Q
    # assume P!=Q
    # if px==py==0... return 0, 0
    if px == qx and py == qy:
        return ecc_point_double(ecc, px, py)
    
    _P = ecc['P']
    t0  = mod_sub(qy, py, _P)
    t1  = mod_sub(qx, px, _P)
    t1  = mod_inv(t1, _P)  # t0 / t1
    lam = (t0 * t1) % _P

    t0 = mod_mult(lam, lam, _P)
    t0 = mod_sub(t0, px, _P)
    Rx = mod_sub(t0, qx, _P)

    t1 = mod_sub(px, Rx, _P)
    t1 = mod_mult(t1, lam, _P)
    Ry = mod_sub(t1, py, _P)
    
    return Rx, Ry

def ecc_point_double(ecc, px, py):
    # 3*t = 2*t + t
    _P = ecc['P']
    _A = ecc['A']
    t0 = mod_mult(px, px, _P)
    t1 = mod_add(t0, t0, _P)
    t1 = mod_add(t1, t0, _P) #3x^2
    t1 = mod_add(t1, _A, _P)

    t0 = mod_add(py, py, _P)
    t0 = mod_inv(t0, _P)

    lam = mod_mult(t0, t1, _P)

    t0 = mod_mult(lam, lam, _P)
    t0 = mod_sub(t0, px, _P)
    Rx = mod_sub(t0, px, _P)

    t1 = mod_sub(px, Rx, _P)
    t1 = mod_mult(t1, lam, _P)
    Ry = mod_sub(t1, py, _P)
    
    return Rx, Ry
    

def run_euclidean(a, n, p):
    # find x, y, s.t. a*x+n*y = 1
    # note n, p are primes, a & n co-prime
    # return (x, y)
    
    sx = 1
    sy = 0
    tx = 0
    ty = 1
    A, B = a, n
    
    while B > 0:
        q = A // B
        r = A - q * B
        A, B = B, r
        rx = sx - q * tx
        ry = sy - q * ty
        sx, sy = tx, ty
        tx, ty = rx, ry
    
    return sx, sy

def run_euclidean_mod(a, n, p):
    # find x, y, s.t. a*x+n*y = 1 (mod n?)
    # note n, p are primes, a & n co-prime
    # since n*y≡0(mod n), no need to return y
    # return (x, y)
    
    sx = 1
    sy = 0
    tx = 0
    ty = 1
    A, B = a, n
    
    while B > 0:
        q = A // B   # shoud < n
        r = A - q * B
        #if r < 0:
        #    r += n
            
        A, B = B, r
        rx = sx - q * tx
        #ry = sy - q * ty
        #sx, sy = tx, ty
        #tx, ty = rx, ry
        sx = tx
        tx = rx

    if sx < 0:
        sx += n
    #if sy < 0:
    #    sy = sy + abs(sy) // n * n + n
        
    return sx, 0

def mod_inv(x, p):
    # return x^-1 % p
    # p must be prime
    inv, _ = run_euclidean_mod(x, p, 1)
    return inv

def gen_temp_tests():
    #tests = [[240, 17, 10007],
    #         [240, 23, 10007],]
    tests = [ [240, 17, 10007] ]
    
    # gen test sets
    big_prm = 10007
    small_prm = [11, 13, 17, 23, 41, 47, 59, 97]
    target_prm = [2, 3, 5, 7]
    test_runs = 10
    random.seed(0)  #fix it
    for i in range(test_runs):
        single = [1, 0, big_prm]
        idx = random.randint(0, len(small_prm)-1)
        single[1] = small_prm[idx]

        factors = random.randint(3, 7)
        for j in range(factors):
            idx = random.randint(0, len(target_prm)-1)
            single[0] = single[0] * target_prm[idx]
        tests.append(single)

    return tests

def run_ecc_add_test():
    print('Test ECC Add')
    ecc = ECC_P256
    Px = 0xb18d33dd457f646ac75ba62cd2d21cb5922aae58f5aafa09b78d5bc5816194ee
    Py = 0xe94df3e38e041e91220463803ca8821824e0a8f882ea7c1a5ec97571c0a0cc7b
    Qx = 0xab7eee3a60797b28b911afb6e0e423e01b144fe71ba236d77dd6ada1718c5d39
    Qy = 0x0b6041cf92686dd3ae0ed9d7ba05815684a7cfc300a2a0848a3ba12bd68ccf8b
    Rx, Ry = ecc_point_add(ecc, Px, Py, Qx, Qy)
    print('Px', hex(Px))
    print('Py', hex(Py))
    print('Qx', hex(Qx))
    print('Qy', hex(Qy))
    print('Rx', hex(Rx))
    print('Ry', hex(Ry))

    ecc = ECC_SIMPLE
    test_pts = [[[5,1], [5,1], [6,3]],\
                [[6,3], [10,6], [9,16]],\
                [[3,1], [7,6], [16,4]],\
                [[16,13], [13,7], [9,1]]]
    for item in test_pts:
        _x, _y = ecc_point_add(ecc, item[0][0], item[0][1], item[1][0], item[1][1])
        print(f'add: {_x},{_y}, ans={item[2][0]},{item[2][1]}')
    
def main():
    """main entry"""
    # test euclidean first
    #tests = gen_temp_tests()
    tests = []
    
    for ti in tests:
        # normal version
        #x, y = run_euclidean(ti[0], ti[1], ti[2])
        #ans = ti[0] * x + ti[1] * y
        #_x = f'{x:8d}' if x > 0 else f'({x:6d})'
        #_y = f'{y:8d}' if y > 0 else f'({y:6d})'
        #print(f'{ti[0]:6d}*{_x} + {ti[1]:6d}*{_y} = {ans}')

        # modular version
        x, y = run_euclidean_mod(ti[0], ti[1], ti[2])
        ans = (ti[0] * x + ti[1] * y) % ti[1]
        _x = f'{x:8d}'# if x > 0 else f'({x:6d})'
        _y = f'{y:8d}'# if y > 0 else f'({y:6d})'
        print(f'{ti[0]:6d}*{_x} + {ti[1]:6d}*{_y} ≡ {ans:3d} mod({ti[1]:4d})')
    # test some int inverse
    run_ecc_add_test()
    

if __name__ == '__main__':
    main()

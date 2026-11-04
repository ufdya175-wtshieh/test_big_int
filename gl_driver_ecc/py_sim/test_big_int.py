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
    
def ecc_point_scale(ecc, px, py, k):
    
    tx, ty = px, py
    
    # ex. 0b1111111001
    kAsBinary = bin(k) 
    
    # 1111111001
    kAsBinary = kAsBinary[2:len(kAsBinary)] 
    # print(kAsBinary)
    
    for i in range(1, len(kAsBinary)):
        currentBit = kAsBinary[i: i+1]
        
        # always apply doubling
        tx, ty = ecc_point_double(ecc, tx, ty)
        
        if currentBit == '1':
            # add base point
            tx, ty = ecc_point_add(ecc, tx, ty, px, py)
    
    return tx, ty

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

def ecc_verify(sigr, sigs, msg):
    """Verify ECDSA signature"""
    priv = 0x3c6094b39e43651a965970cff68c637a1c381289aadfe8c8d95d2674fe1830db
    pubx = 0xc89498d9f1c7fd5cf0c116c9b27ef68b5ef654450a61289ce2075370ad61485d
    puby = 0x41f2d5dd843b3cf33c37e0bba932bb5a1c40f5bd548b36e61cbac5ed956418ce

    #w = s^-1
    #u1 = w*h(x)
    #u2 = w*r
    #P = u1*G + u2*B .. point scaling
    #P.x == r ?
    w = mod_inv(sigs, ECC_N)
    t1 = mod_mult(w, sigs, ECC_N)
    print(" verify", t1)
    u1 = mod_mult(w, msg, ECC_N)
    u2 = mod_mult(w, sigr, ECC_N)
    t1x, t1y = ecc_point_scale(ECC_P256, ECC_Gx, ECC_Gy, u1)
    t2x, t2y = ecc_point_scale(ECC_P256, pubx, puby, u2)
    Px, Py = ecc_point_add(ECC_P256, t1x, t1y, t2x, t2y)
    print("px:", Px)
    print("_r:", sigr)
    if Px == sigr:
        print("signature verify OK")
    else:
        print("signature verify FAIL!")

def run_ecc_verify_test():
    #sigr = 0x4C75B209490ED920E9B6CB8280018AA72ACC4173B0C38F25690BE44DD843085E
    #sigs = 0xBA0485D19A65EE094803F73671D3BC7B8F0EB27A88F3D8E1A77BC54A7EEDA66C
    sigr = 0xba56140bdd55e71e04dafe613bca9662508d013b1df0449b47f0a61f19023e89
    sigs = 0x4881abeea7ce7c7942b4a50ac4484882b005829cac0df63478139a10ad87a8e9
    msg  = 0x4876cc70c76a799537e723288fc2d8e86601ca096ca5ab363c358bc66300e335  #h(x)

    ecc_verify(sigr, sigs, msg)

    # test point scaling
    '''px = 0xea75368e527e7b41b72e998f4baafe89f9abfb6002350c5544e0ea8752a16704
    py = 0x7d6ed056f81a7b6e9c6e8772881cba17dc86475f4ce547b4abe2f6254fe6bcff
    k = 0xbdfd501ac014cf8e7fe74a7cbf2c5e555a2c7d53c708f7199a687a3a6f7a0841
    rx = 0x7931144ff9c7d8a1cc190a7cabc35110f04acb803ae3540b239e2d165ab8f8f5
    ry = 0x24d6312db9c53be57cee80c0f268a738310be9f3ba3fb505169a08b21ec91c37
    tx, ty = ecc_point_scale(ECC_P256, px, py, k)
    print("tx:", hex(tx))
    print("ty:", hex(ty))
    print("test ok" if rx==tx and ry==ty else "test fail")'''

def run_ecc_sign_test():
    priv = 0x3c6094b39e43651a965970cff68c637a1c381289aadfe8c8d95d2674fe1830db
    msg  = 0x4876cc70c76a799537e723288fc2d8e86601ca096ca5ab363c358bc66300e335
    ke   = 0x5010101010101010101010101010101010101010101010101010101010101010

    # r = x of R=kG, k:rand
    # s = (msg + d*r) / rand
    rx, ry = ecc_point_scale(ECC_P256, ECC_Gx, ECC_Gy, ke)
    ki = mod_inv(ke, ECC_N)
    s = mod_mult(priv, rx, ECC_N)
    s = mod_add(s, msg, ECC_N)
    s = mod_mult(s, ki, ECC_N)

    return rx, s
    ...
    
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
    #run_ecc_add_test()
    run_ecc_verify_test()
    _r, _s = run_ecc_sign_test()
    print("sign r:", hex(_r))
    print("sign s:", hex(_s))
    
if __name__ == '__main__':
    main()

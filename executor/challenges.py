import math

def reverse_digits(num):
    return int(str(num)[::-1])


def list_prime_factors(num):
    primes = []
    for i in range(2, int(math.sqrt(num))):
        while num % i == 0:
            add = True
            for p in primes:
                if i % p == 0:
                    add = False
                    break
            if add:
                primes.append(i)
            num = num // i
    if num not in primes:
        primes.append(num)
    return primes


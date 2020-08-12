import math

CHALLENGES = ['reverse_digits', 'list_prime_factors']

def reverse_digits(num):
    return int(str(num)[::-1])


def list_prime_factors(num):
    primes = []
    for i in range(2, int(math.sqrt(num))):
        if num % i == 0:
            add = True
            for p in primes:
                if i % p == 0:
                    add = False
                    break
            if add:
                primes.append(i)
            while num % i == 0:
                num = num // i
    if num not in primes and num != 1:
        primes.append(num)
    return primes


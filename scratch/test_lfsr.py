def lfsr(n, size, xor):
    m = n << 1
    x = m & (1 << size)
    if x:
        m = m - x + 1
    m ^= x >> xor
    return m

def check_period(size, xor):
    start = 1
    curr = start
    period = 0
    while True:
        curr = lfsr(curr, size, xor)
        period += 1
        if curr == start:
            break
        if period > 5000:
            break
    print(f"Size {size}, xor {xor} -> Period {period}")

check_period(2, 1)
check_period(3, 1)
check_period(5, 2)
check_period(7, 1)
check_period(11, 2)

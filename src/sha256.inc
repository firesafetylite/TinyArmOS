/* Shared bounded SHA-256 implementation used by updates and authentication. */
typedef struct {
    UINT32 state[8];
    UINT64 bitLength;
    UINT8 block[64];
    UINTN blockLength;
} SHA256_CONTEXT;

static UINT32 sha256_rotr32(UINT32 value, UINT32 count) {
    return (value >> count) | (value << (32U - count));
}

static void sha256_transform(SHA256_CONTEXT *context, const UINT8 *block) {
    static const UINT32 constants[64] = {
        0x428a2f98U,0x71374491U,0xb5c0fbcfU,0xe9b5dba5U,0x3956c25bU,0x59f111f1U,0x923f82a4U,0xab1c5ed5U,
        0xd807aa98U,0x12835b01U,0x243185beU,0x550c7dc3U,0x72be5d74U,0x80deb1feU,0x9bdc06a7U,0xc19bf174U,
        0xe49b69c1U,0xefbe4786U,0x0fc19dc6U,0x240ca1ccU,0x2de92c6fU,0x4a7484aaU,0x5cb0a9dcU,0x76f988daU,
        0x983e5152U,0xa831c66dU,0xb00327c8U,0xbf597fc7U,0xc6e00bf3U,0xd5a79147U,0x06ca6351U,0x14292967U,
        0x27b70a85U,0x2e1b2138U,0x4d2c6dfcU,0x53380d13U,0x650a7354U,0x766a0abbU,0x81c2c92eU,0x92722c85U,
        0xa2bfe8a1U,0xa81a664bU,0xc24b8b70U,0xc76c51a3U,0xd192e819U,0xd6990624U,0xf40e3585U,0x106aa070U,
        0x19a4c116U,0x1e376c08U,0x2748774cU,0x34b0bcb5U,0x391c0cb3U,0x4ed8aa4aU,0x5b9cca4fU,0x682e6ff3U,
        0x748f82eeU,0x78a5636fU,0x84c87814U,0x8cc70208U,0x90befffaU,0xa4506cebU,0xbef9a3f7U,0xc67178f2U
    };
    UINT32 words[64];
    UINT32 a;
    UINT32 b;
    UINT32 c;
    UINT32 d;
    UINT32 e;
    UINT32 f;
    UINT32 g;
    UINT32 h;
    UINT32 index;
    for (index = 0; index < 16U; index++) {
        UINTN offset = (UINTN)index * 4U;
        words[index] = ((UINT32)block[offset] << 24) |
                       ((UINT32)block[offset + 1U] << 16) |
                       ((UINT32)block[offset + 2U] << 8) |
                       (UINT32)block[offset + 3U];
    }
    for (index = 16U; index < 64U; index++) {
        UINT32 s0 = sha256_rotr32(words[index - 15U], 7U) ^
                    sha256_rotr32(words[index - 15U], 18U) ^
                    (words[index - 15U] >> 3U);
        UINT32 s1 = sha256_rotr32(words[index - 2U], 17U) ^
                    sha256_rotr32(words[index - 2U], 19U) ^
                    (words[index - 2U] >> 10U);
        words[index] = words[index - 16U] + s0 + words[index - 7U] + s1;
    }
    a = context->state[0];
    b = context->state[1];
    c = context->state[2];
    d = context->state[3];
    e = context->state[4];
    f = context->state[5];
    g = context->state[6];
    h = context->state[7];
    for (index = 0; index < 64U; index++) {
        UINT32 sum1 = sha256_rotr32(e, 6U) ^ sha256_rotr32(e, 11U) ^ sha256_rotr32(e, 25U);
        UINT32 choose = (e & f) ^ ((~e) & g);
        UINT32 temp1 = h + sum1 + choose + constants[index] + words[index];
        UINT32 sum0 = sha256_rotr32(a, 2U) ^ sha256_rotr32(a, 13U) ^ sha256_rotr32(a, 22U);
        UINT32 majority = (a & b) ^ (a & c) ^ (b & c);
        UINT32 temp2 = sum0 + majority;
        h = g;
        g = f;
        f = e;
        e = d + temp1;
        d = c;
        c = b;
        b = a;
        a = temp1 + temp2;
    }
    context->state[0] += a;
    context->state[1] += b;
    context->state[2] += c;
    context->state[3] += d;
    context->state[4] += e;
    context->state[5] += f;
    context->state[6] += g;
    context->state[7] += h;
    secure_zero(words, sizeof(words));
}

static void sha256_init(SHA256_CONTEXT *context) {
    context->state[0] = 0x6a09e667U;
    context->state[1] = 0xbb67ae85U;
    context->state[2] = 0x3c6ef372U;
    context->state[3] = 0xa54ff53aU;
    context->state[4] = 0x510e527fU;
    context->state[5] = 0x9b05688cU;
    context->state[6] = 0x1f83d9abU;
    context->state[7] = 0x5be0cd19U;
    context->bitLength = 0;
    context->blockLength = 0;
}

static void sha256_add(SHA256_CONTEXT *context, const UINT8 *data, UINTN bytes) {
    UINTN index;
    for (index = 0; index < bytes; index++) {
        context->block[context->blockLength++] = data[index];
        if (context->blockLength == 64U) {
            sha256_transform(context, context->block);
            context->bitLength += 512U;
            context->blockLength = 0;
        }
    }
}

static void sha256_finish(SHA256_CONTEXT *context, UINT8 digest[32]) {
    UINTN index = context->blockLength;
    UINT64 totalBits;
    context->block[index++] = 0x80U;
    if (index > 56U) {
        while (index < 64U) context->block[index++] = 0;
        sha256_transform(context, context->block);
        index = 0;
    }
    while (index < 56U) context->block[index++] = 0;
    totalBits = context->bitLength + (UINT64)context->blockLength * 8U;
    for (index = 0; index < 8U; index++) {
        context->block[63U - index] = (UINT8)(totalBits >> (index * 8U));
    }
    sha256_transform(context, context->block);
    for (index = 0; index < 8U; index++) {
        digest[index * 4U] = (UINT8)(context->state[index] >> 24);
        digest[index * 4U + 1U] = (UINT8)(context->state[index] >> 16);
        digest[index * 4U + 2U] = (UINT8)(context->state[index] >> 8);
        digest[index * 4U + 3U] = (UINT8)context->state[index];
    }
    secure_zero(context, sizeof(*context));
}

static void sha256_bytes(const UINT8 *data, UINTN bytes, UINT8 digest[32]) {
    SHA256_CONTEXT context;
    sha256_init(&context);
    sha256_add(&context, data, bytes);
    sha256_finish(&context, digest);
}
